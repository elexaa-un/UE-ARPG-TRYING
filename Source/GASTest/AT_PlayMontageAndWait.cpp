#include "AT_PlayMontageAndWait.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"


UAT_PlayMontageAndWait* UAT_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
    UGameplayAbility* OwningAbility,
    FName TaskInstanceName,
    UAnimMontage* MontageToPlay,
    float Rate,
    FName StartSection,
    bool bStopWhenAbilityEnds,
    float AnimRootMotionTranslationScale,
    float StartTimeSeconds,
    bool bAllowInterruptAfterBlendOut)
{
    UAT_PlayMontageAndWait* MyObj =
        NewAbilityTask<UAT_PlayMontageAndWait>(
            OwningAbility,
            TaskInstanceName
        );

    MyObj->MontageToPlay = MontageToPlay;
    MyObj->Rate = Rate;
    MyObj->StartSection = StartSection;
    MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
    MyObj->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
    MyObj->StartTimeSeconds = StartTimeSeconds;
    MyObj->bAllowInterruptAfterBlendOut = bAllowInterruptAfterBlendOut;

    return MyObj;
}


void UAT_PlayMontageAndWait::Activate()
{
    if (!Ability)
    {
        EndTask();
        return;
    }

    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();

    if (!ASC || !MontageToPlay)
    {
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnCancelled.Broadcast();
        }

        EndTask();
        return;
    }

    const float MontageLength = ASC->PlayMontage(
        Ability,
        Ability->GetCurrentActivationInfo(),
        MontageToPlay,
        Rate,
        StartSection,
        StartTimeSeconds
    );

    if (MontageLength <= 0.0f)
    {
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnCancelled.Broadcast();
        }

        EndTask();
        return;
    }

    // Ability 被取消时通知当前 Task。
    InterruptedHandle =
        Ability->OnGameplayAbilityCancelled.AddUObject(
            this,
            &UAT_PlayMontageAndWait::OnGameplayAbilityCancelled
        );

    UAnimInstance* AnimInstance = nullptr;

    if (AActor* AvatarActor = Ability->GetCurrentActorInfo()->AvatarActor.Get())
    {
        if (ACharacter* Character = Cast<ACharacter>(AvatarActor))
        {
            if (USkeletalMeshComponent* Mesh = Character->GetMesh())
            {
                AnimInstance = Mesh->GetAnimInstance();
            }
        }
    }

    if (!AnimInstance)
    {
        StopPlayingMontage();

        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnCancelled.Broadcast();
        }

        EndTask();
        return;
    }

    // Montage 开始 Blend In
    BlendedInDelegate.BindUObject(
        this,
        &UAT_PlayMontageAndWait::OnMontageBlendedIn
    );

    AnimInstance->Montage_SetBlendedInDelegate(
        BlendedInDelegate,
        MontageToPlay
    );

    // Montage 开始 Blend Out
    BlendingOutDelegate.BindUObject(
        this,
        &UAT_PlayMontageAndWait::OnMontageBlendingOut
    );

    AnimInstance->Montage_SetBlendingOutDelegate(
        BlendingOutDelegate,
        MontageToPlay
    );

    // Montage 完全结束
    MontageEndedDelegate.BindUObject(
        this,
        &UAT_PlayMontageAndWait::OnMontageEnded
    );

    AnimInstance->Montage_SetEndDelegate(
        MontageEndedDelegate,
        MontageToPlay
    );

    SetWaitingOnAvatar();
}


void UAT_PlayMontageAndWait::OnMontageBlendedIn(
    UAnimMontage* Montage)
{
    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnBlendedIn.Broadcast();
    }
}


void UAT_PlayMontageAndWait::OnMontageBlendingOut(
    UAnimMontage* Montage,
    bool bInterrupted)
{
    // 确保回调属于当前 Task 播放的 Montage。
    if (Montage != MontageToPlay)
    {
        return;
    }

    if (bInterrupted)
    {
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnInterrupted.Broadcast();
        }

        /*
         * 默认情况下，被打断以后 Task 已经完成使命。
         *
         * bAllowInterruptAfterBlendOut == true 时，
         * 我们允许 BlendOut 之后继续等待 End Delegate。
         */
        if (!bAllowInterruptAfterBlendOut)
        {
            EndTask();
        }

        return;
    }

    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnBlendOut.Broadcast();
    }
}


void UAT_PlayMontageAndWait::OnMontageInterrupted()
{
    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnInterrupted.Broadcast();
    }

    EndTask();
}


void UAT_PlayMontageAndWait::OnGameplayAbilityCancelled()
{
    // Ability 主动取消时停止当前 Montage。
    StopPlayingMontage();

    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnCancelled.Broadcast();
    }

    EndTask();
}


void UAT_PlayMontageAndWait::OnMontageEnded(
    UAnimMontage* Montage,
    bool bInterrupted)
{
    if (Montage != MontageToPlay)
    {
        return;
    }

    if (!bInterrupted)
    {
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnCompleted.Broadcast();
        }
    }
    else if (bAllowInterruptAfterBlendOut)
    {
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnInterrupted.Broadcast();
        }
    }

    EndTask();
}


void UAT_PlayMontageAndWait::ExternalCancel()
{
    OnGameplayAbilityCancelled();

    Super::ExternalCancel();
}


void UAT_PlayMontageAndWait::OnDestroy(bool AbilityEnded)
{
    /*
     * 如果 Ability 结束时要求 Montage 一起结束，
     * 就主动停止当前 Montage。
     */
    if (AbilityEnded && bStopWhenAbilityEnds)
    {
        StopPlayingMontage();
    }

    // 解除 Ability Cancel Delegate。
    if (Ability && InterruptedHandle.IsValid())
    {
        Ability->OnGameplayAbilityCancelled.Remove(
            InterruptedHandle
        );

        InterruptedHandle.Reset();
    }

    Super::OnDestroy(AbilityEnded);
}


bool UAT_PlayMontageAndWait::StopPlayingMontage()
{
    if (!Ability || !MontageToPlay)
    {
        return false;
    }

    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();

    if (!ASC)
    {
        return false;
    }

    /*
     * 防止这个 Task 停掉其他 Ability 后来播放的 Montage。
     *
     * 只有当前 ASC 的 AnimatingAbility 仍然是我们的 Ability，
     * 并且当前 Montage 仍然是 MontageToPlay，才允许停止。
     */
    if (ASC->GetAnimatingAbility() != Ability)
    {
        return false;
    }

    if (ASC->GetCurrentMontage() != MontageToPlay)
    {
        return false;
    }

    ASC->CurrentMontageStop();

    return true;
}


FString UAT_PlayMontageAndWait::GetDebugString() const
{
    const FString MontageName =
        MontageToPlay
        ? MontageToPlay->GetName()
        : TEXT("None");

    return FString::Printf(
        TEXT("PlayMontageAndWait. Montage: %s"),
        *MontageName
    );
}