// Copyright (c) Jared Taylor. All Rights Reserved


#include "Targeting/VigilTargetingStatics.h"

#include "Targeting/VigilTargetingTypes.h"
#include "Types/TargetingSystemTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilTargetingStatics)

FVector UVigilTargetingStatics::GetSourceLocation(const FTargetingRequestHandle& TargetingHandle,
	EVigilTargetLocationSource_AOE LocationSource)
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

FVector UVigilTargetingStatics::GetSourceOffset(const FTargetingRequestHandle& TargetingHandle,
	EVigilTargetLocationSource_AOE LocationSource, FVector DefaultSourceLocationOffset, bool bUseRelativeLocationOffset)
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

FQuat UVigilTargetingStatics::GetSourceRotation(const FTargetingRequestHandle& TargetingHandle, EVigilTargetRotationSource_AOE RotationSource)
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

void UVigilTargetingStatics::InitCollisionParams(const FTargetingRequestHandle& TargetingHandle,
	FCollisionQueryParams& OutParams, bool bIgnoreSourceActor, bool bIgnoreInstigatorActor, bool bTraceComplex)
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

	OutParams.bTraceComplex = bTraceComplex;
	
	// This isn't exported, we can't get to it...
	
	// // If we have a data store to override the collision query params from, add that to our collision params
	// if (const FCollisionQueryTaskData* FoundOverride = UE::TargetingSystem::TTargetingDataStore<FCollisionQueryTaskData>::Find(TargetingHandle))
	// {
	// 	OutParams.AddIgnoredActors(FoundOverride->IgnoredActors);
	// }

	// Vigil is its own thing, lets not use the CVar
	
	// static const auto CVarComplexTracingAOE = IConsoleManager::Get().FindConsoleVariable(TEXT("ts.AOE.EnableComplexTracingAOE"));
	// const bool bComplexTracingAOE = CVarComplexTracingAOE ? CVarComplexTracingAOE->GetBool() : true;
}
