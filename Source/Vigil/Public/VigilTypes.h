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
DECLARE_DYNAMIC_DELEGATE(FOnRequestVigilBP);

UENUM(BlueprintType)
enum class EVigilTargetingSource : uint8
{
	Pawn				UMETA(ToolTip="Use the controlled Pawn as the targeting source"),
	PawnIfValid			UMETA(ToolTip="Use the controlled Pawn as the targeting source, or the Controller if invalid"),
	Controller			UMETA(ToolTip="Use the Controller as the targeting source"),
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
DECLARE_DELEGATE_OneParam(FOnVigilSyncRequested, EVigilNetSyncType SyncType);
DECLARE_DELEGATE(FOnVigilNetSyncCompleted);
DECLARE_DYNAMIC_DELEGATE(FOnVigilNetSyncCompletedBP);

enum class EVigilNetSyncPendingState : uint8
{
	None,
	Pending,
	Completed
};

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

	bool HasValidData() const
	{
		return FocusTag != FGameplayTag::EmptyTag && HitResult.GetActor();
	}
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVigilTargetsReady, UVigilComponent*, VigilComponent, FGameplayTag, FocusTag, const TArray<FVigilFocusResult>&, Results);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnVigilFocusChanged, UVigilComponent*, VigilComponent, FGameplayTag, FocusTag, AActor*, Focus, AActor*, LastFocus, const FVigilFocusResult&, Result);

struct VIGIL_API FVigilNetSyncDelegateHandler
{
	/** Construct from a native or BP Delegate */
	FVigilNetSyncDelegateHandler(FOnVigilNetSyncCompleted&& InDelegate);
	FVigilNetSyncDelegateHandler(FOnVigilNetSyncCompletedBP&& InDelegate);

	/** Call the appropriate native/bp delegate, this could invalidate this struct */
	void Execute() const;
	
	/** Delegate that is called on notification */
	FOnVigilNetSyncCompleted Delegate;
	
	/** Delegate that is called on notification */
	FOnVigilNetSyncCompletedBP DelegateBP;

	/** A handle assigned to this delegate so it acts like a multicast delegate for removal */
	FDelegateHandle DelegateHandle;
	
	/** Indicates this delegate has been removed and will soon be destroyed, do not execute */
	bool bRemoved;
};

struct VIGIL_API FVigilNetSyncDelegateData
{
	/** Object class for cross-referencing with the class callbacks */
	TWeakObjectPtr<UClass> ObjectClass;

	/** All delegates bound to this object */
	TArray<TSharedRef<FVigilNetSyncDelegateHandler>> RegisteredDelegates;
};