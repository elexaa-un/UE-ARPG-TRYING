#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "AI/Observation/EnemyObservation.h"
#include "EnemyVisionComponent.generated.h"

UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class GASTEST_API UEnemyVisionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnemyVisionComponent();

    UFUNCTION(BlueprintPure, Category = "AI|Perception")
    FEnemyObservation GetLatestObservation() const { return LatestObservation; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "0.01", Units = "s"))
    float PerceptionInterval = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "0.0", Units = "cm"))
    float VisionDistance = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "0.0", ClampMax = "360.0", Units = "deg"))
    float HorizontalFOV = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
    bool bRequireLineOfSight = true;

    // Empty defaults to Pawn collision object type.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
    TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
    FName RequiredTargetTag = TEXT("Player");

    // Fallback only when no valid VisionPoint exists: owner-local centimeters, unscaled.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
    FVector EyeOffset = FVector(0.0, 0.0, 70.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Debug")
    bool bDrawVisionDebug = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Debug")
    bool bEnableVisionLogs = true;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void PerformPerception();
    void ScheduleNextPerception();
    float GetSafeInterval() const;
    bool HasLineOfSight(AActor* Candidate, const FVector& Eye, const FVector& TargetLocation) const;
    float CalculateConfidence(float Distance, float Yaw) const;
    void GetVisionFrame(FVector& OutEye, FVector& OutForward, FVector& OutRight) const;
    void DrawVisionDebug(const FVector& Eye, const FVector& Forward, const FVector& Right) const;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Vision", meta = (AllowPrivateAccess = "true"))
    FEnemyObservation LatestObservation;

    // Pairs consecutive visible samples only; cleared immediately on loss.
    TWeakObjectPtr<AActor> PreviousVisibleTarget;
    double PreviousSampleTime = 0.0;
    FTimerHandle PerceptionTimer;
};
