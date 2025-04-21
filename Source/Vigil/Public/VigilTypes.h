// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "VigilTypes.generated.h"

struct FScalableFloat;
DECLARE_LOG_CATEGORY_EXTERN(LogVigil, Log, All);

DECLARE_DELEGATE_OneParam(FOnPauseVigil, bool /* bIsPaused */);
DECLARE_DELEGATE(FOnRequestVigil);

UENUM(BlueprintType)
enum class EVigilTargetingSource : uint8
{
	Pawn						UMETA(ToolTip="Use the controlled Pawn as the targeting source"),
	PawnIfValid					UMETA(ToolTip="Use the controlled Pawn as the targeting source, or the PlayerController if invalid"),
	PlayerController			UMETA(ToolTip="Use the PlayerController as the targeting source"),
};

UENUM(BlueprintType)
enum class EVigilNetSyncType : uint8  // Avoid having to include AbilityTask_NetworkSyncPoint.h
{
	/** Both client and server wait until the other signals */
	BothWait,

	/** Only server will wait for the client signal. Client will signal and immediately continue without waiting to hear from Server. */
	OnlyServerWait,

	/** Only client will wait for the server signal. Server will signal and immediately continue without waiting to hear from Client. */
	OnlyClientWait	
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVigilSync, EVigilNetSyncType, SyncType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVigilSyncCompleted);

USTRUCT(BlueprintType)
struct VIGIL_API FVigilConeShape
{
	GENERATED_BODY()

	FVigilConeShape(float InLength = 0.f, float InAngleWidth = 0.f, float InAngleHeight = 0.f)
		: Length(InLength)
		, AngleWidth(InAngleWidth)
		, AngleHeight(InAngleHeight)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Collision, meta=(UIMin="0", ClampMin="0", Delta="0.1", ForceUnits="cm"))
	float Length;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Collision, meta=(UIMin="0", ClampMin="0", UIMax="180", ClampMax="180", Delta="0.1", ForceUnits="Degrees"))
	float AngleWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Collision, meta=(UIMin="0", ClampMin="0", UIMax="180", ClampMax="180", Delta="0.1", ForceUnits="Degrees"))
	float AngleHeight;

	bool IsPointWithinCone(const FVector& Point, const FVector& ConeOrigin, const FVector& ConeDirection) const;

	FVector GetConeBoxShapeHalfExtent() const;

	static FVigilConeShape MakeConeFromScalableFloat(const FScalableFloat& Length, const FScalableFloat& AngleWidth, const FScalableFloat& AngleHeight);
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
