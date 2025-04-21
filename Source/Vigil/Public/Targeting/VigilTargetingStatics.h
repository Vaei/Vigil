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
	static FVector GetSourceLocation(const FTargetingRequestHandle& TargetingHandle, EVigilTargetLocationSource_AOE LocationSource);

	/** Native Event to get a source location offset for the AOE */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	static FVector GetSourceOffset(const FTargetingRequestHandle& TargetingHandle, EVigilTargetLocationSource_AOE LocationSource,
		FVector DefaultSourceLocationOffset = FVector::ZeroVector, bool bUseRelativeLocationOffset = true);

	/** Native event to get the source rotation for the AOE  */
	UFUNCTION(BlueprintCallable, Category=Vigil)
	static FQuat GetSourceRotation(const FTargetingRequestHandle& TargetingHandle, EVigilTargetRotationSource_AOE RotationSource);

	/** Setup CollisionQueryParams for the AOE */
	static void InitCollisionParams(const FTargetingRequestHandle& TargetingHandle, FCollisionQueryParams& OutParams,
		bool bIgnoreSourceActor = true, bool bIgnoreInstigatorActor = false, bool bTraceComplex = false);
};
