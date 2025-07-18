﻿// Copyright (c) Jared Taylor


#include "VigilTypes.h"

#include "ScalableFloat.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VigilTypes)

DEFINE_LOG_CATEGORY(LogVigil);

bool FVigilConeShape::IsPointWithinCone(const FVector& Point, const FVector& ConeOrigin,
	const FVector& ConeDirection) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilConeShape::IsPointWithinCone);
	
	const FVector ToPoint = Point - ConeOrigin;

	// Project onto cone's forward vector
	const float ForwardDist = FVector::DotProduct(ToPoint, ConeDirection);
	if (ForwardDist < 0 || ForwardDist > Length)
	{
		// Behind or past the cone
		return false;
	}

	// Local Y/Z axes for angle calc
	FVector Right, Up;
	ConeDirection.FindBestAxisVectors(Up, Right);

	const float RightDist = FVector::DotProduct(ToPoint, Right);
	const float UpDist    = FVector::DotProduct(ToPoint, Up);

	// Convert to angle off cone axis
	const float AWidth  = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(RightDist), ForwardDist));
	const float AHeight = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(UpDist), ForwardDist));

	// Compare against half-angles
	return AWidth <= AngleWidth * 0.5f && AHeight <= AngleHeight * 0.5f;
}

FVector FVigilConeShape::GetConeBoxShapeHalfExtent() const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilConeShape::GetConeBoxShapeHalfExtent);
	
	// Convert to radians
	const float HalfAngleWidth = FMath::DegreesToRadians(0.5f * AngleWidth);
	const float HalfAngleHeight = FMath::DegreesToRadians(0.5f * AngleHeight);

	// Project base radius from angles
	const float RadiusX = FMath::Tan(HalfAngleWidth) * Length;
	const float RadiusY = FMath::Tan(HalfAngleHeight) * Length;

	// Box aligned along X = cone direction
	const FVector HalfExtent(0.5f * Length, RadiusX, RadiusY);
	return HalfExtent;
}

FVigilConeShape FVigilConeShape::MakeConeFromScalableFloat(const FScalableFloat& Length,
	const FScalableFloat& AngleWidth, const FScalableFloat& AngleHeight)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(VigilConeShape::MakeConeFromScalableFloat);
	
	return FVigilConeShape(Length.GetValue(), AngleWidth.GetValue(), AngleHeight.GetValue());
}

FVigilNetSyncDelegateHandler::FVigilNetSyncDelegateHandler(FOnVigilNetSyncCompleted&& InDelegate)
	: Delegate(InDelegate)
	, DelegateHandle(FDelegateHandle::EGenerateNewHandleType::GenerateNewHandle)
	, bRemoved(false)
{
}

FVigilNetSyncDelegateHandler::FVigilNetSyncDelegateHandler(FOnVigilNetSyncCompletedBP&& InDelegate)
	: DelegateBP(InDelegate)
	, DelegateHandle(FDelegateHandle::EGenerateNewHandleType::GenerateNewHandle)
	, bRemoved(false)
{
}

void FVigilNetSyncDelegateHandler::Execute() const
{
	if (bRemoved)
	{
		return;
	}

	if (Delegate.IsBound())
	{
		ensure(!DelegateBP.IsBound());

		Delegate.Execute();
	}
	else if (DelegateBP.IsBound())
	{
		DelegateBP.Execute();
	}
}
