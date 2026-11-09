#pragma once

#include "CoreMinimal.h"
#include "CombatCharacterBase.h"
#include "EnemyCharacter.generated.h"

class UWidgetComponent;
class UEnemyVisionComponent;
class UEnemyBrainComponent;
class UEnemyNavigationComponent;
class USceneComponent;


UCLASS()
class GASTEST_API AEnemyCharacter : public ACombatCharacterBase
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

    UFUNCTION(BlueprintPure, Category = "AI|Perception")
    UEnemyVisionComponent* GetEnemyVisionComponent() const { return EnemyVisionComponent; }

    UFUNCTION(BlueprintPure, Category = "AI|Perception")
    USceneComponent* GetVisionPoint() const { return VisionPoint; }

    UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
    bool RequestLightAttack();
    virtual void StopMovementInput() override;

    UFUNCTION(BlueprintPure, Category = "AI|Perception")
    UEnemyNavigationComponent* GetEnemyNavigationComponent() const { return EnemyNavigationComponent; }

    void SetVisionYaw(float Yaw);
    void ResetVisionRotation() { bResettingVision = true; }
    bool IsCombatActionActive() const;
private:
    // Independent of the mesh; edit the inherited component's transform in Blueprint.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> VisionPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UEnemyVisionComponent> EnemyVisionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UEnemyNavigationComponent> EnemyNavigationComponent;
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "UI",
        meta = (AllowPrivateAccess = "true")
    )
    TObjectPtr<UWidgetComponent> HealthBarWidget;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UEnemyBrainComponent> EnemyBrainComponent;

    bool bResettingVision = false;

    UPROPERTY(EditAnywhere, Category = "AI|Perception")
    float VisionResetSpeed = 5.0f;
protected:
    virtual void Tick(float DeltaTime) override;
};
