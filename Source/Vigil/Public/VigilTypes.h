// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "VigilTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVigil, Log, All);

DECLARE_DELEGATE_OneParam(FOnPauseVigil, bool /* bIsPaused */);
DECLARE_DELEGATE(FOnRequestVigil);

UENUM(BlueprintType)
enum class EVigilTargetPresetUpdateMode : uint8
{
	BeginPlay					UMETA(ToolTip="Only update targeting presets on BeginPlay"),
	PawnChanged					UMETA(ToolTip="Update targeting presets when the pawn changes, and on BeginPlay"),
	OnUpdate					UMETA(ToolTip="Update targeting presets every time Vigil updates the focus options"),
};

UENUM(BlueprintType)
enum class EVigilTargetingSource : uint8
{
	Pawn						UMETA(ToolTip="Use the controlled Pawn as the targeting source"),
	PawnIfValid					UMETA(ToolTip="Use the controlled Pawn as the targeting source, or the PlayerController if invalid"),
	PlayerController			UMETA(ToolTip="Use the PlayerController as the targeting source"),
};

USTRUCT(BlueprintType)
struct VIGIL_API FVigilFocusResult
{
	GENERATED_BODY()

	FVigilFocusResult(const FGameplayTag& InFocusTag = FGameplayTag::EmptyTag, const FHitResult& InHitResult = {}, float InScore = 0.f)
		: FocusTag(InFocusTag)
		, HitResult(InHitResult)
		, Score(InScore)
	{}

	UPROPERTY(BlueprintReadOnly, Category=Vigil)
	FGameplayTag FocusTag;
	
	UPROPERTY(BlueprintReadOnly, Category=Vigil)
	FHitResult HitResult;
	
	UPROPERTY(BlueprintReadOnly, Category=Vigil)
	float Score;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVigilComplete, UVigilComponent*, VigilComponent, FGameplayTag, FocusTag, const TArray<FVigilFocusResult>&, Results);
