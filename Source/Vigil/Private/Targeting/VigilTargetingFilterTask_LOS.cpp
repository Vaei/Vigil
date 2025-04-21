// Copyright (c) Jared Taylor


#include "Targeting/VigilTargetingFilterTask_LOS.h"

#include "Targeting/VigilTargetingStatics.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Actor.h"
#include "CollisionShape.h"
#include "Engine/World.h"
#include "System/VigilVersioning.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilTargetingFilterTask_LOS)

namespace FVigilCVars
{
#if UE_ENABLE_DEBUG_DRAWING
	static bool bVigilFilterDebug = false;
	FAutoConsoleVariableRef CVarVigilFilterDebug(
		TEXT("p.Vigil.Filter.Debug"),
		bVigilFilterDebug,
		TEXT("Optionally draw debug for Vigil LOS Filter Task.\n")
		TEXT("If true draw debug for Vigil LOS Filter Task"),
		ECVF_Default);
#endif
}

UVigilTargetingFilterTask_LOS::UVigilTargetingFilterTask_LOS(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CollisionChannel = ECC_Visibility;
	LocationSource = EVigilTargetLocationSource::ViewLocation;
	TargetLocationSource = EVigilTargetLocationSource_LOS::BoundsOrigin;

	bIgnoreSourceActor = true;
	bIgnoreInstigatorActor = false;
	bIgnoreMovableTargets = true;

	bUseRelativeLocationOffset = true;
}

bool UVigilTargetingFilterTask_LOS::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	const UWorld* World = GetSourceContextWorld(TargetingHandle);
	if (World && TargetingHandle.IsValid())
	{
		const AActor* TargetActor = TargetData.HitResult.GetActor();
		if (!TargetActor)
		{
			return true;
		}
		const FVector SourceLocation = GetSourceLocation(TargetingHandle) + GetSourceOffset(TargetingHandle);
		FCollisionQueryParams TraceParams(TEXT("UVigilTargetingFilterTask_LOS"), SCENE_QUERY_STAT_ONLY(UVigilTargetingFilterTask_LOS), false);
		InitCollisionParams(TargetingHandle, TraceParams);

		const bool bMovableTarget = bIgnoreMovableTargets && TargetActor->IsRootComponentMovable();
		if (bMovableTarget)
		{
			TraceParams.AddIgnoredActor(TargetActor);
		}

		FVector TargetLocation = TargetActor->GetActorLocation();
		if (TargetLocationSource == EVigilTargetLocationSource_LOS::BoundsOrigin)
		{
			FVector NotUsed;
			TargetActor->GetActorBounds(true, TargetLocation, NotUsed);
		}
		
		FHitResult Hit;
		const bool bSphereTrace = TraceRadius > 0.f;
		const FCollisionShape Sphere = FCollisionShape::MakeSphere(TraceRadius);
		
		if (CollisionObjectTypes.Num() > 0)
		{
			FCollisionObjectQueryParams ObjectParams;
			for (auto Iter = CollisionObjectTypes.CreateConstIterator(); Iter; ++Iter)
			{
				const ECollisionChannel& Channel = UCollisionProfile::Get()->ConvertToCollisionChannel(false, *Iter);
				ObjectParams.AddObjectTypesToQuery(Channel);
			}

			if (bSphereTrace)
			{
				World->SweepSingleByObjectType(Hit, SourceLocation, TargetLocation, FQuat::Identity,
					ObjectParams, Sphere, TraceParams);
			}
			else
			{
				World->LineTraceSingleByObjectType(Hit, SourceLocation, TargetLocation, ObjectParams,
					TraceParams);
			}
		}
		else if (CollisionProfileName.Name != TEXT("NoCollision"))
		{
			if (bSphereTrace)
			{
				World->SweepSingleByProfile(Hit, SourceLocation, TargetLocation, FQuat::Identity,
					CollisionProfileName.Name, Sphere, TraceParams);
			}
			else
			{
				World->LineTraceSingleByProfile(Hit, SourceLocation, TargetLocation,
					CollisionProfileName.Name, TraceParams);
			}
		}
		else
		{
			if (bSphereTrace)
			{
				World->SweepSingleByChannel(Hit, SourceLocation, TargetLocation, FQuat::Identity,
					CollisionChannel, Sphere, TraceParams);
			}
			else
			{
				World->LineTraceSingleByChannel(Hit, SourceLocation, TargetLocation, CollisionChannel,
					TraceParams);
			}
		}

#if UE_ENABLE_DEBUG_DRAWING
		if (FVigilCVars::bVigilFilterDebug)
		{
			const FColor& DebugColor = Hit.bBlockingHit ? FColor::Red : FColor::Green;
			if (bSphereTrace)
			{
				DrawDebugSphere(World, Hit.ImpactPoint, TraceRadius, 12, DebugColor, false,
					UTargetingSubsystem::GetOverrideTargetingLifeTime());
			}
			else
			{
				DrawDebugLine(World, SourceLocation, Hit.ImpactPoint, DebugColor, false,
					UTargetingSubsystem::GetOverrideTargetingLifeTime(), 0, 1.f);
			}
		}
#endif

		// Note -- we filter out if there is no LOS
		// So we return true if we don't have LOS
		
		// If our target is movable, and we hit nothing, then we have line of sight
		if (bMovableTarget)
		{
			// We hit something, so we don't have LOS, we need to filter it
			return Hit.bBlockingHit;
		}

		// We only have line of sight to our target if we hit it, i.e. nothing interfered
		if (Hit.bBlockingHit && Hit.GetActor() == TargetActor)
		{
			return false;
		}
	}

	// We don't have LOS to our target, so we need to filter it out
	return true;
}

void UVigilTargetingFilterTask_LOS::InitCollisionParams(const FTargetingRequestHandle& TargetingHandle,
	FCollisionQueryParams& OutParams) const
{
	UVigilTargetingStatics::InitCollisionParams(TargetingHandle, OutParams, bIgnoreSourceActor,
		bIgnoreInstigatorActor, bTraceComplex);
}

FVector UVigilTargetingFilterTask_LOS::GetSourceLocation_Implementation(
	const FTargetingRequestHandle& TargetingHandle) const
{
	return UVigilTargetingStatics::GetSourceLocation(TargetingHandle, LocationSource);
}

FVector UVigilTargetingFilterTask_LOS::GetSourceOffset_Implementation(
	const FTargetingRequestHandle& TargetingHandle) const
{
	return UVigilTargetingStatics::GetSourceOffset(TargetingHandle, LocationSource, DefaultSourceLocationOffset,
		bUseRelativeLocationOffset);
}
