// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "VigilTags.h"
#include "VigilTypes.h"
#include "Components/ActorComponent.h"
#include "TargetingSystem/TargetingPreset.h"
#include "VigilComponent.generated.h"


/**
 * Add to your PlayerController
 * Interfaces with the passive VigilScanAbility and handles resulting data
 * Subclass this to add custom functionality
 */
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VIGIL_API UVigilComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Targeting presets used unless overriding GetTargetingPresets() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vigil)
	TMap<FGameplayTag, UTargetingPreset*> DefaultTargetingPresets = { { FVigilTags::Vigil_Focus, nullptr } };

	/**
	 * Determines which actor to use as the source for the targeting request
	 * Unless overridden in GetTargetingSource()
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vigil)
	EVigilTargetingSource DefaultTargetingSource = EVigilTargetingSource::Pawn;

	/** If true, each Vigil request will update targeting presets before proceeding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vigil)
	bool bUpdateTargetingPresetsOnUpdate = false;

	/** If true, will update targeting presets when the owning controller's possessed pawn changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vigil)
	bool bUpdateTargetingPresetsOnPawnChange = false;
	
	/** If true, any change in pawn possession will end existing targeting requests */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vigil)
	bool bEndTargetingRequestsOnPawnChange = false;

public:
	/** Track any change in preset update mode so we can rebind delegates as required */
	UPROPERTY(Transient)
	bool bLastUpdateTargetingPresetsOnPawnChange = false;

	/** Used to throttle the update rate for optimization purposes */
	UPROPERTY(Transient)
	float LastVigilScanTime = -1.f;

	/** Current targeting presets that will be used to perform targeting requests */
	UPROPERTY(Transient, DuplicateTransient)
	TMap<FGameplayTag, UTargetingPreset*> CurrentTargetingPresets;

	/** Existing targeting request handles that are in-progress */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FTargetingRequestHandle> TargetingRequests;

protected:
	/** Owning player controller */
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<APlayerController> PlayerController = nullptr;

public:
	/** Delegate called when a targeting request is completed, populated with targeting results */
	UPROPERTY(BlueprintAssignable, Category=Vigil)
	FOnVigilComplete OnVigilTargetsReady;

	/** VigilScanTask binds to this to pause itself when executed */
	FOnPauseVigil OnPauseVigil;

	/**
	 * VigilScanTask binds to this to be notified of when a vigil is requested
	 * This is a prerequisite for us to be able to end our own targeting requests
	 * Otherwise, the vigil task would not ever receive the callback and know to continue
	 */
	FOnRequestVigil OnRequestVigil;

	/** Request Vigil to re-sync */
	FOnVigilSync OnVigilNetSync;

	/** Callback for when RequestResyncVigil() completes */
	UPROPERTY(BlueprintAssignable, Category=Vigil)
	FOnVigilSyncCompleted OnVigilNetSyncCompleted;
	
public:
	UVigilComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Vigil will scan for focus options at this rate, if it can keep up
	 * The scan rate may be lower if the async targeting request has not completed
	 * This can be used to throttle the number of scans per second for performance reasons
	 * Set to 0 to disable throttling
	 */
	UFUNCTION(BlueprintNativeEvent, Category=Vigil)
	float GetMaxVigilScanRate() const;
	virtual float GetMaxVigilScanRate_Implementation() const { return 0.f; }

	/** Get the Targeting Source passed to the targeting system */
	UFUNCTION(BlueprintNativeEvent, Category=Vigil)
	AActor* GetTargetingSource() const;
	
	/** Retrieve presets used for Vigil's targeting and cache to CurrentTargetingPresets for next update */
	UFUNCTION(BlueprintNativeEvent, Category=Vigil)
	TMap<FGameplayTag, UTargetingPreset*> GetTargetingPresets() const;

public:
	virtual void BeginPlay() override;

	/** Rebind the OnPossessedPawnChanged binding if the requirement changes */
	void UpdatePawnChangedBinding();

	/** Listen for a change in possessed Pawn to optionally clear targeting requests and optionally update targeting presets */
	UFUNCTION()
	void OnPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/**
	 * Retrieves new CurrentTargetingPresets
	 * Ends any requests that are no longer current
	 */
	void UpdateTargetingPresets();

	/** Pause or resume Vigil */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	void PauseVigil(bool bPaused, bool bEndTargetingRequestsOnPause = true);
	
	/**
	 * End all targeting requests that match the PresetTag
	 * If PresetTag is empty then all targeting requests will be ended
	 * @param PresetTag The tag of the preset to end targeting requests for, or all if tag is none
	 * @param bNotifyVigil If true, will notify the VigilScanTask to end its targeting requests
	 * @warning If Vigil is not notified, it could stop running and never resume, advanced use only!
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil, meta=(AdvancedDisplay="bNotifyVigil"))
	void EndTargetingRequests(const FGameplayTag& PresetTag, bool bNotifyVigil = true);

	/**
	 * End all targeting requests
	 * @param bNotifyVigil If true, will notify the VigilScanTask to end its targeting requests
	 * @warning If Vigil is not notified, it could stop running and never resume, advanced use only!
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	void EndAllTargetingRequests(bool bNotifyVigil = true)
	{
		EndTargetingRequests(FGameplayTag::EmptyTag, bNotifyVigil);
	}

	/**
	 * Call to end all targeting requests, then perform a WaitNetSync, and finally resume targeting
	 * Because Vigil exists perpetually and runs on timers, synchronization is never guaranteed
	 * You may want to use this before anything important, and bind to OnVigilReSync to know when it's done
	 *
	 * If Vigil ability is LocalOnly then calls from this function will be ignored completely by Vigil
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	void RequestResyncVigil(EVigilNetSyncType SyncType = EVigilNetSyncType::OnlyServerWait);
};
