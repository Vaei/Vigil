// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Targeting/VigilTargetingTypes.h"
#include "VigilTargetingFilterTask_LOS.generated.h"

/**
 * Used to filter targets by line of sight
 */
UCLASS(Blueprintable, DisplayName="Vigil Targeting Filter Task (LOS)")
class VIGIL_API UVigilTargetingFilterTask_LOS : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	/** The collision channel to use for the overlap check (as long as Collision Profile Name is not set) */
	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	TEnumAsByte<ECollisionChannel> CollisionChannel;
	
	/** The collision profile name to use for the overlap check */
	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	FCollisionProfileName CollisionProfileName;

	/** The object types to use for the overlap check */
	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	TArray<TEnumAsByte<EObjectTypeQuery>> CollisionObjectTypes;

	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	EVigilTargetLocationSource LocationSource;

	// UPROPERTY(EditAnywhere, Category="Vigil Filter")
	// EVigilTargetRotationSource_AOE RotationSource;

	/** The default source location offset used by GetSourceOffset */
	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	FVector DefaultSourceLocationOffset = FVector::ZeroVector;

	/** Should we offset based on world or relative Source object transform? */
	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	uint8 bUseRelativeLocationOffset : 1;
	
	// /** The default source rotation offset used by GetSourceOffset */
	// UPROPERTY(EditAnywhere, Category="Vigil Filter")
	// FRotator DefaultSourceRotationOffset = FRotator::ZeroRotator;

	/** Where to trace to determine if we can locate the target */
	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	EVigilTargetLocationSource_LOS TargetLocationSource;

	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	bool bIgnoreMovableTargets;
	
protected:
	/** Indicates the trace should ignore the source actor */
	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	uint8 bIgnoreSourceActor : 1;

	/** Indicates the trace should ignore the source actor */
	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	uint8 bIgnoreInstigatorActor : 1;
	
	/** When enabled, the trace will be performed against complex collision. */
	UPROPERTY(EditAnywhere, Category="Vigil Filter")
	uint8 bTraceComplex : 1 = false;
	
	/**
	 * Sphere trace radius
	 * Use 0.0 to do a line trace
	 */
	UPROPERTY(EditAnywhere, Category="Vigil Filter", meta=(UIMin="0", ClampMin="0", Delta="0.1", ForceUnits="cm"))
	float TraceRadius = 0.f;
	
public:
	UVigilTargetingFilterTask_LOS(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

protected:
	/** Native Event to get the source location for the AOE */
	UFUNCTION(BlueprintNativeEvent, Category="Vigil Filter")
	FVector GetSourceLocation(const FTargetingRequestHandle& TargetingHandle) const;

	/** Native Event to get a source location offset for the AOE */
	UFUNCTION(BlueprintNativeEvent, Category="Vigil Filter")
	FVector GetSourceOffset(const FTargetingRequestHandle& TargetingHandle) const;

	/** Setup CollisionQueryParams for the trace */
	void InitCollisionParams(const FTargetingRequestHandle& TargetingHandle, FCollisionQueryParams& OutParams) const;
};
