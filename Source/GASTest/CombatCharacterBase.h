#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet_Common.h"
#include "CombatCharacterBase.generated.h"

class UGameplayEffect;
class UGameplayAbility;

UCLASS(Abstract)
class GASTEST_API ACombatCharacterBase
    : public ACharacter,
    public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ACombatCharacterBase();

    // ================================
    // GAS
    // ================================

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
    {
        return AbilitySystemComponent;
    }

    FORCEINLINE UAttributeSet_Common* GetAttributeSet() const
    {
        return AttributeSet;
    }

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void AddCharacterAbilities();

    UFUNCTION(BlueprintPure, Category = "GAS")
    bool HasGameplayTag(FGameplayTag TagToCheck) const;

    // ================================
    // Combat
    // ================================

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void HandleMeleeHit(
        AActor* TargetActor,
        const FHitResult OriginHit,
        float DamageMultiplier = 1.0f
    );

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void SetAttackDamageMultiplier(float NewMultiplier);

    UFUNCTION(BlueprintPure, Category = "Combat")
    float GetAttackDamageMultiplier() const
    {
        return CurrentAttackDamageMultiplier;
    }

    // ================================
    // Health
    // ================================

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FOnHealthChanged,
        float, CurrentHealth,
        float, MaxHealth
    );

    UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
    FOnHealthChanged OnHealthChanged;

    UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
    float GetHealthPercent() const;

    // ================================
   // Movement
   // ================================

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveInWorldDirection(
        const FVector& WorldDirection,
        float ScaleValue = 1.0f
    );

    UFUNCTION(BlueprintCallable, Category = "Movement")
    virtual void StopMovementInput();

    UFUNCTION(BlueprintPure, Category = "Movement")
    FVector GetLastMoveInputDirection() const
    {
        return CurrentMoveInputDirection;
    }

    UFUNCTION(BlueprintPure, Category = "Movement")
    bool HasMoveInput() const
    {
        return bHasMoveInput;
    }

protected:
    virtual void BeginPlay() override;

    // ================================
    // GAS Components
    // ================================

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Abilities"
    )
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Abilities"
    )
    TObjectPtr<UAttributeSet_Common> AttributeSet;

    // ================================
    // Ability Configuration
    // ================================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Abilities"
    )
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // ================================
    // Damage
    // ================================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Combat"
    )
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    // 当前攻击使用的伤害倍率。
    // 由具体 GameplayAbility 在攻击开始前设置。
    // AttackTrace 命中时读取该值。
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Combat|Damage"
    )
    float AttackDamageMultiplier = 1.0f;

private:
    void HandleHealthChanged(const FOnAttributeChangeData& Data);

    UPROPERTY(
        BlueprintReadOnly,
        Category = "Combat",
        meta = (AllowPrivateAccess = "true")
    )
    float CurrentAttackDamageMultiplier = 1.0f;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Movement",
        meta = (AllowPrivateAccess = "true")
    )
    FVector CurrentMoveInputDirection = FVector::ZeroVector;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Movement",
        meta = (AllowPrivateAccess = "true")
    )
    bool bHasMoveInput = false;
};