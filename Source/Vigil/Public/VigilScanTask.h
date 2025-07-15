// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "VigilTypes.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "VigilScanTask.generated.h"

class UVigilNetSyncTask;
class UVigilComponent;
struct FTargetingRequestHandle;

/**
 * Vigil's passive perpetual task that scans for focus targets
 */
UCLASS()
class VIGIL_API UVigilScanTask : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FTimerHandle VigilWaitTimer;

	UPROPERTY()
	FTimerHandle FailsafeTimer;

protected:
	UPROPERTY()
	TWeakObjectPtr<UVigilComponent> VC;

	TOptional<FString> WaitReason;
	TOptional<FString> VeryVerboseWaitReason;
	
public:
	UVigilScanTask(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/**
	 * Vigil's passive perpetual task that scans for focus targets
	 * @param OwningAbility The ability that owns this task
	 * @param ErrorWaitDelay Delay before we attempt any requests after encountering an error
	 * @param FailsafeDelay Delay before we request a new target if we don't get one
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE", DisplayName="Vigil Scan"))
	static UVigilScanTask* VigilScan(UGameplayAbility* OwningAbility, float ErrorWaitDelay = 0.5f, float FailsafeDelay = 1.f);

	virtual void Activate() override;

	void WaitForVigil(float InDelay, const TOptional<FString>& Reason = {}, const TOptional<FString>& VeryVerboseReason = {});
	void RequestVigil();
	void OnVigilCompleteSync(FTargetingRequestHandle TargetingHandle, FGameplayTag FocusTag);
	void OnVigilComplete(FTargetingRequestHandle TargetingHandle, FGameplayTag FocusTag);

	/** Broadcast from VigilComponent */
	UFUNCTION()
	void OnPauseVigil(bool bPaused);

	/** Broadcast from VigilComponent after all our tasks were removed, i.e. we never get our callback to continue */
	UFUNCTION()
	void OnRequestVigil();

	UFUNCTION()
	void OnRequestNetSync(EVigilNetSyncType SyncType);

	UFUNCTION()
	void OnNetSync(UVigilNetSyncTask* SyncTask);
	
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	UPROPERTY()
	float Delay = 0.5f;

	UPROPERTY()
	float FailsafeDelay = 1.f;
	
	/** Tracked to prevent premature GC and allow ending during OnDestroy */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UVigilNetSyncTask>> SyncTasks;

	/** Delayed callback until we get the next targets */
	EVigilNetSyncPendingState PendingNetSync = EVigilNetSyncPendingState::None;

	ENetMode GetOwnerNetMode() const;
	FString GetRoleString() const;
};
