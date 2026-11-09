// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EnemyNavigationComponent.generated.h"

class AEnemyCharacter;

UCLASS( ClassGroup=(AI), meta=(BlueprintSpawnableComponent) )
class GASTEST_API UEnemyNavigationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEnemyNavigationComponent();

	FVector FindBestDirection(FVector originPosition, FVector targetPosition);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UPROPERTY(Transient)
	TObjectPtr<AEnemyCharacter> OwnerEnemy;

};
