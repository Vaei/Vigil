// Copyright (c) Jared Taylor


#include "Sorting/VigilSort_AverageAngleDistance.h"

#include "TargetingSystem/TargetingSubsystem.h"
#include "Targeting/VigilTargetingStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilSort_AverageAngleDistance)


float UVigilSort_AverageAngleDistance::GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	float Dist, Angle, Max;
	UVigilTargetingStatics::GetDistanceToVigilTarget(TargetData.HitResult, Dist, Max);
	UVigilTargetingStatics::GetAngleToVigilTarget(TargetData.HitResult, Angle, Max);
	return 0.5f * (Dist + Angle);
}
