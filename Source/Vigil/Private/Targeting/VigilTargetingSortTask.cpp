// Copyright (c) Jared Taylor


#include "Targeting/VigilTargetingSortTask.h"

#include "Targeting/VigilTargetingStatics.h"
#include "TargetingSystem/TargetingSubsystem.h"


#if UE_ENABLE_DEBUG_DRAWING
#if WITH_EDITORONLY_DATA
#include "Engine/Canvas.h"
#endif
#endif

#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilTargetingSortTask)

namespace VigilSortTaskConstants
{
	const FString PreSortPrefix = TEXT("PreSort");
	const FString PostSortPrefix = TEXT("PostSort");
}

UVigilTargetingSortTask::UVigilTargetingSortTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ScoreMethod = EVigilScoreMethod::Angle;
	bAscending = true;
	bStableSort = false;
}

float UVigilTargetingSortTask::GetAngleScore(const FTargetingDefaultResultData& TargetData,
	const FVector& SourceLocation, const FVector& TargetLocation, const FVector& SourceDirection)
{
	// We want to score this based on how far our target angle is from the source
	const float MaxAngle = TargetData.HitResult.Time;
	const FVector Direction = (TargetLocation - SourceLocation).GetSafeNormal();
	const float AngleDot = SourceDirection | Direction;
	const float AngleDiff = FMath::RadiansToDegrees(FMath::Acos(AngleDot));
	const float Score = FMath::Clamp(AngleDiff / MaxAngle, 0.f, 1.f);
	return Score;
}

float UVigilTargetingSortTask::GetDistanceScore(const FTargetingDefaultResultData& TargetData, const FVector& SourceLocation, const FVector& TargetLocation)
{
	// We want to score this based on how far our target is from the source
	const float MaxDistance = TargetData.HitResult.PenetrationDepth;
	const float Distance = FVector::Distance(SourceLocation, TargetLocation);
	const float Score = FMath::Clamp(Distance / MaxDistance, 0.f, 1.f);
	return Score;
}

float UVigilTargetingSortTask::GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	float Score = 0.f;
	float NotUsed = 0.f;

	EVigilScoreMethod Method = ScoreMethod;
	if (ScoreMethod == EVigilScoreMethod::WeightedAngleDistance)
	{
		// Skip if the value makes no sense
		if (FMath::IsNearlyEqual(AngleWeight, 1.f))
		{
			Method = EVigilScoreMethod::Angle;
		}
		else if (FMath::IsNearlyEqual(AngleWeight, 0.f))
		{
			Method = EVigilScoreMethod::Distance;
		}
		else if (FMath::IsNearlyEqual(AngleWeight, 0.5f))
		{
			Method = EVigilScoreMethod::AverageAngleDistance;
		}
	}

	switch (Method)
	{
	case EVigilScoreMethod::Angle:
		UVigilTargetingStatics::GetAngleToVigilTarget(TargetData.HitResult, Score, NotUsed);
		break;
	case EVigilScoreMethod::Distance:
		UVigilTargetingStatics::GetDistanceToVigilTarget(TargetData.HitResult, Score, NotUsed);
		break;
	case EVigilScoreMethod::AverageAngleDistance:
		{
			float DistanceScore = 0.f;
			UVigilTargetingStatics::GetDistanceToVigilTarget(TargetData.HitResult, DistanceScore, NotUsed);
			UVigilTargetingStatics::GetAngleToVigilTarget(TargetData.HitResult, Score, NotUsed);
			Score = 0.5f * (Score + DistanceScore);
		}
		break;
	case EVigilScoreMethod::WeightedAngleDistance:
		{
			float DistanceScore = 0.f;
			UVigilTargetingStatics::GetDistanceToVigilTarget(TargetData.HitResult, DistanceScore, NotUsed);
			UVigilTargetingStatics::GetAngleToVigilTarget(TargetData.HitResult, Score, NotUsed);
			Score *= AngleWeight;
			DistanceScore *= (1.f - AngleWeight);
			Score += DistanceScore;
		}
	}
	
	return Score;
}

void UVigilTargetingSortTask::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
	Super::Execute(TargetingHandle);

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

#if UE_ENABLE_DEBUG_DRAWING
	ResetSortDebugStrings(TargetingHandle);
#endif

	if (TargetingHandle.IsValid())
	{
		if (FTargetingDefaultResultsSet* ResultData = FTargetingDefaultResultsSet::Find(TargetingHandle))
		{
#if UE_ENABLE_DEBUG_DRAWING
			BuildPreSortDebugString(TargetingHandle, ResultData->TargetResults);
#endif

			const int32 NumTargets = ResultData->TargetResults.Num();

			// We get the highest score first so we can normalize the score afterwards, 
			// every task should have the same max score so none weights more than the others
			float HighestScore = 0.f;
			TArray<float> RawScores;
			for (const FTargetingDefaultResultData& TargetResult : ResultData->TargetResults)
			{
				const float RawScore = GetScoreForTarget(TargetingHandle, TargetResult);
				RawScores.Add(RawScore);
				HighestScore = FMath::Max(HighestScore, RawScore);
			}

			if(ensureMsgf(NumTargets == RawScores.Num(), TEXT("The cached raw scores should be the same size as the number of targets!")))
			{
				// Adding the normalized scores to each target result.
				for (int32 TargetIterator = 0; TargetIterator < NumTargets; ++TargetIterator)
				{
					FTargetingDefaultResultData& TargetResult = ResultData->TargetResults[TargetIterator];
					
					// Driving ascending/descending sorting based on a multiplier so it carries over to other tasks 
					const float SortingMultiplier = bAscending ? 1.f : -1.f;
					TargetResult.Score += UKismetMathLibrary::SafeDivide(RawScores[TargetIterator], HighestScore) * SortingMultiplier;
				}
			}

			auto ByScore = [](const FTargetingDefaultResultData& Lhs, const FTargetingDefaultResultData& Rhs)
			{
				return Lhs.Score < Rhs.Score;
			};

			// sort the set
			if (bStableSort)
			{
				ResultData->TargetResults.StableSort(ByScore);
			}
			else
			{
				ResultData->TargetResults.Sort(ByScore);
			}

#if UE_ENABLE_DEBUG_DRAWING
			BuildPostSortDebugString(TargetingHandle, ResultData->TargetResults);
#endif
		}
	}

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}


#if UE_ENABLE_DEBUG_DRAWING

void UVigilTargetingSortTask::DrawDebug(UTargetingSubsystem* TargetingSubsystem, FTargetingDebugInfo& Info, const FTargetingRequestHandle& TargetingHandle, float XOffset, float YOffset, int32 MinTextRowsToAdvance) const
{
#if WITH_EDITORONLY_DATA
	if (TargetingSubsystem && UTargetingSubsystem::IsTargetingDebugEnabled())
	{
		FTargetingDebugData& DebugData = FTargetingDebugData::FindOrAdd(TargetingHandle);
		const FString& PreSortScratchPadString = DebugData.DebugScratchPadStrings.FindOrAdd(VigilSortTaskConstants::PreSortPrefix + GetNameSafe(this));
		const FString& PostSortScratchPadString = DebugData.DebugScratchPadStrings.FindOrAdd(VigilSortTaskConstants::PostSortPrefix + GetNameSafe(this));
		if (!PreSortScratchPadString.IsEmpty() && !PostSortScratchPadString.IsEmpty())
		{
			if (Info.Canvas)
			{
				Info.Canvas->SetDrawColor(FColor::Yellow);
			}

			FString TaskString = FString::Printf(TEXT("Initial : %s"), *PreSortScratchPadString);
			TargetingSubsystem->DebugLine(Info, TaskString, XOffset, YOffset, MinTextRowsToAdvance);

			TaskString = FString::Printf(TEXT("Sorted : %s"), *PostSortScratchPadString);
			TargetingSubsystem->DebugLine(Info, TaskString, XOffset, YOffset, MinTextRowsToAdvance);
		}
	}
#endif // WITH_EDITORONLY_DATA
}

void UVigilTargetingSortTask::BuildPreSortDebugString(const FTargetingRequestHandle& TargetingHandle, const TArray<FTargetingDefaultResultData>& TargetResults) const
{
#if WITH_EDITORONLY_DATA
	if (UTargetingSubsystem::IsTargetingDebugEnabled())
	{
		FTargetingDebugData& DebugData = FTargetingDebugData::FindOrAdd(TargetingHandle);
		FString& PreSortScratchPadString = DebugData.DebugScratchPadStrings.FindOrAdd(VigilSortTaskConstants::PreSortPrefix + GetNameSafe(this));

		for (const FTargetingDefaultResultData& TargetData : TargetResults)
		{
			if (const AActor* Target = TargetData.HitResult.GetActor())
			{
				if (PreSortScratchPadString.IsEmpty())
				{
					PreSortScratchPadString = *GetNameSafe(Target);
				}
				else
				{
					PreSortScratchPadString += FString::Printf(TEXT(", %s"), *GetNameSafe(Target));
				}
			}
		}
	}
#endif // WITH_EDITORONLY_DATA
}

void UVigilTargetingSortTask::BuildPostSortDebugString(const FTargetingRequestHandle& TargetingHandle, const TArray<FTargetingDefaultResultData>& TargetResults) const
{
#if WITH_EDITORONLY_DATA
	if (UTargetingSubsystem::IsTargetingDebugEnabled())
	{
		FTargetingDebugData& DebugData = FTargetingDebugData::FindOrAdd(TargetingHandle);
		FString& PostSortScratchPadString = DebugData.DebugScratchPadStrings.FindOrAdd(VigilSortTaskConstants::PostSortPrefix + GetNameSafe(this));

		for (const FTargetingDefaultResultData& TargetData : TargetResults)
		{
			if (const AActor* Target = TargetData.HitResult.GetActor())
			{
				if (PostSortScratchPadString.IsEmpty())
				{
					PostSortScratchPadString = *GetNameSafe(Target);
				}
				else
				{
					PostSortScratchPadString += FString::Printf(TEXT(", %s"), *GetNameSafe(Target));
				}
			}
		}
	}
#endif // WITH_EDITORONLY_DATA
}

void UVigilTargetingSortTask::ResetSortDebugStrings(const FTargetingRequestHandle& TargetingHandle) const
{
#if WITH_EDITORONLY_DATA
	FTargetingDebugData& DebugData = FTargetingDebugData::FindOrAdd(TargetingHandle);
	FString& PreSortScratchPadString = DebugData.DebugScratchPadStrings.FindOrAdd(VigilSortTaskConstants::PreSortPrefix + GetNameSafe(this));
	PreSortScratchPadString.Reset();

	FString& PostSortScratchPadString = DebugData.DebugScratchPadStrings.FindOrAdd(VigilSortTaskConstants::PostSortPrefix + GetNameSafe(this));
	PostSortScratchPadString.Reset();
#endif // WITH_EDITORONLY_DATA
}

#endif
