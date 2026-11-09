#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_EnemyLightAttack.generated.h"

class UAnimMontage;

UCLASS()
class GASTEST_API UGA_EnemyLightAttack
    : public UGameplayAbility
{
    GENERATED_BODY()

public:

    UGA_EnemyLightAttack();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData
    ) override;

protected:

    // ================================
    // Attack
    // ================================

    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Enemy Attack"
    )
    TObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Enemy Attack|Damage"
    )
    float DamageMultiplier = 1.0f;

    // ================================
    // Montage Callbacks
    // ================================

    UFUNCTION()
    void OnMontageCompleted();

    UFUNCTION()
    void OnMontageCancelled();

    UFUNCTION()
    void OnMontageInterrupted();
};