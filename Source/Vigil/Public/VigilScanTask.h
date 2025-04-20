// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "VigilScanTask.generated.h"

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
	
public:
	/**
	 * Vigil's passive perpetual task that scans for focus targets
	 * @param OwningAbility The ability that owns this task
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UVigilScanTask* VigilScan(UGameplayAbility* OwningAbility);

	virtual void Activate() override;

	void WaitForVigil(float Delay);
	void RequestVigil();
	void OnVigilComplete(FTargetingRequestHandle TargetingHandle, FGameplayTag FocusTag);

	/** Broadcast from VigilComponent */
	UFUNCTION()
	void OnPauseVigil(bool bPaused);

	/** Broadcast from VigilComponent after all our tasks were removed, i.e. we never get our callback to continue */
	UFUNCTION()
	void OnRequestVigil();
	
	virtual void OnDestroy(bool bInOwnerFinished) override;
};
