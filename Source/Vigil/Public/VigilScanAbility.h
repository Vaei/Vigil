// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"

#include "VigilScanAbility.generated.h"

/**
 * Passive ability that Vigil uses to scan for focus options
 * Call UVigilScanTask from ActivateAbility from your derived ability - that's it!
 */
UCLASS(BlueprintType, Blueprintable, Abstract)
class VIGIL_API UVigilScanAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** Automatically activate this ability after being granted */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AdvancedDisplay, Category=Vigil)
	bool bAutoActivateOnGrantAbility = true;
	
public:
	UVigilScanAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
};
