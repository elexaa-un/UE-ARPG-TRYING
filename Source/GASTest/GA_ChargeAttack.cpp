#include "GA_ChargeAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "GASTestCharacter.h"
#include "CombatCharacterBase.h"


UGA_ChargeAttack::UGA_ChargeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 如果你的项目目前是本地单机测试，这个设置够用。
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}


void UGA_ChargeAttack::ActivateAbility(
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

	/* ================================
	 * Validity Check
	 * ================================ */

	if (!ActorInfo ||
		!ActorInfo->AbilitySystemComponent.IsValid() ||
		!ActorInfo->AvatarActor.IsValid() ||
		!ChargeAttackMontage)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[ChargeAttack] Invalid ActorInfo / ASC / Montage")
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


	/* ================================
	 * Commit
	 * ================================ */

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(
			Handle,
			ActorInfo,
			ActivationInfo,
			true,
			true
		);

		return;
	}


	/* ================================
	 * Reset Runtime State
	 * ================================ */

	ChargeLoopCount = 0;
	ChargeLevel = 0;

	bAttackReleased = false;
	bInChargeLoop = false;

	ChargeStartTime = GetWorld()->GetTimeSeconds();


	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[ChargeAttack] Activate | StartTime=%.2f"),
		ChargeStartTime
	);
	if (AGASTestCharacter* Character =
		Cast<AGASTestCharacter>(
			GetAvatarActorFromActorInfo()
		))
	{
		Character->FaceLockedTarget();
	}

	/* ================================
	 * Play Montage
	 * ================================ */

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			ChargeAttackMontage,
			1.0f,
			ChargeStartSection
		);

	if (!MontageTask)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[ChargeAttack] Failed to create MontageTask")
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
		&UGA_ChargeAttack::OnMontageCompleted
	);

	MontageTask->OnBlendOut.AddDynamic(
		this,
		&UGA_ChargeAttack::OnMontageBlendOut
	);

	MontageTask->OnInterrupted.AddDynamic(
		this,
		&UGA_ChargeAttack::OnMontageInterrupted
	);

	MontageTask->OnCancelled.AddDynamic(
		this,
		&UGA_ChargeAttack::OnMontageCancelled
	);


	MontageTask->ReadyForActivation();


	/* ================================
	 * ChargeStart -> ChargeLoop
	 * ================================ */

	 /*
	  * Montage 本身设置：
	  *
	  * ChargeStart -> ChargeLoop
	  *
	  * 因此这里不需要手动跳转。
	  *
	  * 我们只需要计算 ChargeStart + ChargeLoop
	  * 的 Section 时间。
	  */

	float ChargeStartLength = 0.f;

	const int32 StartIndex =
		ChargeAttackMontage->GetSectionIndex(ChargeStartSection);

	if (StartIndex != INDEX_NONE)
	{
		ChargeStartLength =
			ChargeAttackMontage->GetSectionLength(StartIndex);
	}


	FTimerHandle StartTimer;

	GetWorld()->GetTimerManager().SetTimer(
		StartTimer,
		this,
		&UGA_ChargeAttack::EnterChargeLoop,
		ChargeStartLength,
		false
	);
}


void UGA_ChargeAttack::EnterChargeLoop()
{
	if (!IsActive() || bAttackReleased)
	{
		return;
	}

	bInChargeLoop = true;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[ChargeAttack] Enter ChargeLoop | Loop=%d"),
		ChargeLoopCount + 1
	);


	JumpToSection(ChargeLoopSection);


	const float LoopLength = GetChargeLoopLength();

	if (LoopLength <= 0.f)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[ChargeAttack] Invalid ChargeLoop length")
		);

		ReleaseAttack();

		return;
	}


	GetWorld()->GetTimerManager().SetTimer(
		ChargeLoopTimerHandle,
		this,
		&UGA_ChargeAttack::HandleChargeLoopFinished,
		LoopLength,
		false
	);
}


void UGA_ChargeAttack::HandleChargeLoopFinished()
{
	if (!IsActive() || bAttackReleased)
	{
		return;
	}

	++ChargeLoopCount;


	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[ChargeAttack] Loop Finished | Count=%d/%d | Time=%.2f"),
		ChargeLoopCount,
		MaxChargeLoopCount,
		GetCurrentChargeDuration()
	);


	/* ================================
	 * 满两轮 -> 自动攻击
	 * ================================ */

	if (ChargeLoopCount >= MaxChargeLoopCount)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ChargeAttack] Max Charge -> Auto Release")
		);

		ReleaseAttack();

		return;
	}


	/* ================================
	 * 再播放一次 ChargeLoop
	 * ================================ */

	JumpToSection(ChargeLoopSection);


	const float LoopLength = GetChargeLoopLength();

	GetWorld()->GetTimerManager().SetTimer(
		ChargeLoopTimerHandle,
		this,
		&UGA_ChargeAttack::HandleChargeLoopFinished,
		LoopLength,
		false
	);
}


void UGA_ChargeAttack::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(
		Handle,
		ActorInfo,
		ActivationInfo
	);


	if (!IsActive())
	{
		return;
	}


	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[ChargeAttack] Input Released | Duration=%.2f"),
		GetCurrentChargeDuration()
	);


	ReleaseAttack();
}


void UGA_ChargeAttack::ReleaseAttack()
{
	/* ================================
	 * 防止重复 Release
	 * ================================ */

	if (bAttackReleased || !IsActive())
	{
		return;
	}

	bAttackReleased = true;


	/* ================================
	 * Stop Charge Timer
	 * ================================ */

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ChargeLoopTimerHandle
		);
	}


	/* ================================
	 * Calculate Charge Level
	 * ================================ */

	CalculateChargeLevel();

	// ========================================
	// 通用战斗逻辑：设置伤害倍率
	// ========================================

	if (ACombatCharacterBase* CombatCharacter =
		Cast<ACombatCharacterBase>(
			GetAvatarActorFromActorInfo()
		))
	{
		CombatCharacter->SetAttackDamageMultiplier(
			GetDamageMultiplier()
		);
	}

	// ========================================
	// Player 专属：面向锁定目标
	// ========================================

	if (AGASTestCharacter* PlayerCharacter =
		Cast<AGASTestCharacter>(
			GetAvatarActorFromActorInfo()
		))
	{
		PlayerCharacter->FaceLockedTarget();
	}


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[ChargeAttack] RELEASE | "
			"Duration=%.2f | Level=%d"
		),
		GetCurrentChargeDuration(),
		ChargeLevel
	);


	/* ================================
	 * Jump to Attack
	 * ================================ */

	JumpToSection(AttackSection);
}


void UGA_ChargeAttack::CalculateChargeLevel()
{
	const float Duration = GetCurrentChargeDuration();


	if (Duration >= Level2ChargeTime)
	{
		ChargeLevel = 2;
	}
	else if (Duration >= Level1ChargeTime)
	{
		ChargeLevel = 1;
	}
	else
	{
		ChargeLevel = 0;
	}
}


float UGA_ChargeAttack::GetCurrentChargeDuration() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(
		0.f,
		GetWorld()->GetTimeSeconds() - ChargeStartTime
	);
}


void UGA_ChargeAttack::JumpToSection(
	const FName& SectionName)
{
	if (!CurrentActorInfo ||
		!CurrentActorInfo->GetAnimInstance() ||
		!ChargeAttackMontage)
	{
		return;
	}


	UAnimInstance* AnimInstance =
		CurrentActorInfo->GetAnimInstance();


	if (!AnimInstance->Montage_IsPlaying(
		ChargeAttackMontage))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[ChargeAttack] Montage is not playing. "
				"Cannot jump to %s"
			),
			*SectionName.ToString()
		);

		return;
	}


	AnimInstance->Montage_JumpToSection(
		SectionName,
		ChargeAttackMontage
	);


	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[ChargeAttack] Jump -> %s"),
		*SectionName.ToString()
	);
}


float UGA_ChargeAttack::GetChargeLoopLength() const
{
	if (!ChargeAttackMontage)
	{
		return 0.f;
	}


	const int32 SectionIndex =
		ChargeAttackMontage->GetSectionIndex(
			ChargeLoopSection
		);


	if (SectionIndex == INDEX_NONE)
	{
		return 0.f;
	}


	return ChargeAttackMontage->GetSectionLength(
		SectionIndex
	);
}


/* ================================
 * Montage Callbacks
 * ================================ */

void UGA_ChargeAttack::OnMontageCompleted()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[ChargeAttack] Montage Completed | "
			"Level=%d"
		),
		ChargeLevel
	);


	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		false
	);
}


void UGA_ChargeAttack::OnMontageBlendOut()
{
	// 正常 BlendOut 不额外处理。
}


void UGA_ChargeAttack::OnMontageInterrupted()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[ChargeAttack] Montage Interrupted")
	);


	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		true
	);
}


void UGA_ChargeAttack::OnMontageCancelled()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[ChargeAttack] Montage Cancelled")
	);


	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		true
	);
}


void UGA_ChargeAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ChargeLoopTimerHandle
		);
	}


	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled
	);
}

float UGA_ChargeAttack::GetDamageMultiplier() const
{
	switch (ChargeLevel)
	{
	case 1:
		return Level1DamageMultiplier;

	case 2:
		return Level2DamageMultiplier;

	default:
		return BaseDamageMultiplier;
	}
}