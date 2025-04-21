// Copyright (c) Jared Taylor


#include "Sorting/VigilSort_Distance.h"

#include "TargetingSystem/TargetingSubsystem.h"
#include "Targeting/VigilTargetingStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilSort_Distance)


float UVigilSort_Distance::GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	float Score, Max;
	UVigilTargetingStatics::GetDistanceToVigilTarget(TargetData.HitResult, Score, Max);
	return Score;
}
