// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "VigilTypes.h"

#include "VigilStatics.generated.h"

namespace EDrawDebugTrace
{
	enum Type : int;
}

struct FScalableFloat;

/**
 * Helper functions for Vigil
 */
UCLASS()
class VIGIL_API UVigilStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Rendering|Debug", meta=(WorldContext="WorldContextObject", DevelopmentOnly))
	static bool IsPointWithinCone(const FVector& Point, const FVector ConeOrigin, const FVector& ConeDirection, FVigilConeShape Cone);
	
	UFUNCTION(BlueprintCallable, Category="Rendering|Debug", meta=(WorldContext="WorldContextObject", DevelopmentOnly))
	static void DrawVigilDebugConeBox(const UObject* WorldContextObject, const FVector Origin, const FRotator Orientation,
		FVigilConeShape Cone, FLinearColor LineColor, float Duration=0.f, float Thickness=0.f);

	UFUNCTION(BlueprintCallable, Category="Rendering|Debug", meta=(WorldContext="WorldContextObject", DevelopmentOnly))
	static void DrawVigilDebugCone(const UObject* WorldContextObject, const FVector Origin, const FRotator Orientation,
		FVigilConeShape Cone, FLinearColor LineColor, int32 Segments=16, float Duration=0.f, float Thickness=0.f);
	
	static void DrawVigilDebugConeBox_Internal(const UWorld* World, const FVector& Origin, const FRotator& Orientation,
		FVigilConeShape Cone, FLinearColor Color, float LifeTime = -1.f, float Thickness = 0.f, uint8 DepthPriority = 0);
	
	static void DrawVigilDebugCone_Internal(const UWorld* World, const FVector& Origin, const FRotator& Orientation,
		FVigilConeShape Cone, FLinearColor Color, int32 Segments = 16, float LifeTime = -1.f, 
		float Thickness = 0.f, uint8 DepthPriority = 0);

	UFUNCTION(BlueprintPure, Category=Vigil)
	static FVector GetConeBoxShapeExtent(FVigilConeShape Cone);
	
	// /**
	//  * This is a debugging function for Vigil to ensure that the MakeConeBoxShape works as expected
	//  * It is actually just a box trace
	//  */
	// UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", WorldContext="WorldContextObject", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "Multi Cone Trace By Channel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	// static bool ConeBoxTraceMulti(const UObject* WorldContextObject, const FVector Start, const FVector End,
	// 	float Length, float AngleWidth, float AngleHeight, FRotator Orientation, ETraceTypeQuery TraceChannel, bool bTraceComplex,
	// 	const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits,
	// 	bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green,
	// 	float DrawTime = 5.0f);

	/**
	* Performs a box trace and draws the cone shape
	* For debugging only, results will never be useful
	*/
	// UFUNCTION(BlueprintCallable, Category = "Collision", meta = (DevelopmentOnly, bIgnoreSelf = "true", WorldContext="WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Multi Cone Trace By Channel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	static bool VigilConeTraceDebug(const UObject* WorldContextObject, const FVector Start, const FVector InEnd, FVigilConeShape Cone,
		const FRotator Orientation, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
		TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red,
		FLinearColor TraceHitColor = FLinearColor::Green);
};
