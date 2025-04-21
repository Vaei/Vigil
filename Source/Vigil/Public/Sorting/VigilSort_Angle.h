// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Sorting/VigilSortBase.h"
#include "VigilSort_Angle.generated.h"

/**
 * Used to sort the available targets based on their angle difference from the source
 */
UCLASS(DisplayName="Vigil Sort (Angle)")
class VIGIL_API UVigilSort_Angle : public UVigilSortBase
{
	GENERATED_BODY()

protected:
	/** Called on every target to get a Score for sorting. This score will be added to the Score float in FTargetingDefaultResultData */
	virtual float GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
		const FTargetingDefaultResultData& TargetData) const override;
};
