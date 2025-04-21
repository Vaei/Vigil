// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VigilTypes.h"

#include "VigilStatics.generated.h"

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
	static FVector GetConeBoxShapeHalfExtent(FVigilConeShape Cone);
};
