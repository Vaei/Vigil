// Copyright (c) Jared Taylor


#include "Sorting/VigilSort_Angle.h"

#include "TargetingSystem/TargetingSubsystem.h"
#include "VigilStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilSort_Angle)


float UVigilSort_Angle::GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	float Score, Max;
	UVigilStatics::GetAngleToVigilTarget(TargetData.HitResult, Score, Max);
	return Score;
}
