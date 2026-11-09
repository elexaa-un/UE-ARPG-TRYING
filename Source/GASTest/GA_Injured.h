// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Injured.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
/**
 * 
 */
UCLASS()
class GASTEST_API UGA_Injured : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Injured();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData
    ) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled
    ) override;

protected:

    // =========================
    // Montage
    // =========================

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage")
    TObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage")
    float PlayRate = 1.0f;

    // =========================
    // Montage Callback
    // =========================

    UFUNCTION()
    void OnMontageCompleted();

    UFUNCTION()
    void OnMontageCancelled();

    UFUNCTION()
    void OnMontageInterrupted();

     FName CalculateHitDirection(
        AActor* Target,
         const FHitResult& HitResult
    ) const;
};
