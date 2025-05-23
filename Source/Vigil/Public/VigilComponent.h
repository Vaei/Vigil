﻿// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "VigilTags.h"
#include "VigilTypes.h"
#include "Components/ActorComponent.h"
#include "TargetingSystem/TargetingPreset.h"
#include "UObject/ObjectKey.h"
#include "VigilComponent.generated.h"

class AController;

/**
 * Add to your Controller
 * Interfaces with the passive VigilScanAbility and handles resulting data
 * Subclass this to add custom functionality
 */
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VIGIL_API UVigilComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Targeting presets used unless overriding GetTargetingPresets() */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Vigil)
	TMap<FGameplayTag, UTargetingPreset*> DefaultTargetingPresets = { { FVigilTags::Vigil_Focus, nullptr } };

	/**
	 * Determines which actor to use as the source for the targeting request
	 * Unless overridden in GetTargetingSource()
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Vigil)
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
	/** Owning controller */
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<AController> Controller = nullptr;

public:
	/** Delegate called when a targeting request is completed, populated with targeting results */
	UPROPERTY(BlueprintAssignable, Category=Vigil)
	FOnVigilTargetsReady OnVigilTargetsReady;

	/** Delegate called when a focus target changes */
	UPROPERTY(BlueprintAssignable, Category=Vigil)
	FOnVigilFocusChanged OnVigilFocusChanged;
	
	/** VigilScanTask binds to this to pause itself when executed */
	FOnPauseVigil OnPauseVigil;

	/**
	 * VigilScanTask binds to this to be notified of when a vigil is requested
	 * This is a prerequisite for us to be able to end our own targeting requests
	 * Otherwise, the vigil task would not ever receive the callback and know to continue
	 */
	FOnRequestVigil OnRequestVigil;

	/**
	 * VigilScanTask binds to this to be notified of when wait net sync is requested
	 */
	FOnVigilSyncRequested OnVigilSyncRequested;

protected:
	/** Objects that were registered as awaiting net sync callback */
	TMap<FObjectKey, FVigilNetSyncDelegateData> NetSyncDelegateMap;
	
protected:
	/** Last results of Vigil Focusing update, these are the current focus targets */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=Vigil)
	TMap<FGameplayTag, FVigilFocusResult> CurrentFocusResults;
	
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

	/** Get the current focus result for the given focus tag */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	FVigilFocusResult GetFocusResult(FGameplayTag FocusTag, bool& bValid) const;

	/** Get the current focus actor for the given focus tag */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	AActor* GetFocusActor(FGameplayTag FocusTag) const;

	/**
	 * Notified by UVigilScanTask that our targets are ready
	 * Cache the results and notify any listeners
	 */
	void VigilTargetsReady(const FGameplayTag& FocusTag, const TArray<FVigilFocusResult>& Results);

	UFUNCTION(BlueprintImplementableEvent, Category=Vigil, meta=(DisplayName="On Vigil Targets Ready"))
	void K2_VigilTargetsReady(const FGameplayTag& FocusTag, const TArray<FVigilFocusResult>& Results);

	UFUNCTION(BlueprintImplementableEvent, Category=Vigil, meta=(DisplayName="On Vigil Focus Changed"))
	void K2_VigilFocusChanged(const FGameplayTag& FocusTag, AActor* Focus, AActor* LastFocus, const FVigilFocusResult& Result);
	
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
	 * Make a request for Vigil to run a WaitNetSync
	 * Vigil will clear all targeting requests, run the WaitNetSync, then perform an immediate synchronous request to
	 * maintain the prediction window. When you receive the callback, the targets will be ready, if available.
	 *
	 * Use this from a LocalPredicted ability prior to running your ability logic when predicted focus is required.
	 * 
	 * If Vigil ability is LocalOnly then calls from this function will be ignored completely by Vigil and you will receive no callback.
	 */
	bool RequestVigilNetSync(UObject* Caller, FOnVigilNetSyncCompleted Delegate,
		EVigilNetSyncType SyncType = EVigilNetSyncType::OnlyServerWait);

	/**
	 * Make a request for Vigil to run a WaitNetSync
	 * Vigil will clear all targeting requests, run the WaitNetSync, then perform an immediate synchronous request to
	 * maintain the prediction window. When you receive the callback, the targets will be ready, if available.
	 *
	 * Use this from a LocalPredicted ability prior to running your ability logic when predicted focus is required.
	 * 
	 * If Vigil ability is LocalOnly then calls from this function will be ignored completely by Vigil and you will receive no callback.
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil, meta=(DefaultToSelf="Caller", HidePin="Caller", AdvancedDisplay="SyncType", DisplayName="Request Vigil Wait Net Sync", Keywords="wait net sync"))
	bool K2_RequestVigilNetSync(UObject* Caller, FOnVigilNetSyncCompletedBP Delegate,
		EVigilNetSyncType SyncType = EVigilNetSyncType::OnlyServerWait);

	/** Vigil Scan Task calls this when it completes the net sync and has performed a synchronous targeting update */
	void OnNetSyncCallback();

protected:
	FString GetRoleString() const;
};
