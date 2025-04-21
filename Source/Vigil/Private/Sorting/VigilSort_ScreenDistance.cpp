// Copyright (c) Jared Taylor


#include "Sorting/VigilSort_ScreenDistance.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Targeting/VigilTargetingStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilSort_ScreenDistance)


float UVigilSort_ScreenDistance::GetScoreForTarget_Implementation(const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	if (IsRunningDedicatedServer())
	{
		return 0.f;
	}
	
	const UTargetingSubsystem* TargetSubsystem = GetTargetingSubsystem(TargetingHandle);
	if (APlayerController* PC = TargetSubsystem ? UGameplayStatics::GetPlayerController(TargetSubsystem->GetWorld(), 0) : nullptr)
	{
		// Determine location to project to screen
		FVector WorldLocation = TargetData.HitResult.Location;
		if (LocationSource == EVigilScreenDistanceLocationSource::HitActor)
		{
			WorldLocation = TargetData.HitResult.GetActor()->GetActorLocation();
		}
		else if (LocationSource == EVigilScreenDistanceLocationSource::HitComponent)
		{
			WorldLocation = TargetData.HitResult.GetComponent()->GetComponentLocation();
		}

		// Project to screen
		FVector2D ScreenLocation;
		if (!PC->ProjectWorldLocationToScreen(WorldLocation, ScreenLocation, true))
		{
			return 0.f;
		}
		
		// Get screen size
		const FGeometry Geometry = UWidgetLayoutLibrary::GetViewportWidgetGeometry(PC);
		const FVector2D Size = Geometry.GetAbsoluteSize();
		const FVector2D Center = Size * 0.5f;

		// Calculate distance
		return FVector2D::Distance(Center, ScreenLocation);
	}

	return 0.f;
}
