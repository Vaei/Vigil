﻿// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "VigilTypes.h"
#include "Targeting/VigilTargetingTypes.h"
#include "Tasks/TargetingSelectionTask_AOE.h"
#include "VigilTargetSelection.generated.h"

/**
 * Extend the shapes to include a cone
 * Adds location and rotation sources
 */
UCLASS(Blueprintable, DisplayName="Vigil Target Selection")
class VIGIL_API UVigilTargetSelection : public UTargetingTask
{
	GENERATED_BODY()

protected:
	/** The collision channel to use for the overlap check (as long as Collision Profile Name is not set) */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	TEnumAsByte<ECollisionChannel> CollisionChannel;
	
	/** The collision profile name to use for the overlap check */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	FCollisionProfileName CollisionProfileName;

	/** The object types to use for the overlap check */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	TArray<TEnumAsByte<EObjectTypeQuery>> CollisionObjectTypes;

	/** The collision channel to use for the cone target overlap check */
	UPROPERTY(EditAnywhere, Category="Vigil Selection", meta=(EditCondition="ShapeType==EVigilTargetingShape::Cone", EditConditionHides))
	TEnumAsByte<ECollisionChannel> ConeTargetCollisionChannel;

	/** Location to trace from */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	EVigilTargetLocationSource LocationSource;

	/** Rotation to trace from */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	EVigilTargetRotationSource RotationSource;

	/** What to check against for the cone's target */
	UPROPERTY(EditAnywhere, Category="Vigil Selection", meta=(EditCondition="ShapeType==EVigilTargetingShape::Cone", EditConditionHides))
	EVigilConeTargetLocationSource ConeTargetSource;
	
	/** The default source location offset used by GetSourceOffset */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	FVector DefaultSourceLocationOffset = FVector::ZeroVector;

	/** Should we offset based on world or relative Source object transform? */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	uint8 bUseRelativeLocationOffset : 1;

	/** The default source rotation offset used by GetSourceOffset */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	FRotator DefaultSourceRotationOffset = FRotator::ZeroRotator;

protected:
	/** If true can successfully overlap multiple components on the same actor */
	UPROPERTY(EditAnywhere, Category="Grasp Selection")
	bool bTraceMultipleComponentsPerActor;
	
	/** When enabled, the trace will be performed against complex collision. */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	uint8 bTraceComplex : 1 = false;
	
protected:
	/** Indicates the trace should ignore the source actor */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	uint8 bIgnoreSourceActor : 1;

	/** Indicates the trace should ignore the source actor */
	UPROPERTY(EditAnywhere, Category="Vigil Selection")
	uint8 bIgnoreInstigatorActor : 1;
	
protected:
	/** The shape type to use for the AOE */
	UPROPERTY(EditAnywhere, Category="Vigil Selection Shape")
	EVigilTargetingShape ShapeType = EVigilTargetingShape::Cone;

	UPROPERTY(EditAnywhere, Category="Vigil Selection Shape", meta=(EditCondition="ShapeType==EVigilTargetingShape::Cone", EditConditionHides))
	FScalableFloat ConeLength;

	UPROPERTY(EditAnywhere, Category="Vigil Selection Shape", meta=(EditCondition="ShapeType==EVigilTargetingShape::Cone", EditConditionHides))
	FScalableFloat ConeAngleWidth = 0.0f;

	UPROPERTY(EditAnywhere, Category="Vigil Selection Shape", meta=(EditCondition="ShapeType==EVigilTargetingShape::Cone", EditConditionHides))
	FScalableFloat ConeAngleHeight;

	FVigilConeShape GetConeShape() const
	{
		return FVigilConeShape::MakeConeFromScalableFloat(ConeLength, ConeAngleWidth, ConeAngleHeight);
	}

	/** The half extent to use for box and cylinder */
	UPROPERTY(EditAnywhere, Category="Vigil Selection Shape", meta=(EditCondition="ShapeType==EVigilTargetingShape::Box||ShapeType==EVigilTargetingShape::Cylinder", EditConditionHides))
	FVector HalfExtent = FVector::ZeroVector;

	/** The radius to use for sphere and capsule overlaps */
	UPROPERTY(EditAnywhere, Category="Vigil Selection Shape", meta=(EditCondition="ShapeType==EVigilTargetingShape::Sphere||ShapeType==EVigilTargetingShape::Capsule", EditConditionHides))
	FScalableFloat Radius = 0.0f;

	/** The half height to use for capsule overlap checks */
	UPROPERTY(EditAnywhere, Category="Vigil Selection Shape", meta=(EditCondition="ShapeType==EVigilTargetingShape::Capsule", EditConditionHides))
	FScalableFloat HalfHeight = 0.0f;

	/**
	 * The component tag to use if a custom component is desired as the overlap shape.
	 * Use to look up the component on the source actor
	 */
	UPROPERTY(EditAnywhere, Category="Vigil Selection Shape", meta=(EditCondition="ShapeType==EVigilTargetingShape::SourceComponent", EditConditionHides))
	FName ComponentTag = NAME_None;
	
public:
	UVigilTargetSelection(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** Native Event to get the source location for the AOE */
	UFUNCTION(BlueprintNativeEvent, Category="Vigil Selection")
	FVector GetSourceLocation(const FTargetingRequestHandle& TargetingHandle) const;

	/** Native Event to get a source location offset for the AOE */
	UFUNCTION(BlueprintNativeEvent, Category="Vigil Selection")
	FVector GetSourceOffset(const FTargetingRequestHandle& TargetingHandle) const;

	/** Native event to get the source rotation for the AOE  */
	UFUNCTION(BlueprintNativeEvent, Category="Vigil Selection")
	FQuat GetSourceRotation(const FTargetingRequestHandle& TargetingHandle) const;

	/** Native event to get the source rotation for the AOE  */
	UFUNCTION(BlueprintNativeEvent, Category="Vigil Selection")
	FQuat GetSourceRotationOffset(const FTargetingRequestHandle& TargetingHandle) const;

public:
	/** Evaluation function called by derived classes to process the targeting request */
	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;

protected:
	/** Method to process the trace task immediately */
	void ExecuteImmediateTrace(const FTargetingRequestHandle& TargetingHandle) const;

	/** Method to process the trace task asynchronously */
	void ExecuteAsyncTrace(const FTargetingRequestHandle& TargetingHandle) const;

	/** Callback for an async overlap */
	void HandleAsyncOverlapComplete(const FTraceHandle& InTraceHandle, FOverlapDatum& InOverlapDatum,
		FTargetingRequestHandle TargetingHandle) const;

	/**
	 * Method to take the overlap results and store them in the targeting result data
	 * @return Num valid results
	 */
	int32 ProcessOverlapResults(const FTargetingRequestHandle& TargetingHandle, const TArray<FOverlapResult>& Overlaps) const;
	
protected:
	/** Helper method to build the Collision Shape */
	FCollisionShape GetCollisionShape() const;
	
	/** Helper method to find the custom component defined on the source actor */
	const UPrimitiveComponent* GetCollisionComponent(const FTargetingRequestHandle& TargetingHandle) const;

	/** Setup CollisionQueryParams for the AOE */
	void InitCollisionParams(const FTargetingRequestHandle& TargetingHandle, FCollisionQueryParams& OutParams) const;
	
public:
	/** Debug draws the outlines of the set shape type. */
	void DebugDrawBoundingVolume(const FTargetingRequestHandle& TargetingHandle, const FColor& Color,
		const FColor& ColorAlt, const FOverlapDatum* OverlapDatum = nullptr) const;

#if UE_ENABLE_DEBUG_DRAWING
protected:
	virtual void DrawDebug(UTargetingSubsystem* TargetingSubsystem, FTargetingDebugInfo& Info,
		const FTargetingRequestHandle& TargetingHandle, float XOffset, float YOffset, int32 MinTextRowsToAdvance) const override;
	void BuildDebugString(const FTargetingRequestHandle& TargetingHandle, const TArray<FTargetingDefaultResultData>& TargetResults) const;
	void ResetDebugString(const FTargetingRequestHandle& TargetingHandle) const;
#endif
};
