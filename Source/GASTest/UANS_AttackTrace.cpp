#include "UANS_AttackTrace.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CombatCharacterBase.h"

void UUANS_AttackTrace::NotifyBegin(
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

    if (!MeshComp)
    {
        return;
    }

    UWorld* World = MeshComp->GetWorld();

    // 防止 Animation Editor 预览触发
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    if (!MeshComp->DoesSocketExist(TraceSocketName))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[AttackTrace] Socket/Bone not found: %s"),
            *TraceSocketName.ToString()
        );

        return;
    }

    // 每一拳重新清空命中记录
    HitActors.Empty();

    // 记录攻击窗口开始时的位置
    PreviousLocation =
        MeshComp->GetSocketLocation(TraceSocketName);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[AttackTrace] BEGIN | Socket=%s"),
        *TraceSocketName.ToString()
    );
}


void UUANS_AttackTrace::NotifyTick(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    float FrameDeltaTime,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(
        MeshComp,
        Animation,
        FrameDeltaTime,
        EventReference
    );

    if (!MeshComp)
    {
        return;
    }

    UWorld* World = MeshComp->GetWorld();

    if (!World || !World->IsGameWorld())
    {
        return;
    }

    AActor* Owner = MeshComp->GetOwner();

    if (!Owner)
    {
        return;
    }

    const FVector CurrentLocation =
        MeshComp->GetSocketLocation(TraceSocketName);

    FCollisionQueryParams QueryParams;

    // 不检测攻击者自己
    QueryParams.AddIgnoredActor(Owner);

    TArray<FHitResult> HitResults;

    const FCollisionShape Sphere =
        FCollisionShape::MakeSphere(TraceRadius);

    const bool bHit = World->SweepMultiByChannel(
        HitResults,
        PreviousLocation,
        CurrentLocation,
        FQuat::Identity,
        ECC_Pawn,
        Sphere,
        QueryParams
    );

    if (bDrawDebug)
    {
        DrawDebugLine(
            World,
            PreviousLocation,
            CurrentLocation,
            FColor::Red,
            false,
            0.1f,
            0,
            2.0f
        );

        DrawDebugSphere(
            World,
            CurrentLocation,
            TraceRadius,
            12,
            FColor::Red,
            false,
            0.1f
        );
    }

    if (bHit)
    {
        for (const FHitResult& Hit : HitResults)
        {
            AActor* HitActor = Hit.GetActor();

            if (!HitActor)
            {
                continue;
            }

            // 这一拳已经打过它
            if (HitActors.Contains(HitActor))
            {
                continue;
            }

            HitActors.Add(HitActor);

            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[AttackTrace] Hit: %s"),
                *GetNameSafe(HitActor)
            );

            ACombatCharacterBase* Attacker =
                Cast<ACombatCharacterBase>(Owner);

            if (Attacker)
            {
                Attacker->HandleMeleeHit(HitActor,Hit,Attacker->GetAttackDamageMultiplier());
            }
        }
    }

    // 非常重要
    PreviousLocation = CurrentLocation;
}


void UUANS_AttackTrace::NotifyEnd(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(
        MeshComp,
        Animation,
        EventReference
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[AttackTrace] END")
    );

    HitActors.Empty();
}