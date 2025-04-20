// Copyright (c) Jared Taylor. All Rights Reserved


#include "VigilScanTask.h"

#include "VigilComponent.h"
#include "TargetingSystem/TargetingSubsystem.h"

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
	
	UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::Activate"));
	
	SetWaitingOnAvatar();
	RequestVigil();
}

void UVigilScanTask::WaitForVigil(float Delay)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilScanTask::WaitForVigil);
	
	if (!IsValid(GetWorld()))
	{
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::WaitForVigil: Invalid world. [SYSTEM END]"));
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
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::RequestVigil: Caching VigilComponent"));
		
		// Get the owning player controller
		const TWeakObjectPtr<AActor>& WeakOwner = Ability->GetCurrentActorInfo()->OwnerActor;
		const APlayerController* PlayerController = WeakOwner.IsValid() ? Cast<APlayerController>(WeakOwner) : nullptr;

		// If the player controller is not valid, wait for a bit and try again
		if (UNLIKELY(!PlayerController))
		{
			UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::RequestVigil: Invalid player controller. [SYSTEM WAIT]"));
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
					FText::FromString(WeakOwner->GetName())));
			}
#endif

			UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::RequestVigil: Invalid VigilComponent. [SYSTEM END]"));
			
			// Vigil will not run at all
			return;
		}

		// Bind to the pause delegate
		if (!VC->OnPauseVigil.IsBoundToObject(this))
		{
			UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::RequestVigil: Binding to OnPauseVigil"));
			VC->OnPauseVigil.BindUObject(this, &ThisClass::OnPauseVigil);
		}

		// Bind to request delegate
		if (!VC->OnRequestVigil.IsBoundToObject(this))
		{
			UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::RequestVigil: Binding to OnRequestVigil"));
			VC->OnRequestVigil.BindUObject(this, &ThisClass::OnRequestVigil);
		}
	}

	check(VC.IsValid());

	// Are we on cooldown due to rate throttling?
	const float MaxRate = VC->GetMaxVigilScanRate();
	UE_LOG(LogVigil, VeryVerbose, TEXT("VigilScanTask::RequestVigil: MaxRate: %.2f"), MaxRate);
	if (MaxRate > 0.f)
	{
		// (Thinking out loud...)
		// If updated at 10.0, rate is 0.1, current time is 10.05
		// TimeSince is 0.05, TimeSince < MaxRate == 0.05 < 0.1 == true
		// TimeLeft = MaxRate - TimeSince == 0.1 - 0.05 == 0.05
		
		const float TimeSince = GetWorld()->TimeSince(VC->LastVigilScanTime);
		UE_LOG(LogVigil, VeryVerbose, TEXT("VigilScanTask::RequestVigil: TimeSince: %.2f"), TimeSince);
		if (TimeSince < MaxRate)
		{
			const float TimeLeft = MaxRate - TimeSince;
			UE_LOG(LogVigil, VeryVerbose, TEXT("VigilScanTask::RequestVigil: TimeLeft: %.2f [SYSTEM WAIT]"), TimeLeft);
			WaitForVigil(TimeLeft);
			return;
		}
	}

	// Check if the world and game instance are valid
	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::RequestVigil: Invalid world or game instance. [SYSTEM WAIT]"));
		WaitForVigil(0.5f);
		return;
	}

	// Get the TargetingSubsystem
	UTargetingSubsystem* TargetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UTargetingSubsystem>();
	if (!TargetSubsystem)
	{
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::RequestVigil: Invalid TargetingSubsystem. [SYSTEM WAIT]"));
		WaitForVigil(0.5f);
		return;
	}

	AActor* TargetingSource = VC->GetTargetingSource();
	if (!TargetingSource)
	{
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::RequestVigil: Invalid TargetingSource. [SYSTEM WAIT]"));
		WaitForVigil(0.5f);
		return;
	}
	
	// Check for changes to the targeting preset update mode
	if (VC->TargetingPresetUpdateMode != VC->LastTargetingPresetUpdateMode)
	{
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::RequestVigil: TargetingPresetUpdateMode changed."));
		// Remove or bind the pawn changed binding
		VC->UpdatePawnChangedBinding();
		VC->LastTargetingPresetUpdateMode = VC->TargetingPresetUpdateMode;
	}

	// Optionally update the targeting presets
	if (VC->TargetingPresetUpdateMode == EVigilTargetPresetUpdateMode::OnUpdate)
	{
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::RequestVigil: Updating targeting presets."));
		VC->UpdateTargetingPresets();
	}

	// Get cached targeting presets
	const TMap<FGameplayTag, UTargetingPreset*>& TargetingPresets = VC->CurrentTargetingPresets;
	UE_LOG(LogVigil, VeryVerbose, TEXT("VigilScanTask::RequestVigil: TargetingPresets.Num(): %d"), TargetingPresets.Num());
	for (const auto& Entry : TargetingPresets)
	{
		const FGameplayTag& Tag = Entry.Key;
		const UTargetingPreset* Preset = Entry.Value;

		FTargetingRequestHandle& Handle = VC->TargetingRequests.FindOrAdd(Tag);
		Handle = TargetSubsystem->MakeTargetRequestHandle(Preset, FTargetingSourceContext {TargetingSource});

		FTargetingAsyncTaskData& AsyncTaskData = FTargetingAsyncTaskData::FindOrAdd(Handle);
		AsyncTaskData.bReleaseOnCompletion = true;

		TargetSubsystem->StartAsyncTargetingRequestWithHandle(Handle,
			FTargetingRequestDelegate::CreateUObject(this, &ThisClass::OnVigilComplete, Tag));

		UE_LOG(LogVigil, VeryVerbose, TEXT("VigilScanTask::RequestVigil: Start async targeting for TargetingPresets[%s]: %s"), *Tag.ToString(), *GetNameSafe(Preset));
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
	
	UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::OnVigilComplete: %s"), *FocusTag.ToString());

	if (!VC.IsValid())
	{
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::OnVigilComplete: Invalid VigilComponent. [SYSTEM WAIT]"));
		WaitForVigil(0.5f);
		return;
	}
	
	// Check if the world and game instance are valid
	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::OnVigilComplete: Invalid world or game instance. [SYSTEM WAIT]"));
		VC->EndAllTargetingRequests();
		WaitForVigil(0.5f);
		return;
	}

	// Get the TargetingSubsystem
	const UTargetingSubsystem* TargetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UTargetingSubsystem>();
	if (!TargetSubsystem)
	{
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::OnVigilComplete: Invalid TargetingSubsystem. [SYSTEM WAIT]"));
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

	UE_LOG(LogVigil, VeryVerbose, TEXT("VigilScanTask::OnVigilComplete: FocusResults.Num(): %d"), FocusResults.Num());

	if (FocusResults.Num() > 0 && VC->OnVigilComplete.IsBound())
	{
		UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::OnVigilComplete: Broadcasting results."));
		// Broadcast the results
		VC->OnVigilComplete.Broadcast(VC.Get(), FocusTag, FocusResults);
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
	
	UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::OnPauseVigil: %s"), bPaused ? TEXT("Paused") : TEXT("Unpaused"));
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

	UE_LOG(LogVigil, Verbose, TEXT("VigilScanTask::OnRequestVigil"));

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
		}
	}
	
	Super::OnDestroy(bInOwnerFinished);
}
