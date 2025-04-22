// Copyright (c) Jared Taylor


#include "VigilComponent.h"

#include "TargetingSystem/TargetingSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

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

FVigilFocusResult UVigilComponent::GetFocusResult(FGameplayTag FocusTag, bool& bValid) const
{
	if (const FVigilFocusResult* Result = CurrentFocusResults.Find(FocusTag))
	{
		bValid = Result->HitResult.GetActor() != nullptr;
		return *Result;
	}
	bValid = false;
	return {};
}

AActor* UVigilComponent::GetFocusActor(FGameplayTag FocusTag) const
{
	if (const FVigilFocusResult* Result = CurrentFocusResults.Find(FocusTag))
	{
		return Result->HitResult.GetActor();
	}
	return nullptr;
}

void UVigilComponent::VigilTargetsReady(const FGameplayTag& FocusTag, const TArray<FVigilFocusResult>& Results)
{
	// Update our current focus results
	FVigilFocusResult& Focus = CurrentFocusResults.FindOrAdd(FocusTag);
	AActor* LastFocusActor = Focus.HitResult.GetActor();
	Focus = Results.IsValidIndex(0) ? Results[0] : FVigilFocusResult();

	if (LastFocusActor != Focus.HitResult.GetActor())
	{
		// Notify if the focus actor changed
		if (OnVigilFocusChanged.IsBound())
		{
			OnVigilFocusChanged.Broadcast(
				this, FocusTag, Focus.HitResult.GetActor(), LastFocusActor, Focus);
		}
		K2_VigilFocusChanged(FocusTag, Focus.HitResult.GetActor(), LastFocusActor, Focus);
	}

	// Notify any listeners that the targets are ready
	if (OnVigilTargetsReady.IsBound())
	{
		OnVigilTargetsReady.Broadcast(this, FocusTag, Results);
	}
	K2_VigilTargetsReady(FocusTag, Results);
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

	if (UTargetingSubsystem* TargetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UTargetingSubsystem>())
	{
		// Oddly, there is no 'end all requests' option, and the handles are not accessible, so we track the handles ourselves
		TArray<FGameplayTag> RemovedRequests;
		for (auto& Request : TargetingRequests)
		{
			// If no tag, remove them all
			if (!PresetTag.IsValid() || Request.Key == PresetTag)
			{
				RemovedRequests.Add(Request.Key);
				TargetSubsystem->RemoveAsyncTargetingRequestWithHandle(Request.Value);
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
