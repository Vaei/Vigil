// Copyright (c) Jared Taylor


#include "Sorting/VigilSort_ScreenDistance.h"

#include "TargetingSystem/TargetingSubsystem.h"
#include "Targeting/VigilTargetingStatics.h"
#include "VigilStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilSort_ScreenDistance)


float UVigilSort_ScreenDistance::GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	if (IsRunningDedicatedServer())
	{
		return 0.f;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(VigilSort_ScreenDistance::GetScoreForTarget);

	const UTargetingSubsystem* TargetSubsystem = GetTargetingSubsystem(TargetingHandle);
	if (!TargetSubsystem)
	{
		return 0.f;
	}

	float NormalizedDist, MaxDist;
	return UVigilStatics::GetScreenDistanceToVigilTarget(TargetSubsystem, TargetData.HitResult, LocationSource,
		NormalizedDist, MaxDist);
}
