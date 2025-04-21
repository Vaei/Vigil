// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingTask.h"
#include "VigilSortBase.generated.h"

/**
 * Used to sort the available targets based on our criteria
 */
UCLASS(Abstract)
class VIGIL_API UVigilSortBase : public UTargetingTask
{
	GENERATED_BODY()

protected:
	/** Do we increment (ascending) or decrement the score to find the best option */
	UPROPERTY(EditAnywhere, Category="Vigil Sorting")
	uint8 bAscending : 1;

	/** Should this task use a (slightly slower) sorting algorithm that preserves the relative ordering of targets with equal scores? */
	UPROPERTY(EditAnywhere, Category="Vigil Sorting")
	uint8 bStableSort : 1;

public:
	UVigilSortBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** Called on every target to get a Score for sorting. This score will be added to the Score float in FTargetingDefaultResultData */
	UFUNCTION(BlueprintNativeEvent, Category="Vigil Sorting")
	float GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const;
	
	virtual float GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
		const FTargetingDefaultResultData& TargetData) const { return 0.f; }

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
