// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VigilTypes.h"

#include "VigilStatics.generated.h"

class UVigilComponent;
struct FScalableFloat;

/**
 * Helper functions for Vigil
 */
UCLASS()
class VIGIL_API UVigilStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Attempts to find a Vigil Component for the given Actor
	 * Must be Pawn, PlayerController, or PlayerState or will return nullptr
	 * Will return nullptr on SimulatedProxy
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	static UVigilComponent* FindVigilComponentForActor(AActor* Actor);
	
	/**
	 * Attempts to find a Vigil Component for the given Pawn
	 * Will return nullptr on SimulatedProxy
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	static UVigilComponent* FindVigilComponentForPawn(APawn* Pawn);

	/**
	 * Attempts to find a Vigil Component for the given Player Controller
	 * Will return nullptr for non-PlayerController Controller
	 * Will return nullptr on SimulatedProxy
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	static UVigilComponent* FindVigilComponentForController(AController* Controller);

	/**
	 * Attempts to find a Vigil Component for the given PlayerController
	 * Will return nullptr on SimulatedProxy
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	static UVigilComponent* FindVigilComponentForPlayerController(APlayerController* Controller);

	/**
	 * Attempts to find a Vigil Component for the given PlayerState
	 * Will return nullptr on SimulatedProxy
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	static UVigilComponent* FindVigilComponentForPlayerState(APlayerState* PlayerState);
	
public:
	/**
	 * Compute the distance to the target from the hit result
	 * @param HitResult The hit result to compute the distance from -- must have been output from Vigil Targeting Selection Task
	 * @param NormalizedDistance The normalized distance to the target (0.0 - 1.0)
	 * @param MaxDistance The maximum distance to the target
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil, meta=(DisplayName="Get Distance to Vigil Target"))
	static float GetDistanceToVigilTarget(const FHitResult& HitResult, float& NormalizedDistance, float& MaxDistance);

	/**
	 * Compute the angle to the target from the hit result
	 * @param HitResult The hit result to compute the angle from -- must have been output from Vigil Targeting Selection Task
	 * @param NormalizedAngle The normalized angle to the target (0.0 - 1.0)
	 * @param MaxAngle The maximum angle to the target
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil, meta=(DisplayName="Get Angle to Vigil Target"))
	static float GetAngleToVigilTarget(const FHitResult& HitResult, float& NormalizedAngle, float& MaxAngle);

public:
	static FString NetSyncToString(EVigilNetSyncType SyncType);
	
public:
	UFUNCTION(BlueprintCallable, Category=Vigil, meta=(DisplayName="Vigil Draw Debug Results", Keywords="debug", DevelopmentOnly))
	static void VigilDrawDebugResults(APlayerController* PC, const FGameplayTag& FocusTag,
		const TArray<FVigilFocusResult>& FocusResults, float DrawDuration=0.05f, bool bLocatorAngle = true,
		bool bLocatorDistance = false);

	UFUNCTION(BlueprintCallable, Category=Vigil, meta=(DisplayName="Vigil Add Visual Logger Results", Keywords="debug", DevelopmentOnly))
	static void VigilAddVisualLoggerResults(APlayerController* PC, const FGameplayTag& FocusTag,
		const TArray<FVigilFocusResult>& FocusResults, bool bLocatorAngle = true,
		bool bLocatorDistance = false);

	static void VigilDrawDebugResults_Internal(APlayerController* PC, const FGameplayTag& FocusTag,
		const TArray<FVigilFocusResult>& FocusResults,
		TFunction<void(const UWorld* World, const FVector& Location, const FString& Info, const FColor& Color, float Duration)> DrawStringFunc,
		TFunction<void(const UWorld* World, const FMatrix& Matrix, float Radius, const FColor& Color, float Duration)> DrawCircleFunc,
		bool bLocatorAngle = true,
		bool bLocatorDistance = false, float DrawDuration=0.05f);
	
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
