// Copyright (c) Jared Taylor


#include "Sorting/VigilSort_WeightedAngleDistance.h"

#include "TargetingSystem/TargetingSubsystem.h"
#include "VigilStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilSort_WeightedAngleDistance)


float UVigilSort_WeightedAngleDistance::GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	float Dist, Angle, Max;
	UVigilStatics::GetDistanceToVigilTarget(TargetData.HitResult, Dist, Max);
	UVigilStatics::GetAngleToVigilTarget(TargetData.HitResult, Angle, Max);
	const float AngleScore = Angle * AngleWeight;
	const float DistanceScore = Dist * (1.f - AngleWeight);
	return AngleScore + DistanceScore;
}
