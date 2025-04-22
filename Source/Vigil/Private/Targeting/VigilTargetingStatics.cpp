// Copyright (c) Jared Taylor


#include "Targeting/VigilTargetingStatics.h"

#include "VigilTypes.h"
#include "GameFramework/HUD.h"
#include "Targeting/VigilTargetingTypes.h"
#include "Types/TargetingSystemTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/PrimitiveComponent.h"

#if ENABLE_VISUAL_LOG
#include "VisualLogger/VisualLogger.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilTargetingStatics)

FVector UVigilTargetingStatics::GetSourceLocation(const FTargetingRequestHandle& TargetingHandle,
	EVigilTargetLocationSource LocationSource)
{
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (SourceContext->SourceActor)
		{
			switch (LocationSource)
			{
			case EVigilTargetLocationSource::Actor: return SourceContext->SourceActor->GetActorLocation();
			case EVigilTargetLocationSource::ViewLocation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetPawnViewLocation();
					}
				}
			case EVigilTargetLocationSource::Camera:
				{
					if (const APlayerController* PC = Cast<APlayerController>(SourceContext->SourceActor->GetOwner()))
					{
						if (PC->PlayerCameraManager)
						{
							return PC->PlayerCameraManager->GetCameraLocation();
						}
					}
				}
				break;
			}
		}
	}
	return FVector::ZeroVector;
}

FVector UVigilTargetingStatics::GetSourceOffset(const FTargetingRequestHandle& TargetingHandle,
	EVigilTargetLocationSource LocationSource, FVector DefaultSourceLocationOffset, bool bUseRelativeLocationOffset)
{
	if (!bUseRelativeLocationOffset)
	{
		return DefaultSourceLocationOffset;
	}

	if (DefaultSourceLocationOffset.IsZero())
	{
		return FVector::ZeroVector;
	}
	
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (SourceContext->SourceActor)
		{
			switch (LocationSource)
			{
			case EVigilTargetLocationSource::Actor: return SourceContext->SourceActor->GetActorRotation().RotateVector(DefaultSourceLocationOffset);
			case EVigilTargetLocationSource::ViewLocation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetViewRotation().RotateVector(DefaultSourceLocationOffset);
					}
				}
			case EVigilTargetLocationSource::Camera:
				{
					if (const APlayerController* PC = Cast<APlayerController>(SourceContext->SourceActor->GetOwner()))
					{
						if (PC->PlayerCameraManager)
						{
							return PC->PlayerCameraManager->GetCameraRotation().RotateVector(DefaultSourceLocationOffset);
						}
					}
				}
				break;
			}
		}
	}
	return FVector::ZeroVector;
}

FQuat UVigilTargetingStatics::GetSourceRotation(const FTargetingRequestHandle& TargetingHandle, EVigilTargetRotationSource RotationSource)
{
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (SourceContext->SourceActor)
		{
			switch (RotationSource)
			{
			case EVigilTargetRotationSource::Actor: return SourceContext->SourceActor->GetActorQuat();
			case EVigilTargetRotationSource::ControlRotation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetControlRotation().Quaternion();
					}
				}
			case EVigilTargetRotationSource::ViewRotation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetViewRotation().Quaternion();
					}
				}
				break;
			}
		}
	}
	return FQuat::Identity;
}

void UVigilTargetingStatics::InitCollisionParams(const FTargetingRequestHandle& TargetingHandle,
	FCollisionQueryParams& OutParams, bool bIgnoreSourceActor, bool bIgnoreInstigatorActor, bool bTraceComplex)
{
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (bIgnoreSourceActor && SourceContext->SourceActor)
		{
			OutParams.AddIgnoredActor(SourceContext->SourceActor);
		}

		if (bIgnoreInstigatorActor && SourceContext->InstigatorActor)
		{
			OutParams.AddIgnoredActor(SourceContext->InstigatorActor);
		}
	}

	OutParams.bTraceComplex = bTraceComplex;
	
	// This isn't exported, we can't get to it...
	
	// // If we have a data store to override the collision query params from, add that to our collision params
	// if (const FCollisionQueryTaskData* FoundOverride = UE::TargetingSystem::TTargetingDataStore<FCollisionQueryTaskData>::Find(TargetingHandle))
	// {
	// 	OutParams.AddIgnoredActors(FoundOverride->IgnoredActors);
	// }

	// Vigil is its own thing, lets not use the CVar
	
	// static const auto CVarComplexTracingAOE = IConsoleManager::Get().FindConsoleVariable(TEXT("ts.AOE.EnableComplexTracingAOE"));
	// const bool bComplexTracingAOE = CVarComplexTracingAOE ? CVarComplexTracingAOE->GetBool() : true;
}

float UVigilTargetingStatics::GetDistanceToVigilTarget(const FHitResult& HitResult, float& NormalizedDistance, float& MaxDistance)
{
	// This is the location we traced from to find a target
	const FVector SourceLocation = HitResult.TraceStart;

	// This is the location where we found the target
	const FVector TargetLocation = HitResult.ImpactPoint;
	
	MaxDistance = HitResult.PenetrationDepth;
	const float Distance = FVector::Distance(SourceLocation, TargetLocation);
	NormalizedDistance = FMath::Clamp(Distance / MaxDistance, 0.f, 1.f);
	return Distance;
}

float UVigilTargetingStatics::GetAngleToVigilTarget(const FHitResult& HitResult, float& NormalizedAngle, float& MaxAngle)
{
	// This is the location we traced from to find a target
	const FVector SourceLocation = HitResult.TraceStart;

	// This is the location where we found the target
	const FVector TargetLocation = HitResult.ImpactPoint;
	
	// This is the direction we traced in to find a target
	const FVector SourceDirection = HitResult.Normal;
	
	MaxAngle = HitResult.Time;
	const FVector Direction = (TargetLocation - SourceLocation).GetSafeNormal();
	const float AngleDot = SourceDirection | Direction;
	const float AngleDiff = FMath::RadiansToDegrees(FMath::Acos(AngleDot));
	NormalizedAngle = FMath::Clamp(AngleDiff / MaxAngle, 0.f, 1.f);
	return AngleDiff;
}

void UVigilTargetingStatics::VigilDrawDebugResults(APlayerController* PC, const FGameplayTag& FocusTag,
	const TArray<FVigilFocusResult>& FocusResults, float DrawDuration, bool bLocatorAngle, bool bLocatorDistance)
{
#if UE_ENABLE_DEBUG_DRAWING
	if (!IsValid(PC) || !PC->GetHUD() || !PC->GetWorld())
	{
		return;
	}

	VigilDrawDebugResults_Internal(PC, FocusTag, FocusResults,
		[](const UWorld* World, const FVector& Location, const FString& Info, const FColor& Color, float Duration)
		{
			DrawDebugString(World, Location, Info, nullptr, Color, Duration, true, 1.f);
		},
		[](const UWorld* World, const FMatrix& Matrix, float Radius, const FColor& Color, float Duration)
		{
			DrawDebugCircle(World, Matrix, Radius, 32, Color, false, Duration, 10, 2.f);
		},
		bLocatorAngle, bLocatorDistance, DrawDuration);
#endif
}

void UVigilTargetingStatics::VigilAddVisualLoggerResults(APlayerController* PC, const FGameplayTag& FocusTag,
	const TArray<FVigilFocusResult>& FocusResults, bool bLocatorAngle, bool bLocatorDistance)
{
#if ENABLE_VISUAL_LOG
	if (!IsValid(PC) || !PC->GetWorld())
	{
		return;
	}

	VigilDrawDebugResults_Internal(PC, FocusTag, FocusResults,
	[PC](const UWorld* World, const FVector& Location, const FString& Info, const FColor& Color, float Duration)
	{
		UE_VLOG_LOCATION(PC, LogVigil, Log, Location, 2.f, Color, TEXT("%s"), *Info);
	},
	[PC](const UWorld* World, const FMatrix& Matrix, float Radius, const FColor& Color, float Duration)
	{
		UE_VLOG_CIRCLE_THICK(PC, LogVigil, Log, Matrix.GetOrigin(), Matrix.GetScaledAxis(EAxis::X), Radius * 1.25f, Color, 4.f, TEXT(""));
	},
	bLocatorAngle, bLocatorDistance, 0.f);
#endif
}

void UVigilTargetingStatics::VigilDrawDebugResults_Internal(APlayerController* PC, const FGameplayTag& FocusTag,
	const TArray<FVigilFocusResult>& FocusResults,
	TFunction<void(const UWorld* World, const FVector& Location, const FString& Info, const FColor& Color, float
	Duration)> DrawStringFunc,
	TFunction<void(const UWorld* World, const FMatrix& Matrix, float Radius, const FColor& Color, float Duration)>
	DrawCircleFunc, bool bLocatorAngle, bool bLocatorDistance, float DrawDuration)
{
#if UE_ENABLE_DEBUG_DRAWING
	if (!IsValid(PC) || !PC->GetWorld())
	{
		return;
	}

	for (int32 i=0; i < FocusResults.Num(); i++)
	{
		const FVigilFocusResult& FocusResult = FocusResults[i];
		if (!FocusResult.HitResult.GetActor() || !FocusResult.HitResult.GetComponent())
		{
			continue;
		}

		const FString Tag = FocusResult.FocusTag.ToString();
		const FString Score = FString::Printf(TEXT("P: %d Score:%.1f"), i, FocusResult.Score);

		float NormalizedAngle, MaxAngle, NormalizedDistance, MaxDistance;
		const float AngleValue = GetAngleToVigilTarget(FocusResult.HitResult, NormalizedAngle, MaxAngle);
		const float DistanceValue = GetDistanceToVigilTarget(FocusResult.HitResult, NormalizedDistance, MaxDistance);

		const FString Angle = FString::Printf(TEXT("A: %.1fº / %.1fº (%.1f%%)"), AngleValue, MaxAngle, NormalizedAngle);
		const FString Distance = FString::Printf(TEXT("D: %.1f / %.1f (%.1f%%)"), DistanceValue, MaxDistance, NormalizedDistance);

		const FString Info = FString::Printf(TEXT("%s\n%s\n%s\n%s"), *Tag, *Score, *Angle, *Distance);

		static constexpr uint8 CV = 200;
		static constexpr FColor PrimaryColor = FColor(0, CV, 0);
		static constexpr FColor SecondaryColor = FColor(CV, CV, 0);
		static constexpr FColor TertiaryColor = FColor(CV, CV / 2, 0);
		static constexpr FColor AltColor = FColor(CV, CV, CV);
		static auto GetColor = [](int32 Priority) -> FColor
		{
			if (Priority == 0) return PrimaryColor;
			if (Priority == 1) return SecondaryColor;
			if (Priority == 2) return TertiaryColor;
			return AltColor;
		};

		// Offset the text location to the bottom-right
		const FVector Right = FocusResult.HitResult.GetComponent()->GetRightVector();
		const FVector Up = FocusResult.HitResult.GetComponent()->GetUpVector();
		const FVector WorldLocation = FocusResult.HitResult.ImpactPoint;

		static constexpr float Offset = 5.f;
		const FVector InfoLocation = WorldLocation + (Right * Offset) + (Up * -Offset);

		// Draw the text
		DrawStringFunc(PC->GetWorld(), InfoLocation, Info, GetColor(i), DrawDuration);
		// DrawDebugString(PC->GetWorld(), InfoLocation, Info, nullptr, GetColor(i), DrawDuration, true, 1.f);

		// Draw a locator
		if (bLocatorAngle || bLocatorDistance)
		{
			// Estimate the size to draw a locator at based on the bounds of the target
			const float Radius = FocusResult.HitResult.GetActor() ? FocusResult.HitResult.GetActor()->GetSimpleCollisionRadius() : 0.f;
			const float SmallRadius = Radius * 0.25f;
			const float BigRadius = Radius * 0.8f;

			// Scale the radius based on the angle
			const float AngleRadius = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 0.5f), FVector2D(SmallRadius, BigRadius), NormalizedAngle);
			const float DistanceRadius = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(SmallRadius, BigRadius), NormalizedDistance);
			float LocatorRadius;
			if (bLocatorAngle && bLocatorDistance)
			{
				LocatorRadius = (AngleRadius + DistanceRadius) * 0.5f;
			}
			else if (bLocatorAngle)
			{
				LocatorRadius = AngleRadius;
			}
			else
			{
				LocatorRadius = DistanceRadius;
			}

			// Draw the locator
			const FVector WorldNormal = FocusResult.HitResult.Normal;
			const FColor LocatorColor = (FLinearColor(GetColor(i)) * 0.5f).ToFColor(true);
			FMatrix LocatorMatrix = FRotationMatrix::MakeFromX(WorldNormal);
			LocatorMatrix.SetOrigin(WorldLocation);
			DrawCircleFunc(PC->GetWorld(), LocatorMatrix, LocatorRadius, LocatorColor, DrawDuration);
			// DrawDebugCircle(PC->GetWorld(), LocatorMatrix, LocatorRadius, 16, LocatorColor, false, DrawDuration, 10, 2.f);
		}
	}
#endif
}
