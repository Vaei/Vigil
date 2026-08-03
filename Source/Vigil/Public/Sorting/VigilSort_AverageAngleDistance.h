// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "VigilTypes.h"
#include "Sorting/VigilSortBase.h"
#include "VigilSort_AverageAngleDistance.generated.h"

/**
 * Used to sort the available targets based on their average angle and distance from the source
 */
UCLASS(DisplayName="Vigil Sort (Average Angle Distance)")
class VIGIL_API UVigilSort_AverageAngleDistance : public UVigilSortBase
{
	GENERATED_BODY()

protected:
	/** Whether distance is measured in world space, or from the center of the screen */
	UPROPERTY(EditAnywhere, Category="Vigil Sorting")
	EVigilDistanceMode DistanceMode = EVigilDistanceMode::WorldDistance;

	/** What world location to project to screen space, from which we compare with the screen center */
	UPROPERTY(EditAnywhere, Category="Vigil Sorting", meta=(EditCondition="DistanceMode == EVigilDistanceMode::ScreenDistance", EditConditionHides))
	EVigilScreenDistanceLocationSource LocationSource = EVigilScreenDistanceLocationSource::HitComponent;

protected:
	/** Called on every target to get a Score for sorting. This score will be added to the Score float in FTargetingDefaultResultData */
	virtual float GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
		const FTargetingDefaultResultData& TargetData) const override;
};
