#include "EnemyCharacter.h"
#include "AI/EnemyBrainComponent.h"
#include "AI/Perception/EnemyVisionComponent.h"
#include "AI/EnemyNavigationComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    VisionPoint = CreateDefaultSubobject<USceneComponent>(TEXT("VisionPoint"));
    VisionPoint->SetupAttachment(GetRootComponent());
    VisionPoint->SetRelativeLocation(FVector(0.0, 0.0, 70.0));

    EnemyVisionComponent = CreateDefaultSubobject<UEnemyVisionComponent>(TEXT("EnemyVisionComponent"));
    EnemyNavigationComponent = CreateDefaultSubobject<UEnemyNavigationComponent>(TEXT("EnemyNavigationComponent"));
    EnemyBrainComponent = CreateDefaultSubobject<UEnemyBrainComponent>(TEXT("EnemyBrainComponent"));
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCapsuleComponent()->InitCapsuleSize(
        42.0f,
        96.0f
    );

    // Enemy 默认由移动方向决定朝向。
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;

    GetCharacterMovement()->RotationRate =
        FRotator(0.0f, 500.0f, 0.0f);

    GetCharacterMovement()->MaxWalkSpeed = 350.0f;

    // Enemy自己的血条
    HealthBarWidget =
        CreateDefaultSubobject<UWidgetComponent>(
            TEXT("HealthBarWidget")
        );

    HealthBarWidget->SetupAttachment(
        GetRootComponent()
    );

    HealthBarWidget->SetWidgetSpace(
        EWidgetSpace::Screen
    );

    HealthBarWidget->SetDrawSize(
        FVector2D(150.0f, 20.0f)
    );

    HealthBarWidget->SetRelativeLocation(
        FVector(0.0f, 0.0f, 120.0f)
    );
}

void AEnemyCharacter::StopMovementInput()
{
    Super::StopMovementInput();
    ConsumeMovementInputVector();
    GetCharacterMovement()->StopMovementImmediately();
}

bool AEnemyCharacter::RequestLightAttack()
{
    UAbilitySystemComponent* ASC =
        GetAbilitySystemComponent();

    if (!ASC)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Enemy] ASC is null")
        );

        return false;
    }

    const FGameplayTag AttackTag =
        FGameplayTag::RequestGameplayTag(
            FName("Ability.Attack.Light")
        );

    FGameplayTagContainer AttackTags;
    AttackTags.AddTag(AttackTag);

    const bool bActivated =
        ASC->TryActivateAbilitiesByTag(
            AttackTags
        );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Enemy] RequestLightAttack | Activated=%d"
        ),
        bActivated
    );

    return bActivated;
}

void AEnemyCharacter::SetVisionYaw(float Yaw)
{
    if (!IsValid(VisionPoint))
    {
        return;
    }

    // 外部主动设置观察方向时，取消自动回正
    bResettingVision = false;

    FRotator Rotation = VisionPoint->GetRelativeRotation();
    Rotation.Yaw = Yaw;

    VisionPoint->SetRelativeRotation(Rotation);
}

bool AEnemyCharacter::IsCombatActionActive() const
{
    if (!AbilitySystemComponent)
    {
        return false;
    }

    return AbilitySystemComponent->HasMatchingGameplayTag(
        FGameplayTag::RequestGameplayTag(
            TEXT("Ability.Attack")
        )
    );
}

void AEnemyCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bResettingVision || !IsValid(VisionPoint))
    {
        return;
    }

    const FRotator CurrentRotation =
        VisionPoint->GetRelativeRotation();

    const FRotator TargetRotation(
        CurrentRotation.Pitch,
        0.0f,
        CurrentRotation.Roll
    );

    const FRotator NewRotation =
        FMath::RInterpTo(
            CurrentRotation,
            TargetRotation,
            DeltaTime,
            VisionResetSpeed
        );

    VisionPoint->SetRelativeRotation(NewRotation);

    if (FMath::Abs(NewRotation.Yaw) < 0.5f)
    {
        SetVisionYaw(0.0f);
        bResettingVision = false;
    }
}