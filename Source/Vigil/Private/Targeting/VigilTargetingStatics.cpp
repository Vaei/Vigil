// Copyright (c) Jared Taylor


#include "Targeting/VigilTargetingStatics.h"

#include "Targeting/VigilTargetingTypes.h"
#include "Types/TargetingSystemTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PawnMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilTargetingStatics)

FVector UVigilTargetingStatics::GetSourceLocation(const FTargetingRequestHandle& TargetingHandle,
	EVigilTargetLocationSource LocationSource)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilTargetingStatics::GetSourceLocation);
	
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (SourceContext->SourceActor)
		{
			switch (LocationSource)
			{
			case EVigilTargetLocationSource::Actor: return SourceContext->SourceActor->GetActorLocation();
			case EVigilTargetLocationSource::ViewLocation:
				{
					const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor);
					if (!Pawn)
					{
						const APlayerController* PC = Cast<APlayerController>(SourceContext->SourceActor);
						Pawn = PC ? PC->GetPawn() : nullptr;
					}
					if (Pawn)
					{
						return Pawn->GetPawnViewLocation();
					}
				}
			case EVigilTargetLocationSource::Camera:
				{
					const APlayerController* PC = Cast<APlayerController>(SourceContext->SourceActor);
					if (!PC)
					{
						const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor);
						PC = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
					}
					
					if (PC)
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
	EVigilTargetLocationSource LocationSource, FVector DefaultSourceLocationOffset, bool bUseRelativeLocationOffset)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilTargetingStatics::GetSourceOffset);
	
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
			case EVigilTargetLocationSource::Actor: return SourceContext->SourceActor->GetActorRotation().RotateVector(DefaultSourceLocationOffset);
			case EVigilTargetLocationSource::ViewLocation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetViewRotation().RotateVector(DefaultSourceLocationOffset);
					}
				}
			case EVigilTargetLocationSource::Camera:
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

FQuat UVigilTargetingStatics::GetSourceRotation(const FTargetingRequestHandle& TargetingHandle,
	EVigilTargetRotationSource RotationSource, bool& bZeroVector)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilTargetingStatics::GetSourceRotation);
	
	bZeroVector = false;
	
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		if (SourceContext->SourceActor)
		{
			switch (RotationSource)
			{
			case EVigilTargetRotationSource::Actor: return SourceContext->SourceActor->GetActorQuat();
			case EVigilTargetRotationSource::ControlRotation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetControlRotation().Quaternion();
					}
				}
			case EVigilTargetRotationSource::ViewRotation:
				{
					if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
					{
						return Pawn->GetViewRotation().Quaternion();
					}
				}
				break;
			case EVigilTargetRotationSource::InputVector:
				if (const APawn* Pawn = Cast<APawn>(SourceContext->SourceActor))
				{
					const FVector InputVector = Pawn->GetMovementComponent()->GetLastInputVector();
					if (InputVector.IsNearlyZero())
					{
						bZeroVector = true;
					}
					return Pawn->GetMovementComponent()->GetLastInputVector().ToOrientationQuat();
				}
				break;
			case EVigilTargetRotationSource::Velocity:
				{
					const FVector Velocity = SourceContext->SourceActor->GetVelocity();
					if (Velocity.IsNearlyZero())
					{
						bZeroVector = true;
					}
					return SourceContext->SourceActor->GetVelocity().ToOrientationQuat();
				}
			}
		}
	}
	return FQuat::Identity;
}

void UVigilTargetingStatics::InitCollisionParams(const FTargetingRequestHandle& TargetingHandle,
	FCollisionQueryParams& OutParams, bool bIgnoreSourceActor, bool bIgnoreInstigatorActor, bool bTraceComplex)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilTargetingStatics::InitCollisionParams);
	
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