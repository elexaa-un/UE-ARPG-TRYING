// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_PlayMontage.generated.h"
/**
 * 
 */
 // 播放蒙太奇所需的参数集合，用于描述一次播放任务的配置。
USTRUCT(BlueprintType, Blueprintable)
struct FMontagePlayInfo
{
	GENERATED_BODY()

	/**
	 * 构造播放信息。
	 * @param TaskInstanceName 任务实例名称，用于区分同类播放任务。
	 * @param MontageToPlay 要播放的动画蒙太奇资源。
	 * @param Rate 播放速度倍率，1\.0 为正常速度。
	 * @param StartSection 起始播放的 Section 名称，None 表示从头开始。
	 * @param bStopWhenAbilityEnds 能力结束时是否立即停止该蒙太奇。
	 * @param AnimRootMotionTranslationScale 根运动平移缩放，用于调整位移幅度。
	 * @param StartTimeSeconds 从蒙太奇的指定时间偏移（秒）开始播放。
	 * @param bAllowInterruptAfterBlendOut 在 BlendOut 结束后是否允许被中断。
	 */

	[[nodiscard]] FMontagePlayInfo() = default;

	[[nodiscard]] FMontagePlayInfo(const FName& TaskInstanceName, UAnimMontage* const MontageToPlay, const float Rate,
		const FName& StartSection, const bool bStopWhenAbilityEnds, const float AnimRootMotionTranslationScale,
		const float StartTimeSeconds, const bool bAllowInterruptAfterBlendOut)
		: TaskInstanceName(TaskInstanceName),
		MontageToPlay(MontageToPlay),
		Rate(Rate),
		StartSection(StartSection),
		bStopWhenAbilityEnds(bStopWhenAbilityEnds),
		AnimRootMotionTranslationScale(AnimRootMotionTranslationScale),
		StartTimeSeconds(StartTimeSeconds),
		bAllowInterruptAfterBlendOut(bAllowInterruptAfterBlendOut)
	{
	}

	/** 任务实例名称，用于标识同一能力中的不同播放任务。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TaskInstanceName = NAME_None;

	/** 要播放的蒙太奇资源指针。为空时应在外部进行有效性检查。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* MontageToPlay = nullptr;

	/** 播放速率倍率。可用于加速或减速动画。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Rate = 1.f;

	/** 起始 Section 名称。未指定则从默认入口开始。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName StartSection = NAME_None;

	/** 当能力终止时是否自动停止该蒙太奇。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bStopWhenAbilityEnds = true;

	/** 根运动位移缩放系数，影响角色位移距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AnimRootMotionTranslationScale = 1.f;

	/** 从蒙太奇的指定时间偏移开始播放（单位：秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartTimeSeconds = 0.f;

	/** BlendOut 完成后是否允许被其它蒙太奇/逻辑中断。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowInterruptAfterBlendOut = false;
};

UCLASS()
class GASTEST_API UGA_PlayMontage:public UGameplayAbility
{
	GENERATED_BODY()
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
		bool bWasCancelled) override;
	/*蒙太奇播放信息*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	FMontagePlayInfo MontagePlayInfo;

	/*蒙太奇播放完成时的回调*/
	UFUNCTION(BlueprintCallable, Category = "Ability|Montage")
	void OnMontageComplete();

	/*蒙太奇混合进入完成时的回调*/
	UFUNCTION(BlueprintCallable, Category = "Ability|Montage")
	void OnMontageCancelled();

	/*蒙太奇混合退出时的回调*/
	UFUNCTION(BlueprintCallable, Category = "Ability|Montage")
	void OnMontageInterrupted();
};
