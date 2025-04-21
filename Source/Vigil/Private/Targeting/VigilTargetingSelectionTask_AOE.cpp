// Copyright (c) Jared Taylor. All Rights Reserved


#include "Targeting/VigilTargetingSelectionTask_AOE.h"

#include "VigilStatics.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Engine/OverlapResult.h"

#if UE_ENABLE_DEBUG_DRAWING
#include "Engine/Canvas.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogVigilTargetingSystem, Log, All);

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilTargetingSelectionTask_AOE)

UVigilTargetingSelectionTask_AOE::UVigilTargetingSelectionTask_AOE(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CollisionChannel = ECC_Visibility;
	ConeTargetCollisionChannel = ECC_Visibility;
	bUseRelativeLocationOffset = true;
	bIgnoreSourceActor = true;
	bIgnoreInstigatorActor = false;
	bTraceComplex = false;

	LocationSource = EVigilTargetLocationSource_AOE::Camera;
	RotationSource = EVigilTargetRotationSource_AOE::ViewRotation;
	ConeTargetSource = EVigilConeTargetLocationSource_AOE::Component;

	ConeLength = 1800.f;
	ConeAngleWidth = 25.f;
	ConeAngleHeight = 35.f;
}

FVector UVigilTargetingSelectionTask_AOE::GetSourceLocation_Implementation(
	const FTargetingRequestHandle& TargetingHandle) const
{
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (SourceContext->SourceActor)
		{
			switch (LocationSource)
			{
			case EVigilTargetLocationSource_AOE::Actor: return SourceContext->SourceActor->GetActorLocation();
			case EVigilTargetLocationSource_AOE::ViewLocation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetPawnViewLocation();
					}
				}
			case EVigilTargetLocationSource_AOE::Camera:
				{
					if (const APlayerController* PC = Cast<APlayerController>(SourceContext->SourceActor->GetOwner()))
					{
						if (PC->PlayerCameraManager)
						{
							return PC->PlayerCameraManager->GetCameraLocation();
						}
					}
				}
				break;
			}
		}
	}
	return FVector::ZeroVector;
}

FVector UVigilTargetingSelectionTask_AOE::GetSourceOffset_Implementation(
	const FTargetingRequestHandle& TargetingHandle) const
{
	if (!bUseRelativeLocationOffset)
	{
		return DefaultSourceLocationOffset;
	}

	if (DefaultSourceLocationOffset.IsZero())
	{
		return FVector::ZeroVector;
	}
	
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (SourceContext->SourceActor)
		{
			switch (LocationSource)
			{
			case EVigilTargetLocationSource_AOE::Actor: return SourceContext->SourceActor->GetActorRotation().RotateVector(DefaultSourceLocationOffset);
			case EVigilTargetLocationSource_AOE::ViewLocation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetViewRotation().RotateVector(DefaultSourceLocationOffset);
					}
				}
			case EVigilTargetLocationSource_AOE::Camera:
				{
					if (const APlayerController* PC = Cast<APlayerController>(SourceContext->SourceActor->GetOwner()))
					{
						if (PC->PlayerCameraManager)
						{
							return PC->PlayerCameraManager->GetCameraRotation().RotateVector(DefaultSourceLocationOffset);
						}
					}
				}
				break;
			}
		}
	}
	return FVector::ZeroVector;
}

FQuat UVigilTargetingSelectionTask_AOE::GetSourceRotation_Implementation(
	const FTargetingRequestHandle& TargetingHandle) const
{
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (SourceContext->SourceActor)
		{
			switch (RotationSource)
			{
			case EVigilTargetRotationSource_AOE::Actor: return SourceContext->SourceActor->GetActorQuat();
			case EVigilTargetRotationSource_AOE::ControlRotation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetControlRotation().Quaternion();
					}
				}
			case EVigilTargetRotationSource_AOE::ViewRotation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetViewRotation().Quaternion();
					}
				}
				break;
			}
		}
	}
	return FQuat::Identity;
}

FQuat UVigilTargetingSelectionTask_AOE::GetSourceRotationOffset_Implementation(
	const FTargetingRequestHandle& TargetingHandle) const
{
	return DefaultSourceRotationOffset.Quaternion();
}

void UVigilTargetingSelectionTask_AOE::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
	Super::Execute(TargetingHandle);
	
	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

	// @note: There isn't Async Overlap support based on Primitive Component, so even if using async targeting, it will
	// run this task in "immediate" mode.
	if (IsAsyncTargetingRequest(TargetingHandle) && (ShapeType != EVigilTargetingShape_AOE::SourceComponent))
	{
		ExecuteAsyncTrace(TargetingHandle);
	}
	else
	{
		ExecuteImmediateTrace(TargetingHandle);
	}
}

void UVigilTargetingSelectionTask_AOE::ExecuteImmediateTrace(const FTargetingRequestHandle& TargetingHandle) const
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
		if (ShapeType == EVigilTargetingShape_AOE::SourceComponent)
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
			if (UTargetingSubsystem::IsTargetingDebugEnabled())
			{
				DebugDrawBoundingVolume(TargetingHandle, FColor::Red);
			}
#endif
		}

		ProcessOverlapResults(TargetingHandle, OverlapResults);
	}

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}

void UVigilTargetingSelectionTask_AOE::ExecuteAsyncTrace(const FTargetingRequestHandle& TargetingHandle) const
{
	UWorld* World = GetSourceContextWorld(TargetingHandle);
	if (World && TargetingHandle.IsValid())
	{
		const FVector SourceLocation = GetSourceLocation(TargetingHandle) + GetSourceOffset(TargetingHandle);
		const FQuat SourceRotation = (GetSourceRotation(TargetingHandle) * GetSourceRotationOffset(TargetingHandle)).GetNormalized();

		const FCollisionShape CollisionShape = GetCollisionShape();
		FCollisionQueryParams OverlapParams(TEXT("UVigilTargetingSelectionTask_AOE"), SCENE_QUERY_STAT_ONLY(UVigilTargetingSelectionTask_AOE_Shape), false);
		InitCollisionParams(TargetingHandle, OverlapParams);

		const FOverlapDelegate Delegate = FOverlapDelegate::CreateUObject(this, &UVigilTargetingSelectionTask_AOE::HandleAsyncOverlapComplete, TargetingHandle);
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

void UVigilTargetingSelectionTask_AOE::HandleAsyncOverlapComplete(const FTraceHandle& InTraceHandle,
	FOverlapDatum& InOverlapDatum, FTargetingRequestHandle TargetingHandle) const
{
	if (TargetingHandle.IsValid())
	{
#if UE_ENABLE_DEBUG_DRAWING
		ResetDebugString(TargetingHandle);

		if (UTargetingSubsystem::IsTargetingDebugEnabled())
		{
			const FColor& DebugColor = InOverlapDatum.OutOverlaps.Num() > 0 ? FColor::Red : FColor::Green;
			DebugDrawBoundingVolume(TargetingHandle, DebugColor, &InOverlapDatum);
		}
#endif

		ProcessOverlapResults(TargetingHandle, InOverlapDatum.OutOverlaps);
	}

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}

void UVigilTargetingSelectionTask_AOE::ProcessOverlapResults(const FTargetingRequestHandle& TargetingHandle,
	const TArray<FOverlapResult>& Overlaps) const
{
	// process the overlaps
	if (Overlaps.Num() > 0)
	{
		FTargetingDefaultResultsSet& TargetingResults = FTargetingDefaultResultsSet::FindOrAdd(TargetingHandle);
		const FVector SourceLocation = GetSourceLocation(TargetingHandle) + GetSourceOffset(TargetingHandle);
		for (const FOverlapResult& OverlapResult : Overlaps)
		{
			if (!OverlapResult.GetActor())
			{
				continue;
			}

			// cylinders use box overlaps, so a radius check is necessary to constrain it to the bounds of a cylinder
			if (ShapeType == EVigilTargetingShape_AOE::Cylinder)
			{
				const float RadiusSquared = (HalfExtent.X * HalfExtent.X);
				const float DistanceSquared = FVector::DistSquared2D(OverlapResult.GetActor()->GetActorLocation(), SourceLocation);
				if (DistanceSquared > RadiusSquared)
				{
					continue;
				}
			}

			// cone use box overlaps, so a length and angle check is necessary to constrain it to the bounds of a cone
			if (ShapeType == EVigilTargetingShape_AOE::Cone)
			{
				FVector TargetLocation = OverlapResult.GetActor()->GetActorLocation();
				switch (ConeTargetSource)
				{
				case EVigilConeTargetLocationSource_AOE::Component:
					if (OverlapResult.GetComponent())
					{
						TargetLocation = OverlapResult.GetComponent()->GetComponentLocation();
					}
					break;
				case EVigilConeTargetLocationSource_AOE::Actor:
					break;
				case EVigilConeTargetLocationSource_AOE::TraceMesh:
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

				const FQuat SourceRotation = (GetSourceRotation(TargetingHandle) * GetSourceRotationOffset(TargetingHandle)).GetNormalized();
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
			}
		}

#if UE_ENABLE_DEBUG_DRAWING
		BuildDebugString(TargetingHandle, TargetingResults.TargetResults);
#endif
	}
}

FCollisionShape UVigilTargetingSelectionTask_AOE::GetCollisionShape() const
{
	switch (ShapeType)
	{
	case EVigilTargetingShape_AOE::Cone: return GetConeShape().GetConeBoxShape();
	case EVigilTargetingShape_AOE::Box:	return FCollisionShape::MakeBox(HalfExtent);
	case EVigilTargetingShape_AOE::Cylinder: return FCollisionShape::MakeBox(HalfExtent);
	case EVigilTargetingShape_AOE::Sphere: return FCollisionShape::MakeSphere(Radius.GetValue());
	case EVigilTargetingShape_AOE::Capsule:
		return FCollisionShape::MakeCapsule(Radius.GetValue(), HalfHeight.GetValue());
	default: return {};
	}
}

const UPrimitiveComponent* UVigilTargetingSelectionTask_AOE::GetCollisionComponent(
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

void UVigilTargetingSelectionTask_AOE::InitCollisionParams(const FTargetingRequestHandle& TargetingHandle,
	FCollisionQueryParams& OutParams) const
{
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (bIgnoreSourceActor && SourceContext->SourceActor)
		{
			OutParams.AddIgnoredActor(SourceContext->SourceActor);
		}

		if (bIgnoreInstigatorActor && SourceContext->InstigatorActor)
		{
			OutParams.AddIgnoredActor(SourceContext->InstigatorActor);
		}
	}

	// This isn't exported, we can't get to it...
	
	// // If we have a data store to override the collision query params from, add that to our collision params
	// if (const FCollisionQueryTaskData* FoundOverride = UE::TargetingSystem::TTargetingDataStore<FCollisionQueryTaskData>::Find(TargetingHandle))
	// {
	// 	OutParams.AddIgnoredActors(FoundOverride->IgnoredActors);
	// }

	// Get the cvar for complex tracing
	static const auto CVarComplexTracingAOE = IConsoleManager::Get().FindConsoleVariable(TEXT("ts.AOE.EnableComplexTracingAOE"));
	const bool bComplexTracingAOE = CVarComplexTracingAOE ? CVarComplexTracingAOE->GetBool() : true;
	
	OutParams.bTraceComplex = bComplexTracingAOE && bTraceComplex;
}

void UVigilTargetingSelectionTask_AOE::DebugDrawBoundingVolume(const FTargetingRequestHandle& TargetingHandle,
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
	case EVigilTargetingShape_AOE::Cone:
		UVigilStatics::DrawVigilDebugCone(World, SourceLocation, SourceRotation.Rotator(), GetConeShape(),
			Color, 16, LifeTime, Thickness);
		break;
	case EVigilTargetingShape_AOE::Box:
		DrawDebugBox(World, SourceLocation, CollisionShape.GetExtent(), SourceRotation,
			Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		break;
	case EVigilTargetingShape_AOE::Sphere:
		DrawDebugCapsule(World, SourceLocation, CollisionShape.GetSphereRadius(), CollisionShape.GetSphereRadius(), SourceRotation,
			Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		break;
	case EVigilTargetingShape_AOE::Capsule:
		DrawDebugCapsule(World, SourceLocation, CollisionShape.GetCapsuleHalfHeight(), CollisionShape.GetCapsuleRadius(), SourceRotation,
			Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		break;
	case EVigilTargetingShape_AOE::Cylinder:
		{
			const FVector RotatedExtent = SourceRotation * CollisionShape.GetExtent();
			DrawDebugCylinder(World, SourceLocation - RotatedExtent, SourceLocation + RotatedExtent, CollisionShape.GetExtent().X, 32,
				Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
			break;
		}
	case EVigilTargetingShape_AOE::SourceComponent:
		break;
	}
#endif
}

#if UE_ENABLE_DEBUG_DRAWING
void UVigilTargetingSelectionTask_AOE::DrawDebug(UTargetingSubsystem* TargetingSubsystem, FTargetingDebugInfo& Info,
	const FTargetingRequestHandle& TargetingHandle, float XOffset, float YOffset, int32 MinTextRowsToAdvance) const
{
#if WITH_EDITORONLY_DATA
	if (UTargetingSubsystem::IsTargetingDebugEnabled())
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

void UVigilTargetingSelectionTask_AOE::BuildDebugString(const FTargetingRequestHandle& TargetingHandle,
	const TArray<FTargetingDefaultResultData>& TargetResults) const
{
#if WITH_EDITORONLY_DATA
	if (UTargetingSubsystem::IsTargetingDebugEnabled())
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

void UVigilTargetingSelectionTask_AOE::ResetDebugString(const FTargetingRequestHandle& TargetingHandle) const
{
#if WITH_EDITORONLY_DATA
	FTargetingDebugData& DebugData = FTargetingDebugData::FindOrAdd(TargetingHandle);
	FString& ScratchPadString = DebugData.DebugScratchPadStrings.FindOrAdd(GetNameSafe(this));
	ScratchPadString.Reset();
#endif
}
#endif
