// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "VigilTypes.h"
#include "VigilNetSyncTask.generated.h"

DECLARE_DELEGATE_OneParam(FVigilSyncDelegate, class UVigilNetSyncTask* /*SyncTask*/);

/**
 * Task for providing a generic sync point for client server (one can wait for a signal from the other)
 * Vigil requires the task that provided the callback to be broadcast for internal tracking purposes
 */
UCLASS()
class VIGIL_API UVigilNetSyncTask : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	FVigilSyncDelegate OnSync;

	UFUNCTION()
	void OnSignalCallback();

	virtual void Activate() override;

	static UVigilNetSyncTask* WaitNetSync(UGameplayAbility* OwningAbility, EVigilNetSyncType SyncType);

protected:
	void SyncFinished();

	/** The event we replicate */
	EAbilityGenericReplicatedEvent::Type ReplicatedEventToListenFor;

	EVigilNetSyncType SyncType;
};
