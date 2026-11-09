#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EnemyBrainComponent.generated.h"

class AEnemyCharacter;
class UEnemyVisionComponent;
class UEnemyNavigationComponent;
struct FEnemyObservation;

USTRUCT(BlueprintType)
struct GASTEST_API FEnemyMemory
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Memory")
    bool bHasLastSeenTarget = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Memory")
    FVector LastSeenPosition = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Memory")
    float TimeSinceLastSeen = 0.0f;
};

UENUM(BlueprintType)
enum class EEnemyAIState : uint8
{
    Idle,
    Approach,
    Attack,
    Search
};

enum class EEnemySearchPhase : uint8
{
    MoveToLastSeen,
    LookAround
};
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class GASTEST_API UEnemyBrainComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnemyBrainComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (ClampMin = "0.01", Units = "s"))
    float ThinkInterval = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (ClampMin = "0.0", Units = "cm"))
    float AttackRange = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (ClampMin = "0.01", Units = "s"))
    float AttackCooldown = 1.5f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI")
    EEnemyAIState CurrentState = EEnemyAIState::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "AI|Memory")
    FEnemyMemory Memory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Memory", meta = (ClampMin = "0.0", Units = "s"))
    float MemoryForgetTime = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Memory", meta = (ClampMin = "0.0", Units = "cm"))
    float SearchArrivalDistance = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Debug")
    bool bDrawMemoryDebug = false;

    UPROPERTY(EditAnywhere, Category = "AI|Search")
    float LookSpeed = 2.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Search")
    float MaxLookYaw = 60.0f;

    UPROPERTY(EditAnywhere, Category = "AI|Search")
    float LookAroundDuration = 4.0f;



protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    void Think();
    void UpdateMemory(const FEnemyObservation& Observation);
    void ExecuteSearch(float DeltaTime);
    void SetState(EEnemyAIState NewState);
    void ExecuteLookAround(float DeltaTime);

    UPROPERTY(Transient)
    TObjectPtr<AEnemyCharacter> OwnerEnemy;

    UPROPERTY(Transient)
    TObjectPtr<UEnemyVisionComponent> VisionComponent;

    UPROPERTY(Transient)
    TObjectPtr<UEnemyNavigationComponent> NavigationComponent;

    // An actuator command held between decisions, never a remembered target position.
    FVector MoveDirection = FVector::ZeroVector;
    float ThinkElapsed = 0.0f;
    double NextAttackTime = 0.0;
    double LastSeenObservationTime = 0.0;
    bool bWasTargetVisible = false;
    EEnemySearchPhase SearchPhase =
        EEnemySearchPhase::MoveToLastSeen;
    float SearchLookTimer = 0.0f;
    
};
