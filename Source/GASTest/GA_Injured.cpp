// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Injured.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"

UGA_Injured::UGA_Injured() {
	InstancingPolicy =
		EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// ============================================================
// Activate
// ============================================================
void UGA_Injured::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData) {
    Super::ActivateAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        TriggerEventData
    );
    if (!ActorInfo ||
        !ActorInfo->AbilitySystemComponent.IsValid() ||
        !AttackMontage)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Combo] Activate check failed")
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
    AActor* Target =
        ActorInfo->AvatarActor.Get();

    const FHitResult* HitResult =
        TriggerEventData->ContextHandle.GetHitResult();

    if (!HitResult)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[HitReact] No HitResult in EffectContext")
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
    
    FName name = CalculateHitDirection(Target, *HitResult);
    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::
        CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            AttackMontage,
            PlayRate,
            name,
            true,
            1.0f,
            0.0f,
            false
        );
    if (MontageTask) {
        MontageTask->OnCompleted.AddDynamic(
            this,
            &UGA_Injured::OnMontageCompleted
        );

        MontageTask->OnCancelled.AddDynamic(
            this,
            &UGA_Injured::OnMontageCancelled
        );

        MontageTask->OnInterrupted.AddDynamic(
            this,
            &UGA_Injured::OnMontageInterrupted
        );
        MontageTask->ReadyForActivation();
    }
    else {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[HitMontage] Montage is null")
        );
    }
}

    void UGA_Injured::EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo * ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled)
{
        Super::EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            bReplicateEndAbility,
            bWasCancelled
        );
    
}

    FName UGA_Injured::CalculateHitDirection(
        AActor* Target,
        const FHitResult& HitResult) const
    {
        if (!Target)
        {
            return NAME_None;
        }

        FVector ToHit =
            HitResult.ImpactPoint
            - Target->GetActorLocation();

        ToHit.Z = 0.0f;

        if (!ToHit.Normalize())
        {
            return TEXT("Hit_Front");
        }

        const FVector Forward =
            Target->GetActorForwardVector();

        const FVector Right =
            Target->GetActorRightVector();

        const float ForwardDot =
            FVector::DotProduct(
                Forward,
                ToHit
            );

        const float RightDot =
            FVector::DotProduct(
                Right,
                ToHit
            );

        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "[HitReact] Impact=%s | ForwardDot=%.2f | RightDot=%.2f"
            ),
            *HitResult.ImpactPoint.ToString(),
            ForwardDot,
            RightDot
        );

        const float AngleRadians =
            FMath::Atan2(
                RightDot,
                ForwardDot
            );

        const float AngleDegrees =
            FMath::RadiansToDegrees(
                AngleRadians
            );
        if (AngleDegrees >= -60.0f &&
            AngleDegrees <= 60.0f)
        {
            return TEXT("Hit_Front");
        }

        if (AngleDegrees > 60.0f &&
            AngleDegrees < 120.0f)
        {
            return TEXT("Hit_Right");
        }

        if (AngleDegrees < -60.0f &&
            AngleDegrees > -120.0f)
        {
            return TEXT("Hit_Left");
        }

        return TEXT("Hit_Back");
    }

    // ============================================================
// Montage Callback
// ============================================================

    void UGA_Injured::OnMontageCompleted()
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Combo] Montage Completed")
        );


        EndAbility(
            CurrentSpecHandle,
            CurrentActorInfo,
            CurrentActivationInfo,
            false,
            false
        );
    }


    void UGA_Injured::OnMontageCancelled()
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Combo] Montage Cancelled")
        );


        EndAbility(
            CurrentSpecHandle,
            CurrentActorInfo,
            CurrentActivationInfo,
            true,
            true
        );
    }


    void UGA_Injured::OnMontageInterrupted()
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Combo] Montage Interrupted")
        );


        EndAbility(
            CurrentSpecHandle,
            CurrentActorInfo,
            CurrentActivationInfo,
            true,
            true
        );
    }