// Fill out your copyright notice in the Description page of Project Settings.


#include "AttributeSet_Common.h"

#include "Net/UnrealNetwork.h"

UAttributeSet_Common::UAttributeSet_Common()
{
	MaxHealth = 100.0f;
	CurrentHealth = 100.0f;
	MaxStamina = 100.0f;
	CurrentStamina = 100.0f;
	AttackPower = 10.0f;
}

void UAttributeSet_Common::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSet_Common, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSet_Common, CurrentHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSet_Common, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSet_Common, CurrentStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSet_Common, AttackPower, COND_None, REPNOTIFY_Always);
}

void UAttributeSet_Common::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.1f);
	}
	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.1f);
	}
	if (Attribute == GetCurrentStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	if (Attribute == GetAttackPowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UAttributeSet_Common::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	if (Attribute == GetMaxHealthAttribute())
	{
		if (GetCurrentHealth() > NewValue)
		{
			SetCurrentHealth(NewValue);
		}
	}
	if (Attribute == GetCurrentHealthAttribute())
	{
		if (GetCurrentHealth() <= 0.0f)
		{
			//如果生命值小于等于0，执行一些逻辑，比如角色死亡等等
		}
	}
	if (Attribute == GetMaxStaminaAttribute())
	{
		if (GetCurrentStamina() > NewValue)
		{
			SetCurrentStamina(NewValue);
		}
	}
	if (Attribute == GetCurrentStaminaAttribute())
	{
		if (GetCurrentStamina() < 0.0f)
		{
			SetCurrentStamina(0.0f);
		}
	}
}