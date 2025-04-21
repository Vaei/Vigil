// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "VigilTargetingTypes.h"

#include "VigilTargetingStatics.generated.h"

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
	 */
	UFUNCTION(BlueprintCallable, Category=Vigil, meta=(DisplayName="Get Distance to Vigil Target"))
	static float GetDistanceToVigilTarget(const FHitResult& HitResult, float& NormalizedDistance, float& MaxDistance);
	
	UFUNCTION(BlueprintCallable, Category=Vigil, meta=(DisplayName="Get Angle to Vigil Target"))
	static float GetAngleToVigilTarget(const FHitResult& HitResult, float& NormalizedAngle, float& MaxAngle);
};
