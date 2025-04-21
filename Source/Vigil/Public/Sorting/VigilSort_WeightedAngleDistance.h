// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Sorting/VigilSortBase.h"
#include "VigilSort_WeightedAngleDistance.generated.h"

/**
 * Used to sort the available targets based on their angle and distance from the source, with weighted bias
 */
UCLASS(DisplayName="Vigil Sort (Weighted Angle Distance)")
class VIGIL_API UVigilSort_WeightedAngleDistance : public UVigilSortBase
{
	GENERATED_BODY()

protected:
	/**
	 * How much weight to give to the angle's score
	 * At 0.5, it will be averaged equally with the distance score
	 * At 0.0 it will ignore angle
	 * At 1.0 it will ignore distance
	 */
	UPROPERTY(EditAnywhere, Category="Vigil Sorting", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0", Delta="0.05", ForceUnits="Percent"))
	float AngleWeight = 0.75f;
	
protected:
	/** Called on every target to get a Score for sorting. This score will be added to the Score float in FTargetingDefaultResultData */
	virtual float GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
		const FTargetingDefaultResultData& TargetData) const override;
};
