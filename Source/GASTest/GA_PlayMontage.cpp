// Fill out your copyright notice in the Description page of Project Settings.

#include "GA_PlayMontage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

void UGA_PlayMontage::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	/*做一些空检测，保证代码执行安全*/
	// 基本检查
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) // 可选：根据需要消耗资源/冷却
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ true);
		return;
	}

	if (!MontagePlayInfo.MontageToPlay)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		UE_LOG(LogTemp, Warning, TEXT("UGameplayAbility_PlayMontage: MontageOnPlay is null."));
#endif
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	//创建播放蒙太奇并等待完成的任务
	UAbilityTask_PlayMontageAndWait* PlayMontageAndWait =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			MontagePlayInfo.TaskInstanceName,
			MontagePlayInfo.MontageToPlay,
			MontagePlayInfo.Rate,
			MontagePlayInfo.StartSection,
			MontagePlayInfo.bStopWhenAbilityEnds,
			MontagePlayInfo.AnimRootMotionTranslationScale,
			MontagePlayInfo.StartTimeSeconds,
			MontagePlayInfo.bAllowInterruptAfterBlendOut
		);
	if (PlayMontageAndWait)
	{
		//绑定回调函数
		PlayMontageAndWait->OnCompleted.AddDynamic(this, &UGA_PlayMontage::OnMontageComplete);
		PlayMontageAndWait->OnCancelled.AddDynamic(this, &UGA_PlayMontage::OnMontageCancelled);
		PlayMontageAndWait->OnInterrupted.AddDynamic(this, &UGA_PlayMontage::OnMontageInterrupted);
		//播放蒙太奇并等待完成
		PlayMontageAndWait->ReadyForActivation();
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UGameplayAbility_PlayMontage::ActivateAbility failed to create PlayMontageAndWait task"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_PlayMontage::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_PlayMontage::OnMontageComplete()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UGA_PlayMontage::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_PlayMontage::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
