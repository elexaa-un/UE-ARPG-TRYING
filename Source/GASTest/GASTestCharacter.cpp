// Copyright Epic Games, Inc. All Rights Reserved.

#include "GASTestCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "GameplayEffect.h"
#include "InputActionValue.h"
#include "LockOnComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

#include "GASTest.h"

AGASTestCharacter::AGASTestCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	HealthBarWidget =
		CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidget->SetupAttachment(GetRootComponent());
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetDrawSize(FVector2D(150.0f, 20.0f));
	HealthBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));

	LockShowWidget =
		CreateDefaultSubobject<UWidgetComponent>(TEXT("LockShowWidget"));
	LockShowWidget->SetupAttachment(GetRootComponent());
	LockShowWidget->SetWidgetSpace(EWidgetSpace::Screen);
	LockShowWidget->SetDrawSize(FVector2D(64.0f, 64.0f));
	LockShowWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));

	LockOnComponent =
		CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));
}

void AGASTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent)
	{
		UE_LOG(
			LogGASTest,
			Error,
			TEXT("'%s' failed to find an Enhanced Input component."),
			*GetNameSafe(this)
		);
		return;
	}

	// Base locomotion stays in C++.
	EnhancedInputComponent->BindAction(
		JumpAction,
		ETriggerEvent::Started,
		this,
		&ACharacter::Jump
	);

	EnhancedInputComponent->BindAction(
		JumpAction,
		ETriggerEvent::Completed,
		this,
		&ACharacter::StopJumping
	);

	EnhancedInputComponent->BindAction(
		MoveAction,
		ETriggerEvent::Triggered,
		this,
		&AGASTestCharacter::Move
	);

	EnhancedInputComponent->BindAction(
		MoveAction,
		ETriggerEvent::Completed,
		this,
		&AGASTestCharacter::StopMove
	);

	EnhancedInputComponent->BindAction(
		LookAction,
		ETriggerEvent::Triggered,
		this,
		&AGASTestCharacter::Look
	);

	EnhancedInputComponent->BindAction(
		MouseLookAction,
		ETriggerEvent::Triggered,
		this,
		&AGASTestCharacter::Look
	);

	// Combat input is intentionally NOT bound here.
	// BP_ThirdPersonCharacter:
	// IA_Attack       Started   -> AttackInputPressed
	// IA_ChargeAttack Started   -> ChargeAttackInputPressed
	// IA_ChargeAttack Completed -> ChargeAttackInputReleased
	// IA_Dodge        Started   -> DodgeInputPressed
}

void AGASTestCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void AGASTestCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AGASTestCharacter::DoMove(
	float Right,
	float Forward
)
{
	if (!GetController())
	{
		return;
	}

	const FRotator Rotation =
		GetController()->GetControlRotation();

	const FRotator YawRotation(
		0.0f,
		Rotation.Yaw,
		0.0f
	);

	const FVector ForwardDirection =
		FRotationMatrix(YawRotation)
		.GetUnitAxis(EAxis::X);

	const FVector RightDirection =
		FRotationMatrix(YawRotation)
		.GetUnitAxis(EAxis::Y);

	FVector DesiredDirection =
		ForwardDirection * Forward +
		RightDirection * Right;

	const float InputMagnitude =
		FMath::Clamp(
			DesiredDirection.Size(),
			0.0f,
			1.0f
		);

	MoveInWorldDirection(
		DesiredDirection,
		InputMagnitude
	);
}

void AGASTestCharacter::DoLook(float Yaw, float Pitch)
{
	if (!GetController())
	{
		return;
	}

	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}

void AGASTestCharacter::DoJumpStart()
{
	Jump();
}

void AGASTestCharacter::DoJumpEnd()
{
	StopJumping();
}
void AGASTestCharacter::AttackInputPressed()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const FGameplayTag AttackAbilityTag =
		FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Combo"));

	for (FGameplayAbilitySpec& Spec :
		AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!Spec.Ability ||
			!Spec.Ability->AbilityTags.HasTagExact(AttackAbilityTag))
		{
			continue;
		}

		if (Spec.IsActive())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[AttackInput] Ability already active")
			);

			Spec.InputPressed = true;
			AbilitySystemComponent->AbilitySpecInputPressed(Spec);
			return;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AttackInput] Activate Combo Ability")
		);

		Spec.InputPressed = true;
		AbilitySystemComponent->AbilitySpecInputPressed(Spec);
		AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		return;
	}

	UE_LOG(
		LogTemp,
		Error,
		TEXT("[AttackInput] Combo Ability Spec not found")
	);
}

void AGASTestCharacter::ChargeAttackInputPressed()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const FGameplayTag ChargeAttackTag =
		FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Charge"));

	for (FGameplayAbilitySpec& Spec :
		AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!Spec.Ability ||
			!Spec.Ability->AbilityTags.HasTagExact(ChargeAttackTag))
		{
			continue;
		}

		if (Spec.IsActive())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ChargeInput] Charge Attack already active")
			);
			return;
		}

		Spec.InputPressed = true;
		AbilitySystemComponent->AbilitySpecInputPressed(Spec);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ChargeInput] Activate Charge Attack")
		);

		AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		return;
	}

	UE_LOG(
		LogTemp,
		Error,
		TEXT("[ChargeInput] Charge Attack Ability Spec not found")
	);
}

void AGASTestCharacter::ChargeAttackInputReleased()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const FGameplayTag ChargeAttackTag =
		FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Charge"));

	for (FGameplayAbilitySpec& Spec :
		AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!Spec.Ability ||
			!Spec.Ability->AbilityTags.HasTagExact(ChargeAttackTag))
		{
			continue;
		}

		Spec.InputPressed = false;

		if (!Spec.IsActive())
		{
			return;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ChargeInput] Input Released")
		);

		AbilitySystemComponent->AbilitySpecInputReleased(Spec);
		return;
	}
}

void AGASTestCharacter::DodgeInputPressed()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const FGameplayTag DodgeAbilityTag =
		FGameplayTag::RequestGameplayTag(TEXT("Ability.Dodge"));

	const bool bActivated =
		AbilitySystemComponent->TryActivateAbilitiesByTag(
			FGameplayTagContainer(DodgeAbilityTag),
			true
		);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[DodgeInput] Activate=%d"),
		bActivated
	);
}

void AGASTestCharacter::OnMovementModeChanged(
	EMovementMode PrevMovementMode,
	uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(
		PrevMovementMode,
		PreviousCustomMode
	);

	if (!AbilitySystemComponent)
	{
		return;
	}

	const FGameplayTag InAirTag =
		FGameplayTag::RequestGameplayTag(
			TEXT("State.Movement.InAir")
		);

	const bool bIsInAir =
		GetCharacterMovement()->IsFalling();

	if (bIsInAir)
	{
		AbilitySystemComponent->AddLooseGameplayTag(InAirTag);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Movement] Enter InAir")
		);
	}
	else
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(InAirTag);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Movement] Leave InAir")
		);
	}
}
bool AGASTestCharacter::FaceLockedTarget()
{
	if (!LockOnComponent ||
		!LockOnComponent->IsLockedOn())
	{
		return false;
	}

	AActor* Target =
		LockOnComponent->GetCurrentTarget();

	if (!IsValid(Target))
	{
		return false;
	}

	FVector Direction =
		Target->GetActorLocation() -
		GetActorLocation();

	Direction.Z = 0.0f;

	if (Direction.IsNearlyZero())
	{
		return false;
	}

	Direction.Normalize();

	FRotator TargetRotation =
		Direction.Rotation();

	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;

	SetActorRotation(TargetRotation);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Combat] Face Locked Target | Target=%s | Yaw=%.2f"),
		*GetNameSafe(Target),
		TargetRotation.Yaw
	);

	return true;
}