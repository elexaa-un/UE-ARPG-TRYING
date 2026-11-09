#include "GA_ComboAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GASTestCharacter.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"


UGA_ComboAttack::UGA_ComboAttack()
{
    // Combo 有运行时状态，必须实例化
    InstancingPolicy =
        EGameplayAbilityInstancingPolicy::InstancedPerActor;
}


// ============================================================
// Activate
// ============================================================

void UGA_ComboAttack::ActivateAbility(
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


    // --------------------------------------------------------
    // 基础检查
    // --------------------------------------------------------

    if (!ActorInfo ||
        !ActorInfo->AbilitySystemComponent.IsValid() ||
        !AttackMontage ||
        ComboSections.IsEmpty())
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


    if (!ComboWindowTag.IsValid())
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


    if (!ComboWindowClosedEventTag.IsValid())
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


    // --------------------------------------------------------
    // Commit
    // --------------------------------------------------------

    if (!CommitAbility(
        Handle,
        ActorInfo,
        ActivationInfo))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Combo] CommitAbility failed")
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


    // --------------------------------------------------------
    // 初始化
    // --------------------------------------------------------

    CurrentComboIndex = 0;
    bComboInputBuffered = false;

    if (ACombatCharacterBase* Character =
        Cast<ACombatCharacterBase>(
            GetAvatarActorFromActorInfo()
        ))
    {
        Character->SetAttackDamageMultiplier(
            GetCurrentDamageMultiplier()
        );
    }

    // --------------------------------------------------------
    // 播放 Montage
    // --------------------------------------------------------

    MontageTask =
        UAbilityTask_PlayMontageAndWait::
        CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            AttackMontage,
            PlayRate,
            ComboSections[0],
            true,
            1.0f,
            0.0f,
            false
        );


    if (!MontageTask)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Combo] Failed to create MontageTask")
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
        &UGA_ComboAttack::OnMontageCompleted
    );

    MontageTask->OnCancelled.AddDynamic(
        this,
        &UGA_ComboAttack::OnMontageCancelled
    );

    MontageTask->OnInterrupted.AddDynamic(
        this,
        &UGA_ComboAttack::OnMontageInterrupted
    );
    /*if (!ComboTurnEventTag.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Combo] ComboTurnEventTag invalid")
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
    ComboTurnEventTask =
        UAbilityTask_WaitGameplayEvent::
        WaitGameplayEvent(
            this,
            ComboTurnEventTag,
            nullptr,
            false,
            true
        );

    if (ComboTurnEventTask)
    {
        ComboTurnEventTask->EventReceived.AddDynamic(
            this,
            &UGA_ComboAttack::OnComboTurnEvent
        );

        ComboTurnEventTask->ReadyForActivation();
    }*/
    MontageTask->ReadyForActivation();
}


// ============================================================
// 后续输入
// ============================================================

void UGA_ComboAttack::InputPressed(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    Super::InputPressed(
        Handle,
        ActorInfo,
        ActivationInfo
    );

    const bool bWindowOpen =
        IsComboWindowOpen();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Combo] InputPressed | Index=%d | Window=%d | Grace=%d"
        ),
        CurrentComboIndex,
        bWindowOpen,
        bInGracePeriod
    );

    // 必须处于正常 ComboWindow 或 GracePeriod
    if (!bWindowOpen && !bInGracePeriod)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Combo] Input rejected")
        );

        return;
    }

    // 已经是最后一段
    if (CurrentComboIndex >= ComboSections.Num() - 1)
    {
        return;
    }

    // 防止重复缓存
    if (bComboInputBuffered)
    {
        return;
    }

    bComboInputBuffered = true;

    // 缓存玩家当前移动方向
    CacheComboDirection();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Combo] Input Buffered | Index=%d"),
        CurrentComboIndex
    );

    // 如果动画已经播放完，现在处于 Grace Period
    // 那么输入后立即进入下一段
    if (bInGracePeriod)
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(
                ComboGraceTimerHandle
            );
        }

        bInGracePeriod = false;

        AdvanceCombo();
    }
}

// ============================================================
// Advance Combo
// ============================================================

void UGA_ComboAttack::TryAdvanceCombo()
{
    if (!bComboInputBuffered)
    {
        return;
    }

    const int32 NextComboIndex =
        CurrentComboIndex + 1;

    if (!ComboSections.IsValidIndex(NextComboIndex))
    {
        return;
    }

    if (!SetNextComboSection())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Combo] SetNextComboSection FAILED")
        );

        return;
    }

    bComboInputBuffered = false;

    CurrentComboIndex =
        NextComboIndex;
}
// ============================================================
// Set Next Section
// ============================================================

bool UGA_ComboAttack::JumpToComboSection(
    int32 ComboIndex)
{
    if (!AttackMontage)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Combo] AttackMontage is null")
        );

        return false;
    }

    if (!ComboSections.IsValidIndex(ComboIndex))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Combo] Invalid ComboIndex: %d"),
            ComboIndex
        );

        return false;
    }

    if (!CurrentActorInfo)
    {
        return false;
    }

    ACharacter* Character =
        Cast<ACharacter>(
            CurrentActorInfo->AvatarActor.Get()
        );

    if (!Character)
    {
        return false;
    }

    UAnimInstance* AnimInstance =
        Character->GetMesh()
        ? Character->GetMesh()->GetAnimInstance()
        : nullptr;

    if (!AnimInstance)
    {
        return false;
    }

    const FName TargetSection =
        ComboSections[ComboIndex];

    /*
     * 额外检查：
     * Montage 中是不是真的存在这个 Section。
     */
    const int32 SectionIndex =
        AttackMontage->GetSectionIndex(TargetSection);
    const int32 MontageSectionIndex =
        AttackMontage->GetSectionIndex(TargetSection);
    if (MontageSectionIndex == INDEX_NONE)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Combo] Section NOT FOUND: %s"),
            *TargetSection.ToString()
        );

        return false;
    }

    float SectionStartTime = 0.0f;
    float SectionEndTime = 0.0f;

    AttackMontage->GetSectionStartAndEndTime(
        MontageSectionIndex,
        SectionStartTime,
        SectionEndTime
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Combo] Target=%s | Index=%d | Start=%.3f | End=%.3f | Length=%.3f"
        ),
        *TargetSection.ToString(),
        MontageSectionIndex,
        SectionStartTime,
        SectionEndTime,
        SectionEndTime - SectionStartTime
    );

    if (SectionIndex == INDEX_NONE)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "[Combo] Montage DOES NOT contain section: %s"
            ),
            *TargetSection.ToString()
        );

        return false;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Combo] JumpToSection: %s | MontageSectionIndex=%d"
        ),
        *TargetSection.ToString(),
        SectionIndex
    );

    AnimInstance->Montage_JumpToSection(
        TargetSection,
        AttackMontage
    );

    return true;
}


// ============================================================
// Window
// ============================================================

bool UGA_ComboAttack::IsComboWindowOpen() const
{
    if (!CurrentActorInfo ||
        !CurrentActorInfo->AbilitySystemComponent.IsValid())
    {
        return false;
    }


    return CurrentActorInfo
        ->AbilitySystemComponent
        ->HasMatchingGameplayTag(
            ComboWindowTag
        );
}


// ============================================================
// Montage Callback
// ============================================================

void UGA_ComboAttack::OnMontageCompleted()
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Combo] Montage Completed | Index=%d | Buffered=%d"
        ),
        CurrentComboIndex,
        bComboInputBuffered
    );

    // 已经是最后一段
    if (CurrentComboIndex >= ComboSections.Num() - 1)
    {
        FinishCombo();
        return;
    }

    // 动画播放期间已经按了下一次攻击
    if (bComboInputBuffered)
    {
        AdvanceCombo();
        return;
    }

    // 动画结束时还没有输入
    // 不马上结束 Ability，进入宽限期
    StartComboGracePeriod();
}


void UGA_ComboAttack::OnMontageCancelled()
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


void UGA_ComboAttack::OnMontageInterrupted()
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


// ============================================================
// End
// ============================================================

void UGA_ComboAttack::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Combo] EndAbility | Cancelled=%d"),
        bWasCancelled
    );

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(
            ComboGraceTimerHandle
        );
    }

    CurrentComboIndex = 0;
    bComboInputBuffered = false;
    bInGracePeriod = false;

    MontageTask = nullptr;

    BufferedComboDirection = FVector::ZeroVector;
    bHasBufferedComboDirection = false;

    //ComboTurnEventTask = nullptr;

    Super::EndAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        bReplicateEndAbility,
        bWasCancelled
    );
}

bool UGA_ComboAttack::SetNextComboSection()
{
    if (!AttackMontage ||
        !CurrentActorInfo)
    {
        return false;
    }

    ACharacter* Character =
        Cast<ACharacter>(
            CurrentActorInfo->AvatarActor.Get()
        );

    if (!Character)
    {
        return false;
    }

    USkeletalMeshComponent* Mesh =
        Character->GetMesh();

    if (!Mesh)
    {
        return false;
    }

    UAnimInstance* AnimInstance =
        Mesh->GetAnimInstance();

    if (!AnimInstance)
    {
        return false;
    }

    const int32 NextComboIndex =
        CurrentComboIndex + 1;

    if (!ComboSections.IsValidIndex(CurrentComboIndex) ||
        !ComboSections.IsValidIndex(NextComboIndex))
    {
        return false;
    }

    const FName CurrentSection =
        ComboSections[CurrentComboIndex];

    const FName NextSection =
        ComboSections[NextComboIndex];

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Combo] SetNextSection NOW | %s -> %s"
        ),
        *CurrentSection.ToString(),
        *NextSection.ToString()
    );

    AnimInstance->Montage_SetNextSection(
        CurrentSection,
        NextSection,
        AttackMontage
    );

    return true;
}

void UGA_ComboAttack::ApplyComboDirection()
{
    if (!CurrentActorInfo ||
        !CurrentActorInfo->AvatarActor.IsValid())
    {
        return;
    }

    AGASTestCharacter* Character =
        Cast<AGASTestCharacter>(
            CurrentActorInfo->AvatarActor.Get()
        );

    if (!Character)
    {
        return;
    }


    // ========================================
    // 优先级 1：锁定目标
    // ========================================

    if (Character->FaceLockedTarget())
    {
        // 锁定情况下不再使用玩家缓存的移动方向
        BufferedComboDirection =
            FVector::ZeroVector;

        bHasBufferedComboDirection = false;

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Combo] Direction -> Locked Target")
        );

        return;
    }


    // ========================================
    // 优先级 2：原来的移动输入方向
    // ========================================

    if (!bHasBufferedComboDirection)
    {
        return;
    }

    FVector Direction =
        BufferedComboDirection;

    Direction.Z = 0.0f;

    if (Direction.IsNearlyZero())
    {
        return;
    }

    Direction.Normalize();

    FRotator TargetRotation =
        Direction.Rotation();

    TargetRotation.Pitch = 0.0f;
    TargetRotation.Roll = 0.0f;

    Character->SetActorRotation(
        TargetRotation
    );


    BufferedComboDirection =
        FVector::ZeroVector;

    bHasBufferedComboDirection = false;
}

void UGA_ComboAttack::CacheComboDirection()
{
    if (!CurrentActorInfo ||
        !CurrentActorInfo->AvatarActor.IsValid())
    {
        return;
    }

    AGASTestCharacter* Character =
        Cast<AGASTestCharacter>(
            CurrentActorInfo->AvatarActor.Get()
        );

    if (!Character)
    {
        return;
    }

    FVector MoveDirection =
        Character->GetLastMoveInputDirection();

    MoveDirection.Z = 0.0f;

    // 玩家没有移动输入
    if (MoveDirection.IsNearlyZero())
    {
        BufferedComboDirection =
            FVector::ZeroVector;

        bHasBufferedComboDirection = false;

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Combo] No direction buffered")
        );

        return;
    }

    BufferedComboDirection =
        MoveDirection.GetSafeNormal();

    bHasBufferedComboDirection = true;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Combo] Direction Buffered = %s"),
        *BufferedComboDirection.ToString()
    );
}

//void UGA_ComboAttack::OnComboTurnEvent(
//    FGameplayEventData Payload)
//{
//    UE_LOG(
//        LogTemp,
//        Warning,
//        TEXT("[Combo] ComboTurn Event Received")
//    );
//
//    ApplyComboDirection();
//}

void UGA_ComboAttack::StartComboGracePeriod()
{
    if (!GetWorld())
    {
        FinishCombo();
        return;
    }

    bInGracePeriod = true;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Combo] Grace Period Start | %.2fs"),
        ComboGraceTime
    );

    GetWorld()->GetTimerManager().SetTimer(
        ComboGraceTimerHandle,
        this,
        &UGA_ComboAttack::OnComboGracePeriodExpired,
        ComboGraceTime,
        false
    );
}

void UGA_ComboAttack::OnComboGracePeriodExpired()
{
    bInGracePeriod = false;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Combo] Grace Period Expired | Buffered=%d"
        ),
        bComboInputBuffered
    );

    if (bComboInputBuffered)
    {
        AdvanceCombo();
        return;
    }

    FinishCombo();
}

void UGA_ComboAttack::AdvanceCombo()
{
    bInGracePeriod = false;
    bComboInputBuffered = false;

    ++CurrentComboIndex;

    if (!ComboSections.IsValidIndex(CurrentComboIndex))
    {
        FinishCombo();
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Combo] Advance -> Index=%d | Section=%s"
        ),
        CurrentComboIndex,
        *ComboSections[CurrentComboIndex].ToString()
    );

    // 在下一段攻击开始之前应用之前缓存的移动方向
    ApplyComboDirection();

    MontageTask =
        UAbilityTask_PlayMontageAndWait::
        CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            AttackMontage,
            PlayRate,
            ComboSections[CurrentComboIndex],
            true,
            1.0f,
            0.0f,
            false
        );

    if (!MontageTask)
    {
        FinishCombo();
        return;
    }

    MontageTask->OnCompleted.AddDynamic(
        this,
        &UGA_ComboAttack::OnMontageCompleted
    );

    MontageTask->OnCancelled.AddDynamic(
        this,
        &UGA_ComboAttack::OnMontageCancelled
    );

    MontageTask->OnInterrupted.AddDynamic(
        this,
        &UGA_ComboAttack::OnMontageInterrupted
    );

    ACombatCharacterBase* Character =
        Cast<ACombatCharacterBase>(
            GetAvatarActorFromActorInfo()
        );

    if (Character)
    {
        Character->SetAttackDamageMultiplier(
            GetCurrentDamageMultiplier()
        );
    }

    MontageTask->ReadyForActivation();
}
void UGA_ComboAttack::FinishCombo()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(
            ComboGraceTimerHandle
        );
    }

    bInGracePeriod = false;
    bComboInputBuffered = false;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Combo] Finish Combo")
    );

    EndAbility(
        CurrentSpecHandle,
        CurrentActorInfo,
        CurrentActivationInfo,
        true,
        false
    );
}
float UGA_ComboAttack::GetCurrentDamageMultiplier() const
{
    if (DamageMultipliers.IsValidIndex(CurrentComboIndex))
    {
        return DamageMultipliers[CurrentComboIndex];
    }

    return 1.0f;
}

