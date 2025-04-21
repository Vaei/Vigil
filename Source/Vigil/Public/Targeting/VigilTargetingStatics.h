// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VigilTargetingTypes.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h" 
#include "GameplayTagContainer.h"
#include "VigilTargetingStatics.generated.h"

struct FVigilFocusResult;
struct FTargetingRequestHandle;

/**
 * Helper functions for Vigil Targeting
 */
UCLASS()
class VIGIL_API UVigilTargetingStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Native Event to get the source location for the AOE */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	static FVector GetSourceLocation(const FTargetingRequestHandle& TargetingHandle, EVigilTargetLocationSource LocationSource);

	/** Native Event to get a source location offset for the AOE */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	static FVector GetSourceOffset(const FTargetingRequestHandle& TargetingHandle, EVigilTargetLocationSource LocationSource,
		FVector DefaultSourceLocationOffset = FVector::ZeroVector, bool bUseRelativeLocationOffset = true);

	/** Native event to get the source rotation for the AOE  */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	static FQuat GetSourceRotation(const FTargetingRequestHandle& TargetingHandle, EVigilTargetRotationSource RotationSource);

	/** Setup CollisionQueryParams for the AOE */
	static void InitCollisionParams(const FTargetingRequestHandle& TargetingHandle, FCollisionQueryParams& OutParams,
		bool bIgnoreSourceActor = true, bool bIgnoreInstigatorActor = false, bool bTraceComplex = false);

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
	
	UFUNCTION(BlueprintCallable, Category=Vigil, meta=(DisplayName="Vigil Draw Debug Results", DevelopmentOnly))
	static void VigilDrawDebugResults(APlayerController* PC, const FGameplayTag& FocusTag,
		const TArray<FVigilFocusResult>& FocusResults, float DrawDuration=0.05f, bool bLocatorAngle = true,
		bool bLocatorDistance = false);

	UFUNCTION(BlueprintCallable, Category=Vigil, meta=(DisplayName="Vigil Add Visual Logger Results", DevelopmentOnly))
	static void VigilAddVisualLoggerResults(APlayerController* PC, const FGameplayTag& FocusTag,
		const TArray<FVigilFocusResult>& FocusResults, bool bLocatorAngle = true,
		bool bLocatorDistance = false);

	static void VigilDrawDebugResults_Internal(APlayerController* PC, const FGameplayTag& FocusTag,
		const TArray<FVigilFocusResult>& FocusResults,
		TFunction<void(const UWorld* World, const FVector& Location, const FString& Info, const FColor& Color, float Duration)> DrawStringFunc,
		TFunction<void(const UWorld* World, const FMatrix& Matrix, float Radius, const FColor& Color, float Duration)> DrawCircleFunc,
		bool bLocatorAngle = true,
		bool bLocatorDistance = false, float DrawDuration=0.05f);
};
