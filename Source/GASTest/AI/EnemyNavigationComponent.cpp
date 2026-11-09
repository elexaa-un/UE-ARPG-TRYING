// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyCharacter.h"
#include "AI/EnemyNavigationComponent.h"

// Sets default values for this component's properties
UEnemyNavigationComponent::UEnemyNavigationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UEnemyNavigationComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerEnemy = Cast<AEnemyCharacter>(GetOwner());
	
}
void UEnemyNavigationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

FVector UEnemyNavigationComponent::FindBestDirection(
    FVector OriginPosition,
    FVector TargetPosition)
{
    UNavigationSystemV1* NavSystem =
        FNavigationSystem::GetCurrent<UNavigationSystemV1>(
            GetWorld()
        );

    if (!NavSystem)
    {
        return FVector::ZeroVector;
    }

    UNavigationPath* Path =
        NavSystem->FindPathToLocationSynchronously(
            GetWorld(),
            OriginPosition,
            TargetPosition,
            OwnerEnemy.Get()
        );

    if (!Path || !Path->IsValid())
    {
        return FVector::ZeroVector;
    }

    if (Path->PathPoints.Num() < 2)
    {
        return FVector::ZeroVector;
    }

    FVector Direction =
        Path->PathPoints[1] - OriginPosition;

    Direction.Z = 0.0f;

    return Direction.GetSafeNormal();
}

