// Copyright (c) Jared Taylor


#include "Sorting/VigilSort_Distance.h"

#include "TargetingSystem/TargetingSubsystem.h"
#include "VigilStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilSort_Distance)


float UVigilSort_Distance::GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilSort_ScreenDistance::GetScoreForTarget);

	float Score, Max;
	UVigilStatics::GetDistanceToVigilTarget(TargetData.HitResult, Score, Max);
	return Score;
}
