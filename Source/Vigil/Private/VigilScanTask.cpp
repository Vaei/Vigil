// Copyright (c) Jared Taylor


#include "VigilScanTask.h"

#include "VigilComponent.h"
#include "VigilNetSyncTask.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

#if !UE_BUILD_SHIPPING
#include "Logging/MessageLog.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilScanTask)

namespace FVigilCVars
{
#if UE_ENABLE_DEBUG_DRAWING
	static int32 VigilScanDebug = 0;
	FAutoConsoleVariableRef CVarVigilScanDebug(
		TEXT("p.Vigil.Scan.Debug"),
		VigilScanDebug,
		TEXT("Optionally draw debug for Vigil Scan Task.\n")
		TEXT("0: Disable, 1: Enable for all, 2: Enable for local player only"),
		ECVF_Default);
#endif
}

UVigilScanTask* UVigilScanTask::VigilScan(UGameplayAbility* OwningAbility)
{
	UVigilScanTask* MyObj = NewAbilityTask<UVigilScanTask>(OwningAbility);
	return MyObj;
}

void UVigilScanTask::Activate()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::Activate);
	
	UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::Activate"), *GetRoleString());
	
	SetWaitingOnAvatar();
	RequestVigil();
}

void UVigilScanTask::WaitForVigil(float Delay)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::WaitForVigil);
	
	if (!IsValid(GetWorld()))
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::WaitForVigil: Invalid world. [SYSTEM END]"), *GetRoleString());
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(VigilWaitTimer, this, &UVigilScanTask::RequestVigil, Delay, false);
}

void UVigilScanTask::RequestVigil()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::RequestVigil);
	
	// Cache the VigilComponent if required
	if (!VC.IsValid())
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Caching VigilComponent"), *GetRoleString());
		
		// Get the owning player controller
		const TWeakObjectPtr<AActor>& WeakOwner = Ability->GetCurrentActorInfo()->OwnerActor;
		const APlayerController* PlayerController = nullptr;
		if (const APawn* Pawn = Cast<APawn>(WeakOwner.Get()))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Owner is a pawn"), *GetRoleString());
			PlayerController = Pawn->GetController<APlayerController>();
		}
		else if (const APlayerState* PlayerState = Cast<APlayerState>(WeakOwner.Get()))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Owner is a player state"), *GetRoleString());
			PlayerController = PlayerState->GetPlayerController();
		}
		else
		{
			UE_LOG(LogVigil, Error, TEXT("%s VigilScanTask::RequestVigil: Owner is not a pawn or player state"), *GetRoleString());
		}
		
		// If the player controller is not valid, wait for a bit and try again
		if (UNLIKELY(!PlayerController))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Invalid player controller. [SYSTEM WAIT]"), *GetRoleString());
			WaitForVigil(0.5f);
			return;
		}

		// Find the VigilComponent on the player controller
		VC = PlayerController->FindComponentByClass<UVigilComponent>();
		if (!VC.IsValid())
		{
#if !UE_BUILD_SHIPPING
			if (IsInGameThread())
			{
				FMessageLog ("PIE").Error(FText::Format(
					NSLOCTEXT("VigilScanTask", "VigilComponentNotFound", "VigilComponent not found on {0}"),
					FText::FromString(PlayerController->GetName())));
			}
#endif

			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Invalid VigilComponent. [SYSTEM END]"), *GetRoleString());
			
			// Vigil will not run at all
			return;
		}

		// Bind to the pause delegate
		if (!VC->OnPauseVigil.IsBoundToObject(this))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Binding to OnPauseVigil"), *GetRoleString());
			VC->OnPauseVigil.BindUObject(this, &ThisClass::OnPauseVigil);
		}

		// Bind to request delegate
		if (!VC->OnRequestVigil.IsBoundToObject(this))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Binding to OnRequestVigil"), *GetRoleString());
			VC->OnRequestVigil.BindUObject(this, &ThisClass::OnRequestVigil);
		}

		// Bind to net sync delegate
		if (!VC->OnVigilNetSync.IsAlreadyBound(this, &ThisClass::OnRequestNetSync))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Binding to OnVigilRequestNetSync"), *GetRoleString());
			VC->OnVigilNetSync.AddDynamic(this, &ThisClass::OnRequestNetSync);
		}
	}

	check(VC.IsValid());

	// Are we on cooldown due to rate throttling?
	const float MaxRate = VC->GetMaxVigilScanRate();
	UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::RequestVigil: MaxRate: %.2f"), *GetRoleString(), MaxRate);
	if (MaxRate > 0.f)
	{
		// (Thinking out loud...)
		// If updated at 10.0, rate is 0.1, current time is 10.05
		// TimeSince is 0.05, TimeSince < MaxRate == 0.05 < 0.1 == true
		// TimeLeft = MaxRate - TimeSince == 0.1 - 0.05 == 0.05
		
		const float TimeSince = GetWorld()->TimeSince(VC->LastVigilScanTime);
		UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::RequestVigil: TimeSince: %.2f"), *GetRoleString(), TimeSince);
		if (TimeSince < MaxRate)
		{
			const float TimeLeft = MaxRate - TimeSince;
			UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::RequestVigil: TimeLeft: %.2f [SYSTEM WAIT]"), *GetRoleString(), TimeLeft);
			WaitForVigil(TimeLeft);
			return;
		}
	}

	// Check if the world and game instance are valid
	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Invalid world or game instance. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(0.5f);
		return;
	}

	// Get the TargetingSubsystem
	UTargetingSubsystem* TargetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UTargetingSubsystem>();
	if (!TargetSubsystem)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Invalid TargetingSubsystem. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(0.5f);
		return;
	}

	AActor* TargetingSource = VC->GetTargetingSource();
	if (!TargetingSource)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Invalid TargetingSource. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(0.5f);
		return;
	}
	
	// Check for changes to the targeting preset update mode
	if (VC->bUpdateTargetingPresetsOnPawnChange != VC->bLastUpdateTargetingPresetsOnPawnChange)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: TargetingPresetUpdateMode changed."), *GetRoleString());
		// Remove or bind the pawn changed binding
		VC->UpdatePawnChangedBinding();
		VC->bLastUpdateTargetingPresetsOnPawnChange = VC->bUpdateTargetingPresetsOnPawnChange;
	}

	// Optionally update the targeting presets
	if (VC->bUpdateTargetingPresetsOnUpdate)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Updating targeting presets."), *GetRoleString());
		VC->UpdateTargetingPresets();
	}

	// Get cached targeting presets
	const TMap<FGameplayTag, UTargetingPreset*>& TargetingPresets = VC->CurrentTargetingPresets;
	UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::RequestVigil: TargetingPresets.Num(): %d"), *GetRoleString(), TargetingPresets.Num());

	if (TargetingPresets.Num() == 0)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: No targeting presets. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(0.5f);
		return;
	}

	bool bAwaitingCallback = false;
	for (const auto& Entry : TargetingPresets)
	{
		const FGameplayTag& Tag = Entry.Key;
		const UTargetingPreset* Preset = Entry.Value;

		if (Preset->TargetingTaskSet.Tasks.IsEmpty())
		{
			// If the only available presets only have empty tasks Vigil will never get a callback
			continue;
		}
		
		FTargetingRequestHandle& Handle = VC->TargetingRequests.FindOrAdd(Tag);
		Handle = TargetSubsystem->MakeTargetRequestHandle(Preset, FTargetingSourceContext {TargetingSource});

		FTargetingAsyncTaskData& AsyncTaskData = FTargetingAsyncTaskData::FindOrAdd(Handle);
		AsyncTaskData.bReleaseOnCompletion = true;

		TargetSubsystem->StartAsyncTargetingRequestWithHandle(Handle,
			FTargetingRequestDelegate::CreateUObject(this, &ThisClass::OnVigilComplete, Tag));

		bAwaitingCallback = true;
		
		UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::RequestVigil: Start async targeting for TargetingPresets[%s]: %s"), *GetRoleString(), *Tag.ToString(), *GetNameSafe(Preset));
	}

	if (!bAwaitingCallback)
	{
		// Failed to start any async targeting requests
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Failed to start async targeting requests - TargetingTaskSet(s) are empty!. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(0.5f);
		return;
	}

#if UE_ENABLE_DEBUG_DRAWING
	if (IsInGameThread() && GEngine && Ability && Ability->GetCurrentActorInfo())
	{
		if (FVigilCVars::VigilScanDebug > 0)
		{
			const bool bIsLocalPlayer = Ability->GetCurrentActorInfo()->IsLocallyControlled();
			if (FVigilCVars::VigilScanDebug == 1 || bIsLocalPlayer)
			{
				// Draw the number of current requests to screen		
				const int32 UniqueKey = (VC->GetUniqueID() + 297) % INT32_MAX;
				const FString Info = FString::Printf(TEXT("Vigil TargetingRequests: %d"), VC->TargetingRequests.Num());
				GEngine->AddOnScreenDebugMessage(UniqueKey, 5.f, FColor::Green, Info);
			}
		}
	}	
#endif
}

void UVigilScanTask::OnVigilComplete(FTargetingRequestHandle TargetingHandle, FGameplayTag FocusTag)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::OnVigilComplete);

	const FString RoleStr = Ability->GetCurrentActorInfo()->IsNetAuthority() ? TEXT("Auth") : TEXT("Client");
	
	UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnVigilComplete: %s"), *GetRoleString(), *FocusTag.ToString());

	if (!VC.IsValid())
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnVigilComplete: Invalid VigilComponent. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(0.5f);
		return;
	}
	
	// Check if the world and game instance are valid
	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnVigilComplete: Invalid world or game instance. [SYSTEM WAIT]"), *GetRoleString());
		VC->EndAllTargetingRequests();
		WaitForVigil(0.5f);
		return;
	}

	// Get the TargetingSubsystem
	const UTargetingSubsystem* TargetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UTargetingSubsystem>();
	if (!TargetSubsystem)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnVigilComplete: Invalid TargetingSubsystem. [SYSTEM WAIT]"), *GetRoleString());
		VC->EndAllTargetingRequests();
		WaitForVigil(0.5f);
		return;
	}

	// Get the results from the TargetingSubsystem
	TArray<FVigilFocusResult> FocusResults;
	if (TargetingHandle.IsValid())
	{
		// Process results
		if (FTargetingDefaultResultsSet* Results = FTargetingDefaultResultsSet::Find(TargetingHandle))
		{
			for (const FTargetingDefaultResultData& ResultData : Results->TargetResults)
			{
				FVigilFocusResult Result = { FocusTag, ResultData.HitResult, ResultData.Score };
				FocusResults.Add(Result);
			}
		}

		// Remove the request handle
		VC->TargetingRequests.Remove(FocusTag);
	}

	UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::OnVigilComplete: FocusResults.Num(): %d"), *GetRoleString(), FocusResults.Num());

	if (FocusResults.Num() > 0 && VC->OnVigilTargetsReady.IsBound())
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnVigilComplete: Broadcasting results."), *GetRoleString());
		// Broadcast the results
		VC->OnVigilTargetsReady.Broadcast(VC.Get(), FocusTag, FocusResults);
	}

	// Don't request next vigil if requests are still pending -- otherwise we will re-enter RequestVigil multiple times
	if (VC->TargetingRequests.Num() == 0)
	{
		// Request the next Vigil
		RequestVigil();
	}
}

void UVigilScanTask::OnPauseVigil(bool bPaused)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::OnPauseVigil);
	
	UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnPauseVigil: %s"), *GetRoleString(), bPaused ? TEXT("Paused") : TEXT("Unpaused"));
	if (bPaused)
	{
		// Cancel the current Vigil
		if (IsValid(GetWorld()))
		{
			GetWorld()->GetTimerManager().ClearTimer(VigilWaitTimer);
		}
	}
	else
	{
		// Request the next Vigil
		RequestVigil();
	}
}

void UVigilScanTask::OnRequestVigil()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::OnRequestVigil);

	UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnRequestVigil"), *GetRoleString());

	// VigilComponent ended all our targeting requests and is notifying us to continue
	if (IsValid(GetWorld()))
	{
		// Only continue if we're not already waiting to continue
		if (!GetWorld()->GetTimerManager().IsTimerActive(VigilWaitTimer))
		{
			RequestVigil();
		}
	}
}

void UVigilScanTask::OnRequestNetSync(EVigilNetSyncType SyncType)
{
	if (!IsValid(Ability) || !IsValid(GetOwnerActor()) || !VC.IsValid())
	{
		return;
	}

	// Don't net sync if the ability doesn't run on both server and client
	if (Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalOnly ||
		Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::ServerOnly)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnRequestNetSync: Invalid net execution policy."), *GetRoleString());
		return;
	}

	// End targeting requests until we sync
	VC->EndAllTargetingRequests(false);

	// Wait net sync
	UVigilNetSyncTask* WaitNetSync = UVigilNetSyncTask::WaitNetSync(Ability, SyncType);
	WaitNetSync->OnSync.BindUObject(this, &ThisClass::OnNetSync);
	WaitNetSync->ReadyForActivation();

	// Track active sync points to prevent them getting GC'd prematurely
	SyncTasks.Add(WaitNetSync);
}

void UVigilScanTask::OnNetSync(UVigilNetSyncTask* SyncTask)
{
	// Remove finished net sync
	if (IsValid(SyncTask))
	{
		SyncTasks.RemoveSingle(SyncTask);
	}

	// Notify the VC that we're done
	if (VC.IsValid() && VC->OnVigilNetSyncCompleted.IsBound())
	{
		VC->OnVigilNetSyncCompleted.Broadcast();
	}

	// Request the next Vigil
	RequestVigil();
}

void UVigilScanTask::OnDestroy(bool bInOwnerFinished)
{
	if (IsValid(GetWorld()))
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
		
		if (VC.IsValid())
		{
			if (VC->OnPauseVigil.IsBoundToObject(this))
			{
				VC->OnPauseVigil.Unbind();
			}
			if (VC->OnRequestVigil.IsBoundToObject(this))
			{
				VC->OnRequestVigil.Unbind();
			}
			if (VC->OnVigilNetSync.IsAlreadyBound(this, &ThisClass::OnRequestNetSync))
			{
				VC->OnVigilNetSync.RemoveDynamic(this, &ThisClass::OnRequestNetSync);
			}
		}
	}

	for (UVigilNetSyncTask* WaitNetSync : SyncTasks)
	{
		if (IsValid(WaitNetSync))
		{
			WaitNetSync->EndTask();
		}
	}
	
	Super::OnDestroy(bInOwnerFinished);
}

ENetMode UVigilScanTask::GetOwnerNetMode() const
{
	const AActor* const OwnerActorPtr = Ability->GetCurrentActorInfo()->OwnerActor.Get(/*bEvenIfPendingKill=*/ true);
	return OwnerActorPtr ? OwnerActorPtr->GetNetMode() : NM_MAX;
}

FString UVigilScanTask::GetRoleString() const
{
	if (GetOwnerNetMode() == NM_Standalone || GetOwnerNetMode() == NM_MAX)
	{
		return TEXT("");
	}
	return Ability->GetCurrentActorInfo()->IsNetAuthority() ? TEXT("[ Auth ]") : TEXT("[ Client ]");
}
