// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "VigilTypes.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "VigilScanTask.generated.h"

class UVigilNetSyncTask;
class UAbilityTask_NetworkSyncPoint;
struct FTargetingRequestHandle;
class UVigilComponent;

/**
 * Vigil's passive perpetual task that scans for focus targets
 */
UCLASS()
class VIGIL_API UVigilScanTask : public UAbilityTask
{
	GENERATED_BODY()

public:
	FTimerHandle VigilWaitTimer;

protected:
	UPROPERTY()
	TWeakObjectPtr<UVigilComponent> VC;

	TOptional<FString> WaitReason;
	TOptional<FString> VeryVerboseWaitReason;
	
public:
	/**
	 * Vigil's passive perpetual task that scans for focus targets
	 * @param OwningAbility The ability that owns this task
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE", DisplayName="Vigil Scan"))
	static UVigilScanTask* VigilScan(UGameplayAbility* OwningAbility);

	virtual void Activate() override;

	void WaitForVigil(float Delay, const TOptional<FString>& Reason = {}, const TOptional<FString>& VeryVerboseReason = {});
	void RequestVigil();
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
	/** Tracked to prevent premature GC and allow ending during OnDestroy */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UVigilNetSyncTask>> SyncTasks;

	ENetMode GetOwnerNetMode() const;
	FString GetRoleString() const;
};
