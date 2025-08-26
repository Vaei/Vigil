﻿// Copyright (c) Jared Taylor


#include "VigilScanTask.h"

#include "VigilComponent.h"
#include "VigilNetSyncTask.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "TargetingSystem/TargetingPreset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

#if !UE_BUILD_SHIPPING
#include "Logging/MessageLog.h"
#endif

#include "VigilStatics.h"

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

UVigilScanTask::UVigilScanTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSimulatedTask = false;
	bTickingTask = false;
}

UVigilScanTask* UVigilScanTask::VigilScan(UGameplayAbility* OwningAbility, float ErrorWaitDelay, float FailsafeDelay)
{
	UVigilScanTask* MyObj = NewAbilityTask<UVigilScanTask>(OwningAbility);
	MyObj->Delay = ErrorWaitDelay;
	MyObj->FailsafeDelay = FailsafeDelay;
	return MyObj;
}

void UVigilScanTask::Activate()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::Activate);
	
	UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::Activate"), *GetRoleString());
	
	SetWaitingOnAvatar();
	RequestVigil();
}

void UVigilScanTask::WaitForVigil(float InDelay, const TOptional<FString>& Reason, const TOptional<FString>& VeryVerboseReason)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::WaitForVigil);
	
	if (!IsValid(GetWorld()))
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::WaitForVigil: Invalid world. [SYSTEM END]"), *GetRoleString());
		return;
	}

	WaitReason = Reason;
	VeryVerboseWaitReason = VeryVerboseReason;

	GetWorld()->GetTimerManager().SetTimer(VigilWaitTimer, this, &UVigilScanTask::RequestVigil, InDelay, false);
}

void UVigilScanTask::RequestVigil()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::RequestVigil);

	// Print the last reason we waited, if set
	if (WaitReason.IsSet())
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::WaitForVigil: LastWaitReason: %s [SYSTEM RESUME]"),
			*GetRoleString(), *WaitReason.GetValue());
		WaitReason.Reset();
	}
	if (VeryVerboseWaitReason.IsSet())
	{
		UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::WaitForVigil: LastWaitReason: %s [SYSTEM RESUME]"),
			*GetRoleString(), *VeryVerboseWaitReason.GetValue());
		VeryVerboseWaitReason.Reset();
	}
	
	// Cache the VigilComponent if required
	if (!VC.IsValid())
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Trying to cache VigilComponent..."), *GetRoleString());
		
		// Get the owning controller
		const TWeakObjectPtr<AActor>& WeakOwner = Ability->GetCurrentActorInfo()->OwnerActor;
		const AController* Controller = nullptr;
		if (const APawn* Pawn = Cast<APawn>(WeakOwner.Get()))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Retrieve controller from owner pawn"), *GetRoleString());
			Controller = Pawn->GetController<AController>();
		}
		else if (const APlayerState* PlayerState = Cast<APlayerState>(WeakOwner.Get()))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Retrieve controller from owner player state"), *GetRoleString());
			Controller = PlayerState->GetOwningController();
		}
		else if (const AController* PC = Cast<AController>(WeakOwner.Get()))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Owner is a controller"), *GetRoleString());
			Controller = PC;
		}
		else
		{
			UE_LOG(LogVigil, Error, TEXT("%s VigilScanTask::RequestVigil: Could not retrieve controller because owner is not a pawn or player state or controller"), *GetRoleString());
		}
		
		// If the controller is not valid, wait for a bit and try again
		if (UNLIKELY(!Controller))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Invalid controller. [SYSTEM WAIT]"), *GetRoleString());
			WaitForVigil(Delay, {"Invalid Controller"});
			return;
		}

		// Find the VigilComponent on the controller
		VC = Controller->FindComponentByClass<UVigilComponent>();
		if (!VC.IsValid())
		{
#if !UE_BUILD_SHIPPING
			if (IsInGameThread())
			{
				FMessageLog ("PIE").Error(FText::Format(
					NSLOCTEXT("VigilScanTask", "VigilComponentNotFound", "VigilComponent not found on {0}"),
					FText::FromString(Controller->GetName())));
			}
#endif

			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Invalid VigilComponent. [SYSTEM END]"), *GetRoleString());
			
			// Vigil will not run at all
			return;
		}
		else
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Found and cached VigilComponent: %s"), *GetRoleString(), *VC->GetName());
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
		if (!VC->OnVigilSyncRequested.IsBoundToObject(this))
		{
			UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Binding to OnVigilRequestNetSync"), *GetRoleString());
			VC->OnVigilSyncRequested.BindUObject(this, &ThisClass::OnRequestNetSync);
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
			WaitForVigil(TimeLeft, {}, {"Rate Throttling"});
			return;
		}
		VC->LastVigilScanTime = GetWorld()->GetTimeSeconds();
	}

	// Check if the world and game instance are valid
	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Invalid world or game instance. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(Delay);
		return;
	}

	// Get the TargetingSubsystem
	UTargetingSubsystem* TargetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UTargetingSubsystem>();
	if (!TargetSubsystem)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Invalid TargetingSubsystem. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(Delay, {"Invalid TargetingSubsystem"});
		return;
	}

	AActor* TargetingSource = VC->GetTargetingSource();
	if (!TargetingSource)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Invalid TargetingSource. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(Delay, {"Invalid TargetingSource"});
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
	const TMap<FGameplayTag, TObjectPtr<UTargetingPreset>>& TargetingPresets = VC->CurrentTargetingPresets;
	UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::RequestVigil: TargetingPresets.Num(): %d"), *GetRoleString(), TargetingPresets.Num());

	if (TargetingPresets.Num() == 0)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: No targeting presets. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(Delay, {}, {"No TargetingPresets"});
		return;
	}

	bool bAwaitingCallback = false;
	for (const auto& Entry : TargetingPresets)
	{
		const FGameplayTag& Tag = Entry.Key;
		const UTargetingPreset* Preset = Entry.Value;

		if (!Preset || !Preset->GetTargetingTaskSet() || Preset->GetTargetingTaskSet()->Tasks.IsEmpty())
		{
			// If the only available presets only have empty tasks Vigil will never get a callback
			continue;
		}
		
		FTargetingRequestHandle& Handle = VC->TargetingRequests.FindOrAdd(Tag);
		Handle = TargetSubsystem->MakeTargetRequestHandle(Preset, FTargetingSourceContext {TargetingSource});

#if WITH_EDITOR
		// Debug the frame where the call was made vs completed
		const uint64 DebugFrame = GFrameCounter;
#endif
		
		bAwaitingCallback = true;

		// If we just net synced then perform the request sync (immediate) so the prediction window remains valid
		if (PendingNetSync == EVigilNetSyncPendingState::Pending)
		{
			TargetSubsystem->ExecuteTargetingRequestWithHandle(Handle,
				FTargetingRequestDelegate::CreateUObject(this, &ThisClass::OnVigilCompleteSync, Tag));
		}
		else
		{
			FTargetingAsyncTaskData& AsyncTaskData = FTargetingAsyncTaskData::FindOrAdd(Handle);
			AsyncTaskData.bReleaseOnCompletion = true;

			TargetSubsystem->StartAsyncTargetingRequestWithHandle(Handle,
				FTargetingRequestDelegate::CreateUObject(this, &ThisClass::OnVigilComplete, Tag));
		}
		
		UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::RequestVigil: Start async targeting for TargetingPresets[%s]: %s"), *GetRoleString(), *Tag.ToString(), *GetNameSafe(Preset));
	}

	if (!bAwaitingCallback)
	{
		// Failed to start any async targeting requests
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::RequestVigil: Failed to start async targeting requests - TargetingTaskSet(s) are empty or no Preset assigned! Bad setup! [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(Delay, {}, {"TargetingTaskSet(s) are empty! Bad setup!"});
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

void UVigilScanTask::OnVigilCompleteSync(FTargetingRequestHandle TargetingHandle, FGameplayTag FocusTag)
{
	PendingNetSync = EVigilNetSyncPendingState::Completed;
	OnVigilComplete(TargetingHandle, FocusTag);
	VC->OnNetSyncCallback();
}

void UVigilScanTask::OnVigilComplete(FTargetingRequestHandle TargetingHandle, FGameplayTag FocusTag)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::OnVigilComplete);

#if WITH_EDITOR
	// Debug the frame where the call was made vs completed
	const uint64 DebugFrame = GFrameCounter;
#endif
	
	UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::OnVigilComplete: %s"), *GetRoleString(), *FocusTag.ToString());

	if (!VC.IsValid())
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnVigilComplete: Invalid VigilComponent. [SYSTEM WAIT]"), *GetRoleString());
		WaitForVigil(Delay, {"Invalid VigilComponent"});
		return;
	}
	
	// Check if the world and game instance are valid
	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnVigilComplete: Invalid world or game instance. [SYSTEM WAIT]"), *GetRoleString());
		VC->EndAllTargetingRequests();
		WaitForVigil(Delay, {}, {"Invalid world or game instance"});
		return;
	}

	// Get the TargetingSubsystem
	const UTargetingSubsystem* TargetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UTargetingSubsystem>();
	if (!TargetSubsystem)
	{
		UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnVigilComplete: Invalid TargetingSubsystem. [SYSTEM WAIT]"), *GetRoleString());
		VC->EndAllTargetingRequests();
		WaitForVigil(Delay, {}, {"Invalid TargetingSubsystem"});
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

	// Broadcast the results
	UE_LOG(LogVigil, VeryVerbose, TEXT("%s VigilScanTask::OnVigilComplete: Broadcasting %d results."), *GetRoleString(), FocusResults.Num());
	VC->VigilTargetsReady(FocusTag, FocusResults);

	// Don't request next vigil if requests are still pending -- otherwise we will re-enter RequestVigil multiple times
	if (VC->TargetingRequests.Num() == 0)
	{
		// Request the next Vigil
		RequestVigil();
	}

	// Fail-safe timer to ensure we don't hang indefinitely -- this occurs due to an engine bug where the TargetingSubsystem
	// loses all of its requests when another player joins (so far confirmed for running under one process in PIE only)
	auto OnFailsafeTimer = [this]
	{
		if (VC->TargetingRequests.Num() > 0)
		{
			UE_LOG(LogVigil, Error, TEXT("%s VigilScanTask hung with %d targeting requests. Retrying..."), *GetRoleString(), VC->TargetingRequests.Num());
			VC->EndAllTargetingRequests();
			RequestVigil();
		}
	};
	GetWorld()->GetTimerManager().SetTimer(FailsafeTimer, OnFailsafeTimer, FailsafeDelay, false);
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
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::OnRequestNetSync);
	
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

	UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnRequestNetSync: SyncType: %s"), *GetRoleString(),
		*UVigilStatics::NetSyncToString(SyncType));

	// End targeting requests until we sync
	VC->EndAllTargetingRequests(false);

	// Wait net sync
	UVigilNetSyncTask* WaitNetSync = UVigilNetSyncTask::WaitNetSync(Ability, SyncType);
	WaitNetSync->OnSync.BindUObject(this, &ThisClass::OnNetSync);
	WaitNetSync->ReadyForActivation();

	// Track active sync points to prevent them getting GCd prematurely
	SyncTasks.Add(WaitNetSync);
}

void UVigilScanTask::OnNetSync(UVigilNetSyncTask* SyncTask)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::OnNetSync);

	UE_LOG(LogVigil, Verbose, TEXT("%s VigilScanTask::OnNetSync: SyncTask: %s"), *GetRoleString(), *GetNameSafe(SyncTask));
	
	// Remove finished net sync
	if (IsValid(SyncTask))
	{
		SyncTasks.RemoveSingle(SyncTask);
	}

	// Delay the callback until we get next targets
	PendingNetSync = EVigilNetSyncPendingState::Pending;

	// Request the next Vigil
	RequestVigil();
}

void UVigilScanTask::OnDestroy(bool bInOwnerFinished)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::OnDestroy);
	
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
			if (VC->OnVigilSyncRequested.IsBoundToObject(this))
			{
				VC->OnVigilSyncRequested.Unbind();
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
	if (!IsValid(Ability) || !Ability->GetCurrentActorInfo() || !Ability->GetCurrentActorInfo()->OwnerActor.IsValid())
	{
		return NM_MAX;
	}
	
	static constexpr bool bEvenIfPendingKill = false;
	const AActor* const OwnerActorPtr = Ability->GetCurrentActorInfo()->OwnerActor.Get(bEvenIfPendingKill);
	return OwnerActorPtr ? OwnerActorPtr->GetNetMode() : NM_MAX;
}

FString UVigilScanTask::GetRoleString() const
{
	switch (GetOwnerNetMode())
	{
	case NM_DedicatedServer:
	case NM_ListenServer: return TEXT("Auth");
	case NM_Client:
#if WITH_EDITOR
		if (Ability->GetCurrentActorInfo()->AvatarActor.IsValid())
		{
			return GetDebugStringForWorld(Ability->GetCurrentActorInfo()->AvatarActor->GetWorld());
		}
#endif
		return TEXT("Client");
	default: return TEXT("");
	}
}