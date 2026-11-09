#include "GA_EnemyLightAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

#include "CombatCharacterBase.h"


UGA_EnemyLightAttack::UGA_EnemyLightAttack()
{
    InstancingPolicy =
        EGameplayAbilityInstancingPolicy::InstancedPerActor;
}


void UGA_EnemyLightAttack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        TriggerEventData
    );

    // ========================================
    // Validity Check
    // ========================================

    if (!ActorInfo ||
        !ActorInfo->AbilitySystemComponent.IsValid() ||
        !ActorInfo->AvatarActor.IsValid() ||
        !AttackMontage)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "[EnemyAttack] Invalid ActorInfo / ASC / Montage"
            )
        );

        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            true
        );

        return;
    }


    // ========================================
    // Commit
    // ========================================

    if (!CommitAbility(
        Handle,
        ActorInfo,
        ActivationInfo))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[EnemyAttack] CommitAbility failed")
        );

        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            true
        );

        return;
    }


    // ========================================
    // Combat Character
    // ========================================

    ACombatCharacterBase* Character =
        Cast<ACombatCharacterBase>(
            ActorInfo->AvatarActor.Get()
        );

    if (!Character)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "[EnemyAttack] Avatar is not CombatCharacterBase"
            )
        );

        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            true
        );

        return;
    }


    // ========================================
    // Damage
    // ========================================

    Character->SetAttackDamageMultiplier(
        DamageMultiplier
    );


    // ========================================
    // Montage
    // ========================================

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::
        CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            AttackMontage,
            1.0f
        );

    if (!MontageTask)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "[EnemyAttack] Failed to create MontageTask"
            )
        );

        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            true
        );

        return;
    }


    MontageTask->OnCompleted.AddDynamic(
        this,
        &UGA_EnemyLightAttack::OnMontageCompleted
    );

    MontageTask->OnCancelled.AddDynamic(
        this,
        &UGA_EnemyLightAttack::OnMontageCancelled
    );

    MontageTask->OnInterrupted.AddDynamic(
        this,
        &UGA_EnemyLightAttack::OnMontageInterrupted
    );


    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[EnemyAttack] Activate | Multiplier=%.2f"
        ),
        DamageMultiplier
    );


    MontageTask->ReadyForActivation();
}


void UGA_EnemyLightAttack::OnMontageCompleted()
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[EnemyAttack] Montage Completed")
    );

    EndAbility(
        CurrentSpecHandle,
        CurrentActorInfo,
        CurrentActivationInfo,
        true,
        false
    );
}


void UGA_EnemyLightAttack::OnMontageCancelled()
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[EnemyAttack] Montage Cancelled")
    );

    EndAbility(
        CurrentSpecHandle,
        CurrentActorInfo,
        CurrentActivationInfo,
        true,
        true
    );
}


void UGA_EnemyLightAttack::OnMontageInterrupted()
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[EnemyAttack] Montage Interrupted")
    );

    EndAbility(
        CurrentSpecHandle,
        CurrentActorInfo,
        CurrentActivationInfo,
        true,
        true
    );
}