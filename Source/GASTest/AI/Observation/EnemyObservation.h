#pragma once

#include "CoreMinimal.h"
#include "EnemyObservation.generated.h"

// Geometry-only snapshot: no actor identity; position is valid only while visible.
USTRUCT(BlueprintType)
struct GASTEST_API FEnemyObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Observation")
    bool bTargetVisible = false;

    // Visible point accepted by Vision's range, FOV and LOS gates. Reset on loss.
    UPROPERTY(BlueprintReadOnly, Category = "Observation")
    FVector ObservedWorldPosition = FVector::ZeroVector;

    // Horizontal unit direction: X forward, Y right in the enemy's visual frame.
    UPROPERTY(BlueprintReadOnly, Category = "Observation")
    FVector2D RelativeDirection = FVector2D::ZeroVector;

    // Degrees; positive right, negative left.
    UPROPERTY(BlueprintReadOnly, Category = "Observation")
    float RelativeYaw = 0.0f;

    // Centimeters from eye to target actor center.
    UPROPERTY(BlueprintReadOnly, Category = "Observation")
    float EstimatedDistance = 0.0f;

    // cm/s; positive approaching. Includes observer motion.
    UPROPERTY(BlueprintReadOnly, Category = "Observation")
    float ClosingSpeed = 0.0f;

    // Degrees/s; positive toward observer's right. Includes observer rotation.
    UPROPERTY(BlueprintReadOnly, Category = "Observation")
    float AngularVelocity = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Observation")
    float Confidence = 0.0f;

    // World game time in seconds, also updated on invisible samples.
    UPROPERTY(BlueprintReadOnly, Category = "Observation")
    float ObservationTime = 0.0f;
};
