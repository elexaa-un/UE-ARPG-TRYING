#include "UANS_AttachWindow.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"


void UUANS_AttachWindow::NotifyBegin(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    float TotalDuration,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(
        MeshComp,
        Animation,
        TotalDuration,
        EventReference
    );

    // =========================
    // 基础检查
    // =========================

    if (!MeshComp)
    {
        return;
    }

    UWorld* World = MeshComp->GetWorld();

    /*
     * Montage / Animation Editor 预览时
     * Owner 是 AnimationEditorPreviewActor，
     * 它没有 ASC。
     *
     * Combo Window 只应该在真正的游戏世界执行。
     */
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    AActor* Owner = MeshComp->GetOwner();

    if (!Owner)
    {
        return;
    }

    UAbilitySystemComponent* ASC =
        Owner->FindComponentByClass<UAbilitySystemComponent>();

    if (!ASC)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[ComboWindow] ASC not found | Owner=%s"),
            *GetNameSafe(Owner)
        );

        return;
    }

    // =========================
    // 检查 Window Tag
    // =========================

    if (!AttackWindowTag.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[ComboWindow] AttackWindowTag invalid")
        );

        return;
    }

    // =========================
    // 打开 Combo Window
    // =========================

    ASC->AddLooseGameplayTag(
        AttackWindowTag
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[ComboWindow] OPEN | Owner=%s | Tag=%s"
        ),
        *GetNameSafe(Owner),
        *AttackWindowTag.ToString()
    );
}


void UUANS_AttachWindow::NotifyEnd(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(
        MeshComp,
        Animation,
        EventReference
    );

    // =========================
    // 基础检查
    // =========================

    if (!MeshComp)
    {
        return;
    }

    UWorld* World = MeshComp->GetWorld();

    // 忽略 Animation Editor Preview
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    AActor* Owner = MeshComp->GetOwner();

    if (!Owner)
    {
        return;
    }

    UAbilitySystemComponent* ASC =
        Owner->FindComponentByClass<UAbilitySystemComponent>();

    if (!ASC)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[ComboWindow] ASC not found | Owner=%s"),
            *GetNameSafe(Owner)
        );

        return;
    }

    // =========================
    // 关闭 Combo Window
    // =========================

    if (AttackWindowTag.IsValid())
    {
        ASC->RemoveLooseGameplayTag(
            AttackWindowTag
        );

        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "[ComboWindow] CLOSE | Owner=%s | Tag=%s"
            ),
            *GetNameSafe(Owner),
            *AttackWindowTag.ToString()
        );
    }

    // =========================
    // 通知 GA Window 已关闭
    // =========================

    if (!WindowClosedEventTag.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[ComboWindow] WindowClosedEventTag invalid")
        );

        return;
    }

    FGameplayEventData Payload;

    Payload.EventTag =
        WindowClosedEventTag;

    Payload.Instigator =
        Owner;

    Payload.Target =
        Owner;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[ComboWindow] Sent Closed Event | Tag=%s"
        ),
        *WindowClosedEventTag.ToString()
    );
}