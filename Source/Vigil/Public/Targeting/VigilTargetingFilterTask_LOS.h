// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Targeting/VigilTargetingTypes.h"
#include "VigilTargetingFilterTask_LOS.generated.h"

/**
 * Used to filter targets by line of sight
 */
UCLASS(Blueprintable)
class VIGIL_API UVigilTargetingFilterTask_LOS : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	/** The collision channel to use for the overlap check (as long as Collision Profile Name is not set) */
	UPROPERTY(EditAnywhere, Category="Filter LOS")
	TEnumAsByte<ECollisionChannel> CollisionChannel;
	
	/** The collision profile name to use for the overlap check */
	UPROPERTY(EditAnywhere, Category="Filter LOS")
	FCollisionProfileName CollisionProfileName;

	/** The object types to use for the overlap check */
	UPROPERTY(EditAnywhere, Category="Filter LOS")
	TArray<TEnumAsByte<EObjectTypeQuery>> CollisionObjectTypes;

	UPROPERTY(EditAnywhere, Category="Filter LOS")
	EVigilTargetLocationSource_AOE LocationSource;

	// UPROPERTY(EditAnywhere, Category="Filter LOS")
	// EVigilTargetRotationSource_AOE RotationSource;

	/** The default source location offset used by GetSourceOffset */
	UPROPERTY(EditAnywhere, Category="Filter LOS")
	FVector DefaultSourceLocationOffset = FVector::ZeroVector;

	/** Should we offset based on world or relative Source object transform? */
	UPROPERTY(EditAnywhere, Category="Filter LOS")
	uint8 bUseRelativeLocationOffset : 1;
	
	// /** The default source rotation offset used by GetSourceOffset */
	// UPROPERTY(EditAnywhere, Category="Filter LOS")
	// FRotator DefaultSourceRotationOffset = FRotator::ZeroRotator;

	/** Where to trace to determine if we can locate the target */
	UPROPERTY(EditAnywhere, Category="Filter LOS")
	EVigilTargetLocationSource_LOS TargetLocationSource;

	UPROPERTY(EditAnywhere, Category="Filter LOS")
	bool bIgnoreMovableTargets;
	
protected:
	/** Indicates the trace should ignore the source actor */
	UPROPERTY(EditAnywhere, Category="Filter LOS")
	uint8 bIgnoreSourceActor : 1;

	/** Indicates the trace should ignore the source actor */
	UPROPERTY(EditAnywhere, Category="Filter LOS")
	uint8 bIgnoreInstigatorActor : 1;
	
	/** When enabled, the trace will be performed against complex collision. */
	UPROPERTY(EditAnywhere, Category="Filter LOS")
	uint8 bTraceComplex : 1 = false;
	
	/**
	 * Sphere trace radius
	 * Use 0.0 to do a line trace
	 */
	UPROPERTY(EditAnywhere, Category="Filter LOS", meta=(UIMin="0", ClampMin="0", Delta="0.1", ForceUnits="cm"))
	float TraceRadius = 0.f;
	
public:
	UVigilTargetingFilterTask_LOS(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

protected:
	/** Native Event to get the source location for the AOE */
	UFUNCTION(BlueprintNativeEvent, Category="Filter LOS")
	FVector GetSourceLocation(const FTargetingRequestHandle& TargetingHandle) const;

	/** Native Event to get a source location offset for the AOE */
	UFUNCTION(BlueprintNativeEvent, Category="Filter LOS")
	FVector GetSourceOffset(const FTargetingRequestHandle& TargetingHandle) const;

	/** Setup CollisionQueryParams for the trace */
	void InitCollisionParams(const FTargetingRequestHandle& TargetingHandle, FCollisionQueryParams& OutParams) const;
};
