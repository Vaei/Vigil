// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingTask.h"
#include "VigilTargetingTypes.h"
#include "VigilTargetingSortTask.generated.h"

/**
 * Used to sort the available targets based on our criteria
 */
UCLASS()
class VIGIL_API UVigilTargetingSortTask : public UTargetingTask
{
	GENERATED_BODY()

protected:
	/** How to score the targets */
	UPROPERTY(EditAnywhere, Category="Vigil Sorting")
	EVigilScoreMethod ScoreMethod;

	/**
	 * How much weight to give to the angle's score
	 * At 0.5, it will be averaged equally with the distance score
	 * At 0.0 it will ignore angle
	 * At 1.0 it will ignore distance
	 */
	UPROPERTY(EditAnywhere, Category="Vigil Sorting", meta=(EditCondition="ScoreMethod==EVigilScoreMethod::WeightedAngleDistance", EditConditionHides, ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0", Delta="0.05", ForceUnits="Percent"))
	float AngleWeight = 0.75f;
	
	/** Do we increment (ascending) or decrement the score to find the best option */
	UPROPERTY(EditAnywhere, Category = "Vigil Sorting")
	uint8 bAscending : 1;

	/** Should this task use a (slightly slower) sorting algorithm that preserves the relative ordering of targets with equal scores? */
	UPROPERTY(EditAnywhere, Category = "Vigil Sorting")
	uint8 bStableSort : 1;

public:
	UVigilTargetingSortTask(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	static float GetAngleScore(const FTargetingDefaultResultData& TargetData, const FVector& SourceLocation,
		const FVector& TargetLocation, const FVector& SourceDirection);
	
	static float GetDistanceScore(const FTargetingDefaultResultData& TargetData, const FVector& SourceLocation,
		const FVector& TargetLocation);

protected:
	/** Called on every target to get a Score for sorting. This score will be added to the Score float in FTargetingDefaultResultData */
	virtual float GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle,
		const FTargetingDefaultResultData& TargetData) const;

	/** Evaluation function called by derived classes to process the targeting request */
	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;

	/** Debug Helper Methods */
#if ENABLE_DRAW_DEBUG
private:
	virtual void DrawDebug(UTargetingSubsystem* TargetingSubsystem, FTargetingDebugInfo& Info, const FTargetingRequestHandle& TargetingHandle, float XOffset, float YOffset, int32 MinTextRowsToAdvance) const override;
	void BuildPreSortDebugString(const FTargetingRequestHandle& TargetingHandle, const TArray<FTargetingDefaultResultData>& TargetResults) const;
	void BuildPostSortDebugString(const FTargetingRequestHandle& TargetingHandle, const TArray<FTargetingDefaultResultData>& TargetResults) const;
	void ResetSortDebugStrings(const FTargetingRequestHandle& TargetingHandle) const;
#endif // ENABLE_DRAW_DEBUG
	/** ~Debug Helper Methods */
};
