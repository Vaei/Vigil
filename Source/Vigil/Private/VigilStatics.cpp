// Copyright (c) Jared Taylor


#include "VigilStatics.h"

#include "KismetTraceUtils.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilStatics)

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

FVector UVigilStatics::GetConeBoxShapeExtent(FVigilConeShape Cone)
{
	return Cone.GetConeBoxShapeExtent();
}

// bool UVigilStatics::ConeBoxTraceMulti(const UObject* WorldContextObject, const FVector Start, const FVector End,
// 	float Length, float AngleWidth, float AngleHeight, FRotator Orientation, ETraceTypeQuery TraceChannel, bool bTraceComplex,
// 	const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits,
// 	bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
// {
// 	static const FName BoxTraceMultiName(TEXT("ConeBoxTraceMulti"));
// 	FCollisionQueryParams Params(BoxTraceMultiName, SCENE_QUERY_STAT_ONLY(KismetTraceUtils), bTraceComplex);
// 	Params.bReturnPhysicalMaterial = true;
// 	Params.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable; // Ask for face index, as long as we didn't disable globally
// 	Params.AddIgnoredActors(ActorsToIgnore);
// 	if (bIgnoreSelf)
// 	{
// 		const AActor* IgnoreActor = Cast<AActor>(WorldContextObject);
// 		if (IgnoreActor)
// 		{
// 			Params.AddIgnoredActor(IgnoreActor);
// 		}
// 		else
// 		{
// 			// find owner
// 			const UObject* CurrentObject = WorldContextObject;
// 			while (CurrentObject)
// 			{
// 				CurrentObject = CurrentObject->GetOuter();
// 				IgnoreActor = Cast<AActor>(CurrentObject);
// 				if (IgnoreActor)
// 				{
// 					Params.AddIgnoredActor(IgnoreActor);
// 					break;
// 				}
// 			}
// 		}
// 	}
// 	
// 	FCollisionShape ConeBox = MakeConeBoxShape(Length, AngleWidth, AngleHeight);
// 	
// 	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
// 	bool const bHit = World ? World->SweepMultiByChannel(OutHits, Start, End, Orientation.Quaternion(),
// 		UEngineTypes::ConvertToCollisionChannel(TraceChannel), ConeBox, Params) : false;
//
// #if ENABLE_DRAW_DEBUG
// 	DrawDebugBoxTraceMulti(World, Start, End, ConeBox.GetExtent(), Orientation, DrawDebugType, bHit, OutHits, TraceColor, TraceHitColor, DrawTime);
// #endif
//
// 	return bHit;
// }

bool UVigilStatics::VigilConeTraceDebug(const UObject* WorldContextObject, const FVector InStart, const FVector InEnd,
	FVigilConeShape Cone, const FRotator Orientation, ETraceTypeQuery TraceChannel, bool bTraceComplex,
	const TArray<AActor*>& ActorsToIgnore, TArray<FHitResult>& OutHits,	bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor)
{
#if UE_ENABLE_DEBUG_DRAWING
	const FVector Dir = (InEnd - InStart).GetSafeNormal();
	const FVector StartOffset = Dir * Cone.Length * 0.5f;
	const FVector Start = InStart + StartOffset;
	const FVector End = InEnd + StartOffset;
	const FVector HalfSize = Cone.GetConeBoxShapeExtent();

	// Start with a box trace
	const bool bHit = UKismetSystemLibrary::BoxTraceMulti(WorldContextObject, Start, End,
		HalfSize, Orientation, TraceChannel, bTraceComplex, ActorsToIgnore, EDrawDebugTrace::None, OutHits, bIgnoreSelf,
		TraceColor, TraceHitColor, 0.f);

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	// // Iterate hits to test if within cone
	// OutHits.RemoveAll([&Cone, &Start, &Dir, &World](const FHitResult& Hit)
	// {
	// 	// This will never give useful results it is only for debug drawing!!
	// 	DrawDebugPoint(World, Hit.ImpactPoint, 8.f, FColor::Yellow, false, 0.f);
	// 	return !Cone.IsPointWithinCone(Hit.ImpactPoint, Start, Dir);
	// });

	// bHit = OutHits.Num() > 0;
	
	const FLinearColor& Color = bHit ? TraceHitColor : TraceColor;
	DrawVigilDebugCone_Internal(World, InStart, Orientation, Cone, Color, 16, 0.f, 2.f);

	return bHit;
#else
	return false;
#endif
}
