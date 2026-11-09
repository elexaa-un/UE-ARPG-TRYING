#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "UANS_AttackTrace.generated.h"

UCLASS()
class GASTEST_API UUANS_AttackTrace : public UAnimNotifyState
{
    GENERATED_BODY()

public:

    virtual void NotifyBegin(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        float TotalDuration,
        const FAnimNotifyEventReference& EventReference
    ) override;

    virtual void NotifyTick(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        float FrameDeltaTime,
        const FAnimNotifyEventReference& EventReference
    ) override;

    virtual void NotifyEnd(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference
    ) override;

protected:

    /**
     * 当前攻击使用的骨骼/Socket。
     *
     * UE Manny:
     * hand_r
     * hand_l
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackTrace")
    FName TraceSocketName = TEXT("hand_r");

    /**
     * 拳头碰撞半径。
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackTrace")
    float TraceRadius = 15.0f;

    /**
     * 调试绘制。
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackTrace")
    bool bDrawDebug = true;

private:

    /**
     * 上一帧拳头位置。
     */
    FVector PreviousLocation = FVector::ZeroVector;

    /**
     * 这一拳已经命中的 Actor。
     *
     * 防止一个攻击窗口重复造成伤害。
     */
    TSet<TWeakObjectPtr<AActor>> HitActors;
};