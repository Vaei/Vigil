// Copyright (c) Jared Taylor


#include "VigilStatics.h"

#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "VigilComponent.h"
#include "VigilTypes.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/HUD.h"
#include "Components/PrimitiveComponent.h"

#if ENABLE_VISUAL_LOG
#include "VisualLogger/VisualLogger.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilStatics)


UVigilComponent* UVigilStatics::FindVigilComponentForActor(AActor* Actor)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilStatics::FindVigilComponentForActor);

	if (!IsValid(Actor))
	{
		return nullptr;
	}

	// Only Local and Authority has a PlayerController
	if (Actor->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return nullptr;
	}

	// Maybe the actor is a player controller
	if (const APlayerController* PlayerController = Cast<APlayerController>(Actor))
	{
		return PlayerController->FindComponentByClass<UVigilComponent>();
	}

	// Maybe the actor is a pawn
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const APlayerController* PlayerController = Pawn->GetController<APlayerController>())
		{
			return PlayerController->FindComponentByClass<UVigilComponent>();
		}
	}

	// Maybe the actor is a player state
	if (const APlayerState* PlayerState = Cast<APlayerState>(Actor))
	{
		if (const APlayerController* PlayerController = PlayerState->GetPlayerController())
		{
			return PlayerController->FindComponentByClass<UVigilComponent>();
		}
	}

	// Unsupported actor
	return nullptr;
}

UVigilComponent* UVigilStatics::FindVigilComponentForPawn(APawn* Pawn)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilStatics::FindVigilComponentForPawn);
	
	if (!IsValid(Pawn))
	{
		return nullptr;
	}
	
	// Only Local and Authority has a PlayerController
	if (Pawn->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return nullptr;
	}
	
	if (const APlayerController* PlayerController = Pawn->GetController<APlayerController>())
	{
		return PlayerController->FindComponentByClass<UVigilComponent>();
	}
	return nullptr;
}

UVigilComponent* UVigilStatics::FindVigilComponentForController(AController* Controller)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilStatics::FindVigilComponentForController);
	
	if (!IsValid(Controller))
	{
		return nullptr;
	}
	
	// Only Local and Authority has a PlayerController
	if (Controller->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return nullptr;
	}
	
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		return PlayerController->FindComponentByClass<UVigilComponent>();
	}
	return nullptr;
}

UVigilComponent* UVigilStatics::FindVigilComponentForPlayerController(APlayerController* Controller)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilStatics::FindVigilComponentForPlayerController);
	
	if (!IsValid(Controller))
	{
		return nullptr;
	}
	
	// Only Local and Authority has a PlayerController
	if (Controller->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return nullptr;
	}

	return Controller->FindComponentByClass<UVigilComponent>();
}

UVigilComponent* UVigilStatics::FindVigilComponentForPlayerState(APlayerState* PlayerState)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilStatics::FindVigilComponentForPlayerState);
	
	if (!IsValid(PlayerState))
	{
		return nullptr;
	}
	
	// Only Local and Authority has a PlayerController
	if (PlayerState->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return nullptr;
	}
	
	if (const APlayerController* PlayerController = PlayerState->GetPlayerController())
	{
		return PlayerController->FindComponentByClass<UVigilComponent>();
	}
	return nullptr;
}

float UVigilStatics::GetDistanceToVigilTarget(const FHitResult& HitResult, float& NormalizedDistance, float& MaxDistance)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilStatics::GetDistanceToVigilTarget);
	
	// This is the location we traced from to find a target
	const FVector SourceLocation = HitResult.TraceStart;

	// This is the location where we found the target
	const FVector TargetLocation = HitResult.ImpactPoint;
	
	MaxDistance = HitResult.PenetrationDepth;
	const float Distance = FVector::Distance(SourceLocation, TargetLocation);
	NormalizedDistance = FMath::Clamp(Distance / MaxDistance, 0.f, 1.f);
	return Distance;
}

float UVigilStatics::GetAngleToVigilTarget(const FHitResult& HitResult, float& NormalizedAngle, float& MaxAngle)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilStatics::GetAngleToVigilTarget);
	
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

FString UVigilStatics::NetSyncToString(EVigilNetSyncType SyncType)
{
	switch (SyncType)
	{
	case EVigilNetSyncType::BothWait: return TEXT("BothWait");
	case EVigilNetSyncType::OnlyServerWait: return TEXT("OnlyServerWait");
	case EVigilNetSyncType::OnlyClientWait: return TEXT("OnlyClientWait");
	default: return TEXT("Unknown");
	}
}

void UVigilStatics::VigilDrawDebugResults(APlayerController* PC, const FGameplayTag& FocusTag,
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

void UVigilStatics::VigilAddVisualLoggerResults(APlayerController* PC, const FGameplayTag& FocusTag,
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

void UVigilStatics::VigilDrawDebugResults_Internal(APlayerController* PC, const FGameplayTag& FocusTag,
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

bool UVigilStatics::IsPointWithinCone(const FVector& Point, const FVector ConeOrigin, const FVector& ConeDirection,
	FVigilConeShape Cone)
{
	return Cone.IsPointWithinCone(Point, ConeOrigin, ConeDirection);
}

void UVigilStatics::DrawVigilDebugConeBox(const UObject* WorldContextObject, const FVector Origin,
	const FRotator Orientation, FVigilConeShape Cone, FLinearColor LineColor,
	float Duration, float Thickness)
{
#if UE_ENABLE_DEBUG_DRAWING
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		DrawVigilDebugConeBox_Internal(World, Origin, Orientation, Cone, LineColor,
			Duration, Thickness);
	}
#endif
}

void UVigilStatics::DrawVigilDebugCone(const UObject* WorldContextObject, const FVector Origin,
	const FRotator Orientation, FVigilConeShape Cone, FLinearColor LineColor,
	int32 Segments, float Duration, float Thickness)
{
#if UE_ENABLE_DEBUG_DRAWING
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		DrawVigilDebugCone_Internal(World, Origin, Orientation, Cone, LineColor,
			Segments, Duration, Thickness);
	}
#endif
}

void UVigilStatics::DrawVigilDebugConeBox_Internal(const UWorld* World, const FVector& Origin, const FRotator& Orientation,
	FVigilConeShape Cone, FLinearColor Color, float LifeTime, float Thickness, uint8 DepthPriority)
{
#if UE_ENABLE_DEBUG_DRAWING
	const float HalfAngleWidthRad = FMath::DegreesToRadians(0.5f * Cone.AngleWidth);
	const float HalfAngleHeightRad = FMath::DegreesToRadians(0.5f * Cone.AngleHeight);

	const float RadiusY = FMath::Tan(HalfAngleWidthRad) * Cone.Length;
	const float RadiusZ = FMath::Tan(HalfAngleHeightRad) * Cone.Length;

	const FVector Forward = Orientation.Vector();
	const FVector Right = Orientation.RotateVector(FVector::RightVector);
	const FVector Up = Orientation.RotateVector(FVector::UpVector);

	const FVector Tip = Origin;
	const FVector BaseCenter = Tip + Forward * Cone.Length;

	// Define corners of the base plane
	const FVector BaseTopLeft     = BaseCenter + Right * RadiusY + Up * RadiusZ;
	const FVector BaseTopRight    = BaseCenter - Right * RadiusY + Up * RadiusZ;
	const FVector BaseBottomLeft  = BaseCenter + Right * RadiusY - Up * RadiusZ;
	const FVector BaseBottomRight = BaseCenter - Right * RadiusY - Up * RadiusZ;

	const FColor LineColor = Color.ToFColor(true);

	// Draw edges from tip to base corners
	DrawDebugLine(World, Tip, BaseTopLeft,     LineColor, false, LifeTime, DepthPriority, Thickness);
	DrawDebugLine(World, Tip, BaseTopRight,    LineColor, false, LifeTime, DepthPriority, Thickness);
	DrawDebugLine(World, Tip, BaseBottomLeft,  LineColor, false, LifeTime, DepthPriority, Thickness);
	DrawDebugLine(World, Tip, BaseBottomRight, LineColor, false, LifeTime, DepthPriority, Thickness);

	// Draw base box edges
	DrawDebugLine(World, BaseTopLeft,     BaseTopRight,    LineColor, false, LifeTime, DepthPriority, Thickness);
	DrawDebugLine(World, BaseTopRight,    BaseBottomRight, LineColor, false, LifeTime, DepthPriority, Thickness);
	DrawDebugLine(World, BaseBottomRight, BaseBottomLeft,  LineColor, false, LifeTime, DepthPriority, Thickness);
	DrawDebugLine(World, BaseBottomLeft,  BaseTopLeft,     LineColor, false, LifeTime, DepthPriority, Thickness);
#endif
}

void UVigilStatics::DrawVigilDebugCone_Internal(const UWorld* World, const FVector& Origin, const FRotator& Orientation,
	FVigilConeShape Cone, FLinearColor Color, int32 Segments, float LifeTime, float Thickness, uint8 DepthPriority)
{
#if UE_ENABLE_DEBUG_DRAWING
	const FVector Forward = Orientation.Vector();
	const FVector Right = Orientation.RotateVector(FVector::RightVector);
	const FVector Up = Orientation.RotateVector(FVector::UpVector);

	const float HalfAngleX = FMath::DegreesToRadians(0.5f * Cone.AngleWidth);
	const float HalfAngleY = FMath::DegreesToRadians(0.5f * Cone.AngleHeight);
	const float RadiusX = FMath::Tan(HalfAngleX) * Cone.Length;
	const float RadiusY = FMath::Tan(HalfAngleY) * Cone.Length;

	const FVector BaseCenter = Origin + Forward * Cone.Length;

	const float Step = 2.0f * PI / Segments;
	FVector PrevPoint;

	for (int32 i = 0; i <= Segments; ++i)
	{
		const float Angle = Step * i;
		// Ellipse in base plane
		FVector Offset = RadiusX * FMath::Cos(Angle) * Right +
						 RadiusY * FMath::Sin(Angle) * Up;
		FVector Point = BaseCenter + Offset;

		// Edge from tip
		DrawDebugLine(World, Origin, Point, Color.ToFColor(true), false, LifeTime, DepthPriority, Thickness);

		if (i > 0)
		{
			DrawDebugLine(World, PrevPoint, Point, Color.ToFColor(true), false, LifeTime, DepthPriority, Thickness);
		}

		PrevPoint = Point;
	}
#endif
}

FVector UVigilStatics::GetConeBoxShapeHalfExtent(FVigilConeShape Cone)
{
	return Cone.GetConeBoxShapeHalfExtent();
}
