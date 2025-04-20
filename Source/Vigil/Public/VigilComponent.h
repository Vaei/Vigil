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
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vigil)
	bool bEndTargetingRequestsOnPawnChange = false;
	
	/** Determines when we call GetTargetingPresets() to update the cached presets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Vigil)
	EVigilTargetPresetUpdateMode TargetingPresetUpdateMode = EVigilTargetPresetUpdateMode::BeginPlay;

public:
	UPROPERTY(Transient)
	EVigilTargetPresetUpdateMode LastTargetingPresetUpdateMode = EVigilTargetPresetUpdateMode::BeginPlay;

	UPROPERTY(Transient)
	float LastVigilScanTime = -1.f;

	UPROPERTY(Transient, DuplicateTransient)
	TMap<FGameplayTag, UTargetingPreset*> CurrentTargetingPresets;
	
	UPROPERTY(Transient)
	TMap<FGameplayTag, FTargetingRequestHandle> TargetingRequests;

protected:
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<APlayerController> PlayerController = nullptr;

public:
	/** Delegate called when a targeting request is completed, populated with targeting results */
	UPROPERTY(BlueprintAssignable, Category=Vigil)
	FOnVigilComplete OnVigilComplete;

	/** VigilScanTask binds to this to pause itself when executed */
	FOnPauseVigil OnPauseVigil;

	/**
	 * VigilScanTask binds to this to be notified of when a vigil is requested
	 * This is a prerequisite for us to be able to end our own targeting requests
	 * Otherwise, the vigil task would not ever receive the callback and know to continue
	 */
	FOnRequestVigil OnRequestVigil;
	
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

	void UpdatePawnChangedBinding();
	
	UFUNCTION()
	void OnPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void UpdateTargetingPresets();

	UFUNCTION(BlueprintCallable, Category=Vigil)
	void PauseVigil(bool bPaused, bool bEndTargetingRequestsOnPause = true);
	
	/**
	 * End all targeting requests that match the PresetTag
	 * If PresetTag is empty then all targeting requests will be ended
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	void EndTargetingRequests(const FGameplayTag& PresetTag);

	/** End all targeting requests */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	void EndAllTargetingRequests()
	{
		EndTargetingRequests(FGameplayTag::EmptyTag);
	}
};
