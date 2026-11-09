#include "GA_Dodge.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "TimerManager.h"

UGA_Dodge::UGA_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 如果你的项目暂时是单机，这样即可。
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_Dodge::ActivateAbility(
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

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
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

	ACharacter* Character =
		Cast<ACharacter>(ActorInfo->AvatarActor.Get());

	if (!Character || !DodgeMontage)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Dodge] Character or DodgeMontage is NULL")
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

	// -------------------------
	// 1. 判断普通闪避方向
	// -------------------------

	DodgeDirection = CalculateDodgeDirection();

	const FName SectionName =
		GetSectionName(DodgeDirection);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Dodge] Activate | Section=%s"),
		*SectionName.ToString()
	);

	// -------------------------
	// 2. 开启 Perfect Window
	// -------------------------

	bPerfectWindowActive = true;
	bPerfectDodgeTriggered = false;

	Character->GetWorldTimerManager().SetTimer(
		PerfectWindowTimer,
		this,
		&UGA_Dodge::ClosePerfectWindow,
		PerfectWindowDuration,
		false
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Dodge] Perfect Window OPEN")
	);

	// -------------------------
	// 3. 播放普通闪避
	// -------------------------

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			DodgeMontage,
			1.0f,
			SectionName
		);

	if (!MontageTask)
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

	MontageTask->OnCompleted.AddDynamic(
		this,
		&UGA_Dodge::OnMontageCompleted
	);

	MontageTask->OnBlendOut.AddDynamic(
		this,
		&UGA_Dodge::OnMontageBlendOut
	);

	MontageTask->OnInterrupted.AddDynamic(
		this,
		&UGA_Dodge::OnMontageInterrupted
	);

	MontageTask->OnCancelled.AddDynamic(
		this,
		&UGA_Dodge::OnMontageCancelled
	);

	MontageTask->ReadyForActivation();
}

EDodgeDirection UGA_Dodge::CalculateDodgeDirection() const
{
	if (!CurrentActorInfo ||
		!CurrentActorInfo->AvatarActor.IsValid())
	{
		return EDodgeDirection::Back;
	}

	const ACharacter* Character =
		Cast<ACharacter>(
			CurrentActorInfo->AvatarActor.Get()
		);

	if (!Character)
	{
		return EDodgeDirection::Back;
	}

	/*
	 * GetLastMovementInputVector()
	 *
	 * 这里拿的是玩家最近一次移动输入。
	 *
	 * 对你的 Enhanced Input WASD 来说非常适合。
	 */
	const FVector Input =
		Character->GetLastMovementInputVector();

	// 没有 WASD
	if (Input.IsNearlyZero())
	{
		return EDodgeDirection::Back;
	}

	const FVector Forward =
		Character->GetActorForwardVector();

	const FVector Right =
		Character->GetActorRightVector();

	const float ForwardAmount =
		FVector::DotProduct(
			Input.GetSafeNormal(),
			Forward
		);

	const float RightAmount =
		FVector::DotProduct(
			Input.GetSafeNormal(),
			Right
		);

	/*
	 * 判断主要方向。
	 *
	 * W+D 这种情况下：
	 * 哪个方向分量更大，就使用哪个。
	 */

	if (FMath::Abs(ForwardAmount) >=
		FMath::Abs(RightAmount))
	{
		if (ForwardAmount >= 0.0f)
		{
			return EDodgeDirection::Front;
		}

		return EDodgeDirection::Back;
	}

	if (RightAmount >= 0.0f)
	{
		return EDodgeDirection::Right;
	}

	return EDodgeDirection::Left;
}

FName UGA_Dodge::GetSectionName(
	EDodgeDirection Direction) const
{
	switch (Direction)
	{
	case EDodgeDirection::Front:
		return TEXT("Front");

	case EDodgeDirection::Back:
		return TEXT("Back");

	case EDodgeDirection::Left:
		return TEXT("Left");

	case EDodgeDirection::Right:
		return TEXT("Right");

	default:
		return TEXT("Back");
	}
}
void UGA_Dodge::ClosePerfectWindow()
{
	if (!bPerfectWindowActive)
	{
		return;
	}

	bPerfectWindowActive = false;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Dodge] Perfect Window CLOSE")
	);
}

void UGA_Dodge::TriggerPerfectDodge(
	const FVector& ImpactPoint)
{
	if (!bPerfectWindowActive)
	{
		return;
	}

	if (bPerfectDodgeTriggered)
	{
		return;
	}

	if (!CurrentActorInfo ||
		!CurrentActorInfo->AvatarActor.IsValid())
	{
		return;
	}

	ACharacter* Character =
		Cast<ACharacter>(
			CurrentActorInfo->AvatarActor.Get()
		);

	if (!Character)
	{
		return;
	}

	bPerfectDodgeTriggered = true;
	bPerfectWindowActive = false;

	Character->GetWorldTimerManager().ClearTimer(
		PerfectWindowTimer
	);

	// =========================
	// 判断攻击来源左右
	// =========================

	FVector ToImpact =
		ImpactPoint -
		Character->GetActorLocation();

	ToImpact.Z = 0.0f;

	if (ToImpact.IsNearlyZero())
	{
		return;
	}

	ToImpact.Normalize();

	const FVector Right =
		Character->GetActorRightVector();

	const float Side =
		FVector::DotProduct(
			Right,
			ToImpact
		);

	FName PerfectSection;

	/*
	 * Side > 0
	 *
	 * 攻击来自角色右侧
	 *
	 * → 往左闪
	 */
	if (Side > 0.0f)
	{
		PerfectSection = TEXT("PerfectLeft");
	}
	else
	{
		/*
		 * 攻击来自左侧
		 *
		 * → 往右闪
		 */
		PerfectSection = TEXT("PerfectRight");
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[Dodge] PERFECT! Side=%.2f | Section=%s"
		),
		Side,
		*PerfectSection.ToString()
	);

	PlayPerfectSection(PerfectSection);
}

void UGA_Dodge::PlayPerfectSection(
	FName SectionName)
{
	if (!CurrentActorInfo ||
		!CurrentActorInfo->AnimInstance.IsValid())
	{
		return;
	}

	UAnimInstance* AnimInstance =
		CurrentActorInfo->AnimInstance.Get();

	if (!AnimInstance || !DodgeMontage)
	{
		return;
	}

	AnimInstance->Montage_JumpToSection(
		SectionName,
		DodgeMontage
	);
}
void UGA_Dodge::OnMontageCompleted()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Dodge] Montage Completed")
	);

	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		false
	);
}

void UGA_Dodge::OnMontageBlendOut()
{
}

void UGA_Dodge::OnMontageInterrupted()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Dodge] Montage Interrupted")
	);

	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		true
	);
}

void UGA_Dodge::OnMontageCancelled()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Dodge] Montage Cancelled")
	);

	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		true
	);
}

void UGA_Dodge::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ActorInfo &&
		ActorInfo->AvatarActor.IsValid())
	{
		if (ACharacter* Character =
			Cast<ACharacter>(
				ActorInfo->AvatarActor.Get()
			))
		{
			Character
				->GetWorldTimerManager()
				.ClearTimer(
					PerfectWindowTimer
				);
		}
	}

	bPerfectWindowActive = false;
	bPerfectDodgeTriggered = false;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled
	);
}