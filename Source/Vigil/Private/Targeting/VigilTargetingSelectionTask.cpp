// Copyright (c) Jared Taylor


#include "Targeting/VigilTargetingSelectionTask.h"

#include "VigilStatics.h"
#include "Targeting/VigilTargetingStatics.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "System/VigilVersioning.h"

#if UE_ENABLE_DEBUG_DRAWING
#if WITH_EDITORONLY_DATA
#include "Engine/Canvas.h"
#endif
#endif

DEFINE_LOG_CATEGORY_STATIC(LogVigilTargetingSystem, Log, All);

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilTargetingSelectionTask)

namespace FVigilCVars
{
#if UE_ENABLE_DEBUG_DRAWING
	static bool bVigilSelectionDebug = false;
	FAutoConsoleVariableRef CVarVigilSelectionDebug(
		TEXT("p.Vigil.Selection.Debug"),
		bVigilSelectionDebug,
		TEXT("Optionally draw debug for Vigil AOE Selection Task.\n")
		TEXT("If true draw debug for Vigil AOE Selection Task"),
		ECVF_Default);
#endif
}

UVigilTargetingSelectionTask::UVigilTargetingSelectionTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CollisionChannel = ECC_Visibility;
	ConeTargetCollisionChannel = ECC_Visibility;
	bUseRelativeLocationOffset = true;
	bIgnoreSourceActor = true;
	bIgnoreInstigatorActor = false;
	bTraceComplex = false;

	LocationSource = EVigilTargetLocationSource::Camera;
	RotationSource = EVigilTargetRotationSource::ViewRotation;
	ConeTargetSource = EVigilConeTargetLocationSource::Component;

	ConeLength = 1800.f;
	ConeAngleWidth = 25.f;
	ConeAngleHeight = 35.f;
}

FVector UVigilTargetingSelectionTask::GetSourceLocation_Implementation(
	const FTargetingRequestHandle& TargetingHandle) const
{
	return UVigilTargetingStatics::GetSourceLocation(TargetingHandle, LocationSource);
}

FVector UVigilTargetingSelectionTask::GetSourceOffset_Implementation(
	const FTargetingRequestHandle& TargetingHandle) const
{
	return UVigilTargetingStatics::GetSourceOffset(TargetingHandle, LocationSource, DefaultSourceLocationOffset,
		bUseRelativeLocationOffset);
}

FQuat UVigilTargetingSelectionTask::GetSourceRotation_Implementation(
	const FTargetingRequestHandle& TargetingHandle) const
{
	return UVigilTargetingStatics::GetSourceRotation(TargetingHandle, RotationSource);
}

FQuat UVigilTargetingSelectionTask::GetSourceRotationOffset_Implementation(
	const FTargetingRequestHandle& TargetingHandle) const
{
	return DefaultSourceRotationOffset.Quaternion();
}

void UVigilTargetingSelectionTask::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
	Super::Execute(TargetingHandle);
	
	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

	// @note: There isn't Async Overlap support based on Primitive Component, so even if using async targeting, it will
	// run this task in "immediate" mode.
	if (IsAsyncTargetingRequest(TargetingHandle) && (ShapeType != EVigilTargetingShape::SourceComponent))
	{
		ExecuteAsyncTrace(TargetingHandle);
	}
	else
	{
		ExecuteImmediateTrace(TargetingHandle);
	}
}

void UVigilTargetingSelectionTask::ExecuteImmediateTrace(const FTargetingRequestHandle& TargetingHandle) const
{
#if UE_ENABLE_DEBUG_DRAWING
	ResetDebugString(TargetingHandle);
#endif // UE_ENABLE_DEBUG_DRAWING

	const UWorld* World = GetSourceContextWorld(TargetingHandle);
	if (World && TargetingHandle.IsValid())
	{
		const FVector SourceLocation = GetSourceLocation(TargetingHandle) + GetSourceOffset(TargetingHandle);
		const FQuat SourceRotation = (GetSourceRotation(TargetingHandle) * GetSourceRotationOffset(TargetingHandle)).GetNormalized();

		TArray<FOverlapResult> OverlapResults;
		if (ShapeType == EVigilTargetingShape::SourceComponent)
		{
			if (const UPrimitiveComponent* CollisionComponent = GetCollisionComponent(TargetingHandle))
			{
				FComponentQueryParams ComponentQueryParams(SCENE_QUERY_STAT(UVigilTargetingSelectionTask_AOE_Component));
				InitCollisionParams(TargetingHandle, ComponentQueryParams);
				World->ComponentOverlapMulti(OverlapResults, CollisionComponent, CollisionComponent->GetComponentLocation(), CollisionComponent->GetComponentRotation(), ComponentQueryParams);
			}
			else
			{
				UE_LOG(LogVigilTargetingSystem, Warning, TEXT("UVigilTargetingSelectionTask_AOE::Execute - Failed to find a collision component w/ tag [%s] for a SourceComponent ShapeType."), *ComponentTag.ToString());
			}
		}
		else
		{
			const FCollisionShape CollisionShape = GetCollisionShape();
			FCollisionQueryParams OverlapParams(TEXT("UVigilTargetingSelectionTask_AOE"), SCENE_QUERY_STAT_ONLY(UVigilTargetingSelectionTask_AOE), false);
			InitCollisionParams(TargetingHandle, OverlapParams);

			if (CollisionObjectTypes.Num() > 0)
			{
				FCollisionObjectQueryParams ObjectParams;
				for (auto Iter = CollisionObjectTypes.CreateConstIterator(); Iter; ++Iter)
				{
					const ECollisionChannel& Channel = UCollisionProfile::Get()->ConvertToCollisionChannel(false, *Iter);
					ObjectParams.AddObjectTypesToQuery(Channel);
				}

				World->OverlapMultiByObjectType(OverlapResults, SourceLocation, SourceRotation, ObjectParams, CollisionShape, OverlapParams);
			}
			else if (CollisionProfileName.Name != TEXT("NoCollision"))
			{
				World->OverlapMultiByProfile(OverlapResults, SourceLocation, SourceRotation, CollisionProfileName.Name, CollisionShape, OverlapParams);
			}
			else
			{
				World->OverlapMultiByChannel(OverlapResults, SourceLocation, SourceRotation, CollisionChannel, CollisionShape, OverlapParams);
			}

#if UE_ENABLE_DEBUG_DRAWING
			if (FVigilCVars::bVigilSelectionDebug)
			{
				DebugDrawBoundingVolume(TargetingHandle, FColor::Red);
			}
#endif
		}

		ProcessOverlapResults(TargetingHandle, OverlapResults);
	}

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}

void UVigilTargetingSelectionTask::ExecuteAsyncTrace(const FTargetingRequestHandle& TargetingHandle) const
{
	UWorld* World = GetSourceContextWorld(TargetingHandle);
	if (World && TargetingHandle.IsValid())
	{
		const FVector SourceLocation = GetSourceLocation(TargetingHandle) + GetSourceOffset(TargetingHandle);
		const FQuat SourceRotation = (GetSourceRotation(TargetingHandle) * GetSourceRotationOffset(TargetingHandle)).GetNormalized();

		const FCollisionShape CollisionShape = GetCollisionShape();
		FCollisionQueryParams OverlapParams(TEXT("UVigilTargetingSelectionTask_AOE"), SCENE_QUERY_STAT_ONLY(UVigilTargetingSelectionTask_AOE_Shape), false);
		InitCollisionParams(TargetingHandle, OverlapParams);

		const FOverlapDelegate Delegate = FOverlapDelegate::CreateUObject(this, &UVigilTargetingSelectionTask::HandleAsyncOverlapComplete, TargetingHandle);
		if (CollisionObjectTypes.Num() > 0)
		{
			FCollisionObjectQueryParams ObjectParams;
			for (auto Iter = CollisionObjectTypes.CreateConstIterator(); Iter; ++Iter)
			{
				const ECollisionChannel& Channel = UCollisionProfile::Get()->ConvertToCollisionChannel(false, *Iter);
				ObjectParams.AddObjectTypesToQuery(Channel);
			}

			World->AsyncOverlapByObjectType(SourceLocation, SourceRotation, ObjectParams, CollisionShape, OverlapParams, &Delegate);
		}
		else if (CollisionProfileName.Name != TEXT("NoCollision"))
		{
			World->AsyncOverlapByProfile(SourceLocation, SourceRotation, CollisionProfileName.Name, CollisionShape, OverlapParams, &Delegate);
		}
		else
		{
			World->AsyncOverlapByChannel(SourceLocation, SourceRotation, CollisionChannel, CollisionShape, OverlapParams, FCollisionResponseParams::DefaultResponseParam, &Delegate);
		}
	}
	else
	{
		SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
	}
}

void UVigilTargetingSelectionTask::HandleAsyncOverlapComplete(const FTraceHandle& InTraceHandle,
	FOverlapDatum& InOverlapDatum, FTargetingRequestHandle TargetingHandle) const
{
	if (TargetingHandle.IsValid())
	{
#if UE_ENABLE_DEBUG_DRAWING
		ResetDebugString(TargetingHandle);

		if (FVigilCVars::bVigilSelectionDebug)
		{
			const FColor& DebugColor = InOverlapDatum.OutOverlaps.Num() > 0 ? FColor::Red : FColor::Green;
			DebugDrawBoundingVolume(TargetingHandle, DebugColor, &InOverlapDatum);
		}
#endif

		ProcessOverlapResults(TargetingHandle, InOverlapDatum.OutOverlaps);
	}

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}

void UVigilTargetingSelectionTask::ProcessOverlapResults(const FTargetingRequestHandle& TargetingHandle,
	const TArray<FOverlapResult>& Overlaps) const
{
	// process the overlaps
	if (Overlaps.Num() > 0)
	{
		FTargetingDefaultResultsSet& TargetingResults = FTargetingDefaultResultsSet::FindOrAdd(TargetingHandle);
		const FVector SourceLocation = GetSourceLocation(TargetingHandle) + GetSourceOffset(TargetingHandle);
		const FQuat SourceRotation = (GetSourceRotation(TargetingHandle) * GetSourceRotationOffset(TargetingHandle)).GetNormalized();

		for (const FOverlapResult& OverlapResult : Overlaps)
		{
			if (!OverlapResult.GetActor())
			{
				continue;
			}

			// cylinders use box overlaps, so a radius check is necessary to constrain it to the bounds of a cylinder
			if (ShapeType == EVigilTargetingShape::Cylinder)
			{
				const float RadiusSquared = (HalfExtent.X * HalfExtent.X);
				const float DistanceSquared = FVector::DistSquared2D(OverlapResult.GetActor()->GetActorLocation(), SourceLocation);
				if (DistanceSquared > RadiusSquared)
				{
					continue;
				}
			}

			// cone use box overlaps, so a length and angle check is necessary to constrain it to the bounds of a cone
			if (ShapeType == EVigilTargetingShape::Cone)
			{
				FVector TargetLocation = OverlapResult.GetActor()->GetActorLocation();
				switch (ConeTargetSource)
				{
				case EVigilConeTargetLocationSource::Component:
					if (OverlapResult.GetComponent())
					{
						TargetLocation = OverlapResult.GetComponent()->GetComponentLocation();
					}
					break;
				case EVigilConeTargetLocationSource::Actor:
					break;
				case EVigilConeTargetLocationSource::TraceMesh:
					{
						if (OverlapResult.GetComponent())
						{
							TargetLocation = OverlapResult.GetComponent()->GetComponentLocation();
						}

						FCollisionQueryParams Params(TEXT("UVigilTargetingSelectionTask_AOE_ConeTargetMesh"),
							SCENE_QUERY_STAT_ONLY(UVigilTargetingSelectionTask_AOE_ConeTargetMesh), true);
						InitCollisionParams(TargetingHandle, Params);
						Params.bTraceComplex = true;
						
						const UWorld* World = GetSourceContextWorld(TargetingHandle);
						FHitResult Hit;
						if (World->LineTraceSingleByChannel(Hit, SourceLocation, TargetLocation, ConeTargetCollisionChannel, Params))
						{
							if (Hit.bBlockingHit)  // We probably don't care about start penetrating?
							{
								TargetLocation = Hit.ImpactPoint;
							}
						}
					}
					break;
				}

				const FVector SourceDirection = SourceRotation.Vector();
				if (!GetConeShape().IsPointWithinCone(TargetLocation, SourceLocation, SourceDirection))
				{
					continue;
				}}

			bool bAddResult = true;
			for (const FTargetingDefaultResultData& ResultData : TargetingResults.TargetResults)
			{
				if (ResultData.HitResult.GetActor() == OverlapResult.GetActor())
				{
					bAddResult = false;
					break;
				}
			}

			if (bAddResult)
			{
				FTargetingDefaultResultData* ResultData = new(TargetingResults.TargetResults) FTargetingDefaultResultData();
				ResultData->HitResult.HitObjectHandle = OverlapResult.OverlapObjectHandle;
				ResultData->HitResult.Component = OverlapResult.GetComponent();
				ResultData->HitResult.ImpactPoint = OverlapResult.GetActor()->GetActorLocation();
				ResultData->HitResult.Location = OverlapResult.GetActor()->GetActorLocation();
				ResultData->HitResult.bBlockingHit = OverlapResult.bBlockingHit;
				ResultData->HitResult.TraceStart = SourceLocation;
				ResultData->HitResult.Item = OverlapResult.ItemIndex;
				ResultData->HitResult.Distance = FVector::Distance(OverlapResult.GetActor()->GetActorLocation(), SourceLocation);

				// Store the normal based on where we are looking based on source rotation
				ResultData->HitResult.Normal = SourceRotation.Vector();

				// Store the trace radius into Time based on the shape type
				switch (ShapeType)
				{
				case EVigilTargetingShape::Cone:
					ResultData->HitResult.Time = FMath::Max(GetConeShape().AngleHeight, GetConeShape().AngleWidth);
					break;
				case EVigilTargetingShape::Box:
				case EVigilTargetingShape::Cylinder:
					ResultData->HitResult.Time = FMath::Max3(HalfExtent.X, HalfExtent.Y, HalfExtent.Z);
					break;
				case EVigilTargetingShape::Sphere:
					ResultData->HitResult.Time = Radius.GetValue();
					break;
				case EVigilTargetingShape::Capsule:
					ResultData->HitResult.Time = HalfHeight.GetValue();
					break;
				case EVigilTargetingShape::SourceComponent:
					ResultData->HitResult.Time = Radius.GetValue();
					break;
				}

				// The default extent is twice the size of a shape, so we need to divide it by 2
				ResultData->HitResult.Time *= 0.5f;

				// Store the max distance into PenetrationDepth based on the shape type
				switch (ShapeType)
				{
				case EVigilTargetingShape::Cone:
					ResultData->HitResult.PenetrationDepth = GetConeShape().Length;
					break;
				case EVigilTargetingShape::Box:
				case EVigilTargetingShape::Cylinder:
					ResultData->HitResult.PenetrationDepth = FMath::Max3(HalfExtent.X, HalfExtent.Y, HalfExtent.Z);
					break;
				case EVigilTargetingShape::Sphere:
					ResultData->HitResult.PenetrationDepth = Radius.GetValue();
					break;
				case EVigilTargetingShape::Capsule:
					ResultData->HitResult.PenetrationDepth = HalfHeight.GetValue();
					break;
				case EVigilTargetingShape::SourceComponent:
					ResultData->HitResult.PenetrationDepth = Radius.GetValue();
					break;
				}
				ResultData->HitResult.PenetrationDepth *= 0.5f;
			}
		}

#if UE_ENABLE_DEBUG_DRAWING
		BuildDebugString(TargetingHandle, TargetingResults.TargetResults);
#endif
	}
}

FCollisionShape UVigilTargetingSelectionTask::GetCollisionShape() const
{
	switch (ShapeType)
	{
	case EVigilTargetingShape::Cone: return GetConeShape().GetConeBoxShape();
	case EVigilTargetingShape::Box:	return FCollisionShape::MakeBox(HalfExtent);
	case EVigilTargetingShape::Cylinder: return FCollisionShape::MakeBox(HalfExtent);
	case EVigilTargetingShape::Sphere: return FCollisionShape::MakeSphere(Radius.GetValue());
	case EVigilTargetingShape::Capsule:
		return FCollisionShape::MakeCapsule(Radius.GetValue(), HalfHeight.GetValue());
	default: return {};
	}
}

const UPrimitiveComponent* UVigilTargetingSelectionTask::GetCollisionComponent(
	const FTargetingRequestHandle& TargetingHandle) const
{
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (SourceContext->SourceActor)
		{
			TArray<UPrimitiveComponent*> PrimitiveComponents;
			SourceContext->SourceActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

			for (const UPrimitiveComponent* Component : PrimitiveComponents)
			{
				if (Component && Component->ComponentHasTag(ComponentTag))
				{
					return Component;
				}
			}
		}
	}

	return nullptr;
}

void UVigilTargetingSelectionTask::InitCollisionParams(const FTargetingRequestHandle& TargetingHandle,
	FCollisionQueryParams& OutParams) const
{
	UVigilTargetingStatics::InitCollisionParams(TargetingHandle, OutParams, bIgnoreSourceActor,
		bIgnoreInstigatorActor, bTraceComplex);
}

void UVigilTargetingSelectionTask::DebugDrawBoundingVolume(const FTargetingRequestHandle& TargetingHandle,
	const FColor& Color, const FOverlapDatum* OverlapDatum) const
{
#if UE_ENABLE_DEBUG_DRAWING
	const UWorld* World = GetSourceContextWorld(TargetingHandle);
	const FVector SourceLocation = OverlapDatum ? OverlapDatum->Pos : GetSourceLocation(TargetingHandle) + GetSourceOffset(TargetingHandle);
	const FQuat SourceRotation = OverlapDatum ? OverlapDatum->Rot : (GetSourceRotation(TargetingHandle) * GetSourceRotationOffset(TargetingHandle)).GetNormalized();
	const FCollisionShape CollisionShape = GetCollisionShape();

	constexpr bool bPersistentLines = false;
	const float LifeTime = UTargetingSubsystem::GetOverrideTargetingLifeTime();
	constexpr uint8 DepthPriority = 0;
	constexpr float Thickness = 2.0f;

	switch (ShapeType)
	{
	case EVigilTargetingShape::Cone:
		UVigilStatics::DrawVigilDebugCone(World, SourceLocation, SourceRotation.Rotator(), GetConeShape(),
			Color, 16, LifeTime, Thickness);
		break;
	case EVigilTargetingShape::Box:
		DrawDebugBox(World, SourceLocation, CollisionShape.GetExtent(), SourceRotation,
			Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		break;
	case EVigilTargetingShape::Sphere:
		DrawDebugCapsule(World, SourceLocation, CollisionShape.GetSphereRadius(), CollisionShape.GetSphereRadius(), SourceRotation,
			Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		break;
	case EVigilTargetingShape::Capsule:
		DrawDebugCapsule(World, SourceLocation, CollisionShape.GetCapsuleHalfHeight(), CollisionShape.GetCapsuleRadius(), SourceRotation,
			Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		break;
	case EVigilTargetingShape::Cylinder:
		{
			const FVector RotatedExtent = SourceRotation * CollisionShape.GetExtent();
			DrawDebugCylinder(World, SourceLocation - RotatedExtent, SourceLocation + RotatedExtent, CollisionShape.GetExtent().X, 32,
				Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
			break;
		}
	case EVigilTargetingShape::SourceComponent:
		break;
	}
#endif
}

#if UE_ENABLE_DEBUG_DRAWING
void UVigilTargetingSelectionTask::DrawDebug(UTargetingSubsystem* TargetingSubsystem, FTargetingDebugInfo& Info,
	const FTargetingRequestHandle& TargetingHandle, float XOffset, float YOffset, int32 MinTextRowsToAdvance) const
{
#if WITH_EDITORONLY_DATA
	if (FVigilCVars::bVigilSelectionDebug)
	{
		FTargetingDebugData& DebugData = FTargetingDebugData::FindOrAdd(TargetingHandle);
		const FString& ScratchPadString = DebugData.DebugScratchPadStrings.FindOrAdd(GetNameSafe(this));
		if (!ScratchPadString.IsEmpty())
		{
			if (Info.Canvas)
			{
				Info.Canvas->SetDrawColor(FColor::Yellow);
			}

			const FString TaskString = FString::Printf(TEXT("Results : %s"), *ScratchPadString);
			TargetingSubsystem->DebugLine(Info, TaskString, XOffset, YOffset, MinTextRowsToAdvance);
		}
	}
#endif
}

void UVigilTargetingSelectionTask::BuildDebugString(const FTargetingRequestHandle& TargetingHandle,
	const TArray<FTargetingDefaultResultData>& TargetResults) const
{
#if WITH_EDITORONLY_DATA
	if (FVigilCVars::bVigilSelectionDebug)
	{
		FTargetingDebugData& DebugData = FTargetingDebugData::FindOrAdd(TargetingHandle);
		FString& ScratchPadString = DebugData.DebugScratchPadStrings.FindOrAdd(GetNameSafe(this));

		for (const FTargetingDefaultResultData& TargetData : TargetResults)
		{
			if (const AActor* Target = TargetData.HitResult.GetActor())
			{
				if (ScratchPadString.IsEmpty())
				{
					ScratchPadString = FString::Printf(TEXT("%s"), *GetNameSafe(Target));
				}
				else
				{
					ScratchPadString += FString::Printf(TEXT(", %s"), *GetNameSafe(Target));
				}
			}
		}
	}
#endif
}

void UVigilTargetingSelectionTask::ResetDebugString(const FTargetingRequestHandle& TargetingHandle) const
{
#if WITH_EDITORONLY_DATA
	FTargetingDebugData& DebugData = FTargetingDebugData::FindOrAdd(TargetingHandle);
	FString& ScratchPadString = DebugData.DebugScratchPadStrings.FindOrAdd(GetNameSafe(this));
	ScratchPadString.Reset();
#endif
}
#endif
