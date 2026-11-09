#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AN_ComboTurn.generated.h"

UCLASS()
class GASTEST_API UAN_ComboTurn
    : public UAnimNotify
{
    GENERATED_BODY()

public:

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combo"
    )
    FGameplayTag ComboTurnEventTag;

    virtual void Notify(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference
    ) override;
};