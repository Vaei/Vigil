// Copyright (c) Jared Taylor


#include "VigilStatics.h"

#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "VigilComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilStatics)

UVigilComponent* UVigilStatics::FindVigilComponent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	
	// Only Local and Authority has a PlayerController
	if (Actor->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return nullptr;
	}
	
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const APlayerController* PlayerController = Pawn->GetController<APlayerController>())
		{
			return PlayerController->FindComponentByClass<UVigilComponent>();
		}
	}
	else if (const APlayerState* PlayerState = Cast<APlayerState>(Actor))
	{
		if (const APlayerController* PlayerController = PlayerState->GetPlayerController())
		{
			return PlayerController->FindComponentByClass<UVigilComponent>();
		}
	}
	else if (const APlayerController* PlayerController = Cast<APlayerController>(Actor))
	{
		return PlayerController->FindComponentByClass<UVigilComponent>();
	}
	return nullptr;
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
