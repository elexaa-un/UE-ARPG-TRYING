#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_ChargeAttack.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

/**
 * 蓄力重攻击
 *
 * ChargeStart -> ChargeLoop #1 -> ChargeLoop #2 -> Attack
 *
 * - 玩家任意时刻松开输入：立即进入 Attack
 * - ChargeLoop 最多播放两轮
 * - 第二轮结束仍未松开：自动进入 Attack
 */
UCLASS()
class GASTEST_API UGA_ChargeAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ChargeAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:

	/* ================================
	 * Montage
	 * ================================ */

	 /** 蓄力攻击 Montage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge Attack")
	TObjectPtr<UAnimMontage> ChargeAttackMontage;

	/** 蓄力开始 Section */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge Attack|Sections")
	FName ChargeStartSection = TEXT("ChargeStart");

	/** 蓄力循环 Section */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge Attack|Sections")
	FName ChargeLoopSection = TEXT("ChargeLoop");

	/** 攻击 Section */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge Attack|Sections")
	FName AttackSection = TEXT("Attack");


	/* ================================
	 * Charge
	 * ================================ */

	 /** 最大循环次数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge Attack|Charge")
	int32 MaxChargeLoopCount = 2;

	/**
	 * 一级蓄力所需时间。
	 * 注意：这里是玩家实际按住攻击键的时间。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge Attack|Charge")
	float Level1ChargeTime = 0.8f;

	/**
	 * 二级蓄力时间。
	 * 达到这个时间后即视为满蓄力。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge Attack|Charge")
	float Level2ChargeTime = 1.6f;


	/* ================================
	 * Runtime
	 * ================================ */

	 /** 当前已经完成多少轮 ChargeLoop */
	UPROPERTY(BlueprintReadOnly, Category = "Charge Attack|Runtime")
	int32 ChargeLoopCount = 0;

	/** 最终蓄力等级：0 / 1 / 2 */
	UPROPERTY(BlueprintReadOnly, Category = "Charge Attack|Runtime")
	int32 ChargeLevel = 0;

	/** 开始蓄力的世界时间 */
	float ChargeStartTime = 0.f;

	/** 防止重复进入 Attack */
	bool bAttackReleased = false;

	/** 是否已经进入 ChargeLoop */
	bool bInChargeLoop = false;


	/* ================================
	 * Montage callbacks
	 * ================================ */

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();


	/* ================================
	 * Internal
	 * ================================ */

	 /** 释放攻击 */
	void ReleaseAttack();

	/** 根据实际蓄力时间计算蓄力等级 */
	void CalculateChargeLevel();

	/** 进入 ChargeLoop */
	void EnterChargeLoop();

	/** 当前 ChargeLoop 播放结束 */
	void HandleChargeLoopFinished();

	/** 跳转 Montage Section */
	void JumpToSection(const FName& SectionName);

	/** 获取当前蓄力时间 */
	float GetCurrentChargeDuration() const;


	/* ================================
	 * Timer
	 * ================================ */

	 /**
	  * 用 Timer 控制 ChargeLoop 的结束。
	  *
	  * 第一版实现中，比依赖 Montage Section End Delegate 更简单稳定。
	  */
	FTimerHandle ChargeLoopTimerHandle;

	/** 当前 ChargeLoop 的长度 */
	float GetChargeLoopLength() const;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Charge Attack|Damage"
	)
	float BaseDamageMultiplier = 1.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Charge Attack|Damage"
	)
	float Level1DamageMultiplier = 1.3f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Charge Attack|Damage"
	)
	float Level2DamageMultiplier = 2.0f;

	UFUNCTION(BlueprintPure, Category = "Charge Attack|Damage")
	float GetDamageMultiplier() const;
};