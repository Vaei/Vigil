// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "VigilTargetingTypes.generated.h"


UENUM(BlueprintType)
enum class EVigilTargetingShape_AOE : uint8
{
	Cone,
	Box,
	Cylinder,
	Sphere,
	Capsule,
	SourceComponent,
};

UENUM(BlueprintType)
enum class EVigilTargetLocationSource_AOE : uint8
{
	Actor,
	ViewLocation,
	Camera,
};

UENUM(BlueprintType)
enum class EVigilTargetRotationSource_AOE : uint8
{
	Actor,
	ControlRotation,
	ViewRotation,
};

UENUM(BlueprintType)
enum class EVigilConeTargetLocationSource_AOE : uint8
{
	Component				UMETA(ToolTip="Use the component location if the component exists, otherwise actor location"),
	Actor					UMETA(ToolTip="Use the actor location"),
	TraceMesh				UMETA(ToolTip="Complex line trace towards the component if it exists, otherwise the actor, and use the impact point - EXPENSIVE!"),
};

UENUM(BlueprintType)
enum class EVigilTargetLocationSource_LOS : uint8
{
	BoundsOrigin			UMETA(ToolTip="Use the origin of the actor's bounds"),
	Actor					UMETA(ToolTip="Use the actor location"),
};
