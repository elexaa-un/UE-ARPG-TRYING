#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "UANS_AttachWindow.generated.h"


UCLASS()
class GASTEST_API UUANS_AttachWindow :
    public UAnimNotifyState
{
    GENERATED_BODY()


public:

    virtual void NotifyBegin(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        float TotalDuration,
        const FAnimNotifyEventReference& EventReference
    ) override;


    virtual void NotifyEnd(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference
    ) override;


protected:

    /**
     * State.Attack.ComboWindow
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    FGameplayTag AttackWindowTag;


    /**
     * Event.Attack.ComboWindowClosed
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    FGameplayTag WindowClosedEventTag;
};