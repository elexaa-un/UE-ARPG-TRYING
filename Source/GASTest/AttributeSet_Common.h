// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet_Common.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


UCLASS()
class GASTEST_API UAttributeSet_Common : public UAttributeSet
{
	GENERATED_BODY()

public:
	UAttributeSet_Common();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Common, MaxHealth)

		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attributes")
	FGameplayAttributeData CurrentHealth;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Common, CurrentHealth)

		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attributes")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Common, MaxStamina)

		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attributes")
	FGameplayAttributeData CurrentStamina;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Common, CurrentStamina)

		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attributes")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UAttributeSet_Common, AttackPower)

};