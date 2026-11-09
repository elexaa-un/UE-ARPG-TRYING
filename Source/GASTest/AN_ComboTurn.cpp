#include "AN_ComboTurn.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Actor.h"

void UAN_ComboTurn::Notify(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(
        MeshComp,
        Animation,
        EventReference
    );

    if (!MeshComp)
    {
        return;
    }

    AActor* Owner =
        MeshComp->GetOwner();

    if (!Owner)
    {
        return;
    }

    if (!ComboTurnEventTag.IsValid())
    {
        return;
    }

    FGameplayEventData EventData;

    EventData.EventTag =
        ComboTurnEventTag;

    EventData.Instigator =
        Owner;

    EventData.Target =
        Owner;

    UAbilitySystemBlueprintLibrary::
        SendGameplayEventToActor(
            Owner,
            ComboTurnEventTag,
            EventData
        );
}