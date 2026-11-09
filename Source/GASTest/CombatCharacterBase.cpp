 #include "CombatCharacterBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"

ACombatCharacterBase::ACombatCharacterBase()
{
    PrimaryActorTick.bCanEverTick = false;

    AbilitySystemComponent =
        CreateDefaultSubobject<UAbilitySystemComponent>(
            TEXT("AbilitySystemComponent")
        );

    check(AbilitySystemComponent);

    AttributeSet =
        CreateDefaultSubobject<UAttributeSet_Common>(
            TEXT("AttributeSet")
        );

    check(AttributeSet);
}

void ACombatCharacterBase::BeginPlay()
{
    Super::BeginPlay();

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(
            this,
            this
        );

        AbilitySystemComponent
            ->GetGameplayAttributeValueChangeDelegate(
                UAttributeSet_Common::GetCurrentHealthAttribute()
            )
            .AddUObject(
                this,
                &ACombatCharacterBase::HandleHealthChanged
            );
    }

    AddCharacterAbilities();
}

void ACombatCharacterBase::AddCharacterAbilities()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    for (TSubclassOf<UGameplayAbility>& Ability : DefaultAbilities)
    {
        if (!Ability)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("'%s' tried to add a null ability."),
                *GetNameSafe(this)
            );

            continue;
        }

        AbilitySystemComponent->GiveAbility(
            FGameplayAbilitySpec(
                Ability,
                1,
                INDEX_NONE,
                this
            )
        );

        UE_LOG(
            LogTemp,
            Log,
            TEXT("'%s' Granted ability '%s'"),
            *GetNameSafe(this),
            *Ability->GetName()
        );
    }
}

bool ACombatCharacterBase::HasGameplayTag(
    FGameplayTag TagToCheck
) const
{
    return
        AbilitySystemComponent &&
        TagToCheck.IsValid() &&
        AbilitySystemComponent->HasMatchingGameplayTag(
            TagToCheck
        );
}

void ACombatCharacterBase::HandleMeleeHit(
    AActor* TargetActor,
    const FHitResult OriginHit,
    float DamageMultiplier
)
{
    if (!TargetActor ||
        TargetActor == this ||
        !AbilitySystemComponent ||
        !AttributeSet)
    {
        return;
    }

    if (!DamageEffectClass)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Damage] DamageEffectClass is null")
        );

        return;
    }

    IAbilitySystemInterface* TargetASI =
        Cast<IAbilitySystemInterface>(TargetActor);

    if (!TargetASI)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "[Damage] Target has no AbilitySystemInterface: %s"
            ),
            *GetNameSafe(TargetActor)
        );

        return;
    }

    UAbilitySystemComponent* TargetASC =
        TargetASI->GetAbilitySystemComponent();

    if (!TargetASC)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Damage] Target %s has no ASC"),
            *GetNameSafe(TargetActor)
        );

        return;
    }

    FGameplayEffectContextHandle EffectContext =
        AbilitySystemComponent->MakeEffectContext();

    EffectContext.AddSourceObject(this);
    EffectContext.AddHitResult(OriginHit, true);

    FGameplayEffectSpecHandle SpecHandle =
        AbilitySystemComponent->MakeOutgoingSpec(
            DamageEffectClass,
            1.0f,
            EffectContext
        );

    if (!SpecHandle.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Damage] Failed to create GameplayEffectSpec")
        );

        return;
    }

    const float CurrentAttack =
        AttributeSet->GetAttackPower();

    const float Damage =
        CurrentAttack * DamageMultiplier;

    const FGameplayTag DamageTag =
        FGameplayTag::RequestGameplayTag(
            TEXT("Data.Damage")
        );

    SpecHandle.Data->SetSetByCallerMagnitude(
        DamageTag,
        -Damage
    );

    AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(
        *SpecHandle.Data.Get(),
        TargetASC
    );

    const float NewHealth =
        TargetASC->GetNumericAttribute(
            UAttributeSet_Common::GetCurrentHealthAttribute()
        );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Damage] Target=%s | Damage=%.1f | Health=%.1f"
        ),
        *GetNameSafe(TargetActor),
        Damage,
        NewHealth
    );

    // ================================
    // Hit React Event
    // ================================

    const FGameplayTag HitReactTag =
        FGameplayTag::RequestGameplayTag(
            TEXT("Event.HitReact")
        );

    FGameplayEventData EventData;

    EventData.EventTag = HitReactTag;
    EventData.Instigator = this;
    EventData.Target = TargetActor;
    EventData.ContextHandle = EffectContext;

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        TargetActor,
        HitReactTag,
        EventData
    );
}

void ACombatCharacterBase::HandleHealthChanged(
    const FOnAttributeChangeData& Data
)
{
    if (!AttributeSet)
    {
        return;
    }

    OnHealthChanged.Broadcast(
        Data.NewValue,
        AttributeSet->GetMaxHealth()
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Health] %s | %.1f / %.1f"),
        *GetNameSafe(this),
        Data.NewValue,
        AttributeSet->GetMaxHealth()
    );
}

float ACombatCharacterBase::GetHealthPercent() const
{
    if (!AttributeSet)
    {
        return 0.0f;
    }

    const float MaxHealth =
        AttributeSet->GetMaxHealth();

    if (MaxHealth <= 0.0f)
    {
        return 0.0f;
    }

    return FMath::Clamp(
        AttributeSet->GetCurrentHealth() / MaxHealth,
        0.0f,
        1.0f
    );
}

void ACombatCharacterBase::SetAttackDamageMultiplier(
    float NewMultiplier
)
{
    CurrentAttackDamageMultiplier =
        FMath::Max(
            0.0f,
            NewMultiplier
        );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[Damage] Set Attack Multiplier = %.2f"
        ),
        CurrentAttackDamageMultiplier
    );
}

void ACombatCharacterBase::MoveInWorldDirection(
    const FVector& WorldDirection,
    float ScaleValue
)
{
    FVector Direction = WorldDirection;
    Direction.Z = 0.0f;

    if (Direction.IsNearlyZero() ||
        FMath::IsNearlyZero(ScaleValue))
    {
        StopMovementInput();
        return;
    }

    Direction.Normalize();

    CurrentMoveInputDirection = Direction;
    bHasMoveInput = true;

    AddMovementInput(Direction, ScaleValue);
}

void ACombatCharacterBase::StopMovementInput()
{
    CurrentMoveInputDirection = FVector::ZeroVector;
    bHasMoveInput = false;
}