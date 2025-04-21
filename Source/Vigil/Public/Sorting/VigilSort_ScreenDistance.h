// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Sorting/VigilSortBase.h"
#include "VigilSort_ScreenDistance.generated.h"

UENUM(BlueprintType)
enum class EVigilScreenDistanceLocationSource : uint8
{
	HitComponent,
	HitActor,
	HitLocation,
};

/**
 * Used to sort the available targets based on their distance from the center of the screen/viewport
 * LOCAL player only -- not available to dedicated servers or simulated proxies
 */
UCLASS(DisplayName="Vigil Sort (Screen Distance)")
class VIGIL_API UVigilSort_ScreenDistance : public UVigilSortBase
{
	GENERATED_BODY()

protected:
	/** What world location to project to screen space, from which we compare with the screen center */
	UPROPERTY(EditAnywhere, Category="Vigil Sorting")
	EVigilScreenDistanceLocationSource LocationSource = EVigilScreenDistanceLocationSource::HitComponent;
	
protected:
	/** Called on every target to get a Score for sorting. This score will be added to the Score float in FTargetingDefaultResultData */
	virtual float GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
		const FTargetingDefaultResultData& TargetData) const override;
};
