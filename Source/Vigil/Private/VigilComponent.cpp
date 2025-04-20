// Copyright (c) Jared Taylor. All Rights Reserved


#include "VigilComponent.h"

#include "TargetingSystem/TargetingSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilComponent)


UVigilComponent::UVigilComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// No ticking or replication, ever
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	
	SetIsReplicatedByDefault(false);
}

AActor* UVigilComponent::GetTargetingSource_Implementation() const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilComponent::GetTargetingSource);
	
	switch (DefaultTargetingSource)
	{
	case EVigilTargetingSource::Pawn: return PlayerController ? PlayerController->GetPawn() : nullptr;
	case EVigilTargetingSource::PawnIfValid:
		{
			if (PlayerController && PlayerController->GetPawn())
			{
				return PlayerController->GetPawn();
			}
			return PlayerController;
		}
	case EVigilTargetingSource::PlayerController: return PlayerController;
	}
	return nullptr;
}

TMap<FGameplayTag, UTargetingPreset*> UVigilComponent::GetTargetingPresets_Implementation() const
{
	return DefaultTargetingPresets;
}

void UVigilComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache the owning player controller
	PlayerController = Cast<APlayerController>(GetOwner());

	// Cache the preset update mode to detect changed
	bLastUpdateTargetingPresetsOnPawnChange = bUpdateTargetingPresetsOnPawnChange;

	// Get the targeting presets
	CurrentTargetingPresets = GetTargetingPresets();

	// Bind the pawn changed event if required
	UpdatePawnChangedBinding();
}

void UVigilComponent::UpdatePawnChangedBinding()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilComponent::UpdatePawnChangedBinding);

	// We call this if TargetingPresetUpdateMode changed to allow this property to be modified during runtime
	
	if (IsValid(PlayerController))
	{
		if (PlayerController->OnPossessedPawnChanged.IsAlreadyBound(this, &ThisClass::OnPawnChanged))
		{
			PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::OnPawnChanged);
		}
		
		if (bUpdateTargetingPresetsOnPawnChange || bEndTargetingRequestsOnPawnChange)
		{
			PlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPawnChanged);
		}
	}
}

void UVigilComponent::OnPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilComponent::OnPawnChanged);

	// Optionally end targeting requests
	if (bEndTargetingRequestsOnPawnChange)
	{
		EndAllTargetingRequests();
	}

	// Optionally update targeting presets
	if (bUpdateTargetingPresetsOnPawnChange)
	{
		UpdateTargetingPresets();
	}
}

void UVigilComponent::UpdateTargetingPresets()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilComponent::UpdateTargetingPresets);
	
	const TMap<FGameplayTag, UTargetingPreset*> LastTargetingPresets = CurrentTargetingPresets;
	CurrentTargetingPresets = GetTargetingPresets();

	// Clear out any targeting presets that are no longer valid
	for (const auto& Preset : LastTargetingPresets)
	{
		if (!CurrentTargetingPresets.Contains(Preset.Key))
		{
			EndTargetingRequests(Preset.Key);
		}
	}
}

void UVigilComponent::PauseVigil(bool bPaused, bool bEndTargetingRequestsOnPause)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilComponent::PauseVigil);
	
	if (bEndTargetingRequestsOnPause && bPaused)
	{
		EndAllTargetingRequests(false);
	}
	(void)OnPauseVigil.ExecuteIfBound(bPaused);
}

void UVigilComponent::EndTargetingRequests(const FGameplayTag& PresetTag, bool bNotifyVigil)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilComponent::EndTargetingRequests);
	
	if (!IsValid(GetWorld()) || !IsValid(GetWorld()->GetGameInstance()))
	{
		return;
	}

	if (UTargetingSubsystem* Targeting = GetWorld()->GetGameInstance()->GetSubsystem<UTargetingSubsystem>())
	{
		// Oddly, there is no 'end all requests' option, and the handles are not accessible, so we track the handles ourselves
		TArray<FGameplayTag> RemovedRequests;
		for (auto& Request : TargetingRequests)
		{
			// If no tag, remove them all
			if (!PresetTag.IsValid() || Request.Key == PresetTag)
			{
				RemovedRequests.Add(Request.Key);
				Targeting->RemoveAsyncTargetingRequestWithHandle(Request.Value);
			}
		}

		// If all requests were removed, clear the array
		if (RemovedRequests.Num() == TargetingRequests.Num())
		{
			TargetingRequests.Empty();
		}
		else
		{
			for (auto& RemovedRequest : RemovedRequests)
			{
				TargetingRequests.Remove(RemovedRequest);
			}
		}
	}

	// If we removed all requests, call the callback to tell Vigil to update itself
	// It won't receive any callback if there is no pending request, so we need to trigger this
	if (TargetingRequests.Num() == 0 && bNotifyVigil)
	{
		(void)OnRequestVigil.ExecuteIfBound();
	}
}

void UVigilComponent::RequestResyncVigil(EVigilNetSyncType SyncType)
{
	if (OnVigilNetSync.IsBound())
	{
		OnVigilNetSync.Broadcast(SyncType);
	}
}
