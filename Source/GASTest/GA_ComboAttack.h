#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_ComboAttack.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

UCLASS()
class GASTEST_API UGA_ComboAttack : public UGameplayAbility
{
    GENERATED_BODY()

public:

    UGA_ComboAttack();

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

    /**
     * GAS 在 Ability 已经激活的情况下，
     * 再次收到输入时调用。
     */
    virtual void InputPressed(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo
    ) override;

protected:

    // =========================
    // Montage
    // =========================

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
    TObjectPtr<UAnimMontage> AttackMontage;

    /**
     * 必须和 Montage Section 名称一致。
     *
     * 0 = Attack01
     * 1 = Attack02
     * 2 = Attack03
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
    TArray<FName> ComboSections;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
    float PlayRate = 1.0f;


    // =========================
    // GameplayTag
    // =========================

    /**
     * ANS 开启窗口时添加的 Tag。
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo|Tags")
    FGameplayTag ComboWindowTag;

    /**
     * ANS 关闭窗口时发送的 Event。
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo|Tags")
    FGameplayTag ComboWindowClosedEventTag;


    // =========================
    // Runtime State
    // =========================

    UPROPERTY(BlueprintReadOnly, Category = "Combo")
    int32 CurrentComboIndex = 0;

    /**
     * 玩家是否已经输入下一刀。
     */
    UPROPERTY(BlueprintReadOnly, Category = "Combo")
    bool bComboInputBuffered = false;


    // =========================
    // Ability Tasks
    // =========================

    UPROPERTY()
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;



    // =========================
    // Montage Callback
    // =========================

    UFUNCTION()
    void OnMontageCompleted();

    UFUNCTION()
    void OnMontageCancelled();

    UFUNCTION()
    void OnMontageInterrupted();


    // =========================
// Combo
// =========================

    void TryAdvanceCombo();

    bool JumpToComboSection(int32 ComboIndex);

    bool IsComboWindowOpen() const;

    bool SetNextComboSection();


    // =========================
    // Combo Direction
    // =========================

    void CacheComboDirection();

    void ApplyComboDirection();

    UPROPERTY()
    FVector BufferedComboDirection =
        FVector::ZeroVector;

    bool bHasBufferedComboDirection = false;

    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Combo|Damage"
    )
    TArray<float> DamageMultipliers = {
        1.0f,
        1.2f,
        1.5f
    };

    UFUNCTION(BlueprintPure, Category = "Combo|Damage")
    float GetCurrentDamageMultiplier() const;
    // =========================
    // Combo Turn Event
    // =========================

    /*UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Combo|Tags"
    )
    FGameplayTag ComboTurnEventTag;

    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitGameplayEvent>
        ComboTurnEventTask;

    UFUNCTION()
    void OnComboTurnEvent(
        FGameplayEventData Payload
    );*/

    //==========================
// Combo Grace
//==========================

// 当前是否处于动画结束后的输入宽限期
    bool bInGracePeriod = false;

    // 动画结束后仍可输入下一段攻击的时间
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
    float ComboGraceTime = 0.3f;

    // Grace Period Timer
    FTimerHandle ComboGraceTimerHandle;

    void StartComboGracePeriod();

    void OnComboGracePeriodExpired();

    void AdvanceCombo();

    void FinishCombo();
};