#pragma once

#include "CoreMinimal.h"
#include "CombatCharacterBase.h"
#include "GASTestCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UWidgetComponent;
class ULockOnComponent;

struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(
    LogTemplateCharacter,
    Log,
    All
);

UCLASS(Abstract)
class GASTEST_API AGASTestCharacter
    : public ACombatCharacterBase
{
    GENERATED_BODY()

public:
    AGASTestCharacter();

    // ================================
    // Player Combat Input
    // ================================

    UFUNCTION(BlueprintCallable, Category = "GAS|Input")
    void AttackInputPressed();

    UFUNCTION(BlueprintCallable, Category = "GAS|Input")
    void ChargeAttackInputPressed();

    UFUNCTION(BlueprintCallable, Category = "GAS|Input")
    void ChargeAttackInputReleased();

    UFUNCTION(BlueprintCallable, Category = "GAS|Input")
    void DodgeInputPressed();

    // ================================
    // Lock-on
    // ================================

    UFUNCTION(BlueprintPure, Category = "LockOn")
    ULockOnComponent* GetLockOnComponent() const
    {
        return LockOnComponent;
    }

    UFUNCTION(BlueprintCallable, Category = "Combat|LockOn")
    bool FaceLockedTarget();
    // ================================
    // Camera
    // ================================

    FORCEINLINE USpringArmComponent* GetCameraBoom() const
    {
        return CameraBoom;
    }

    FORCEINLINE UCameraComponent* GetFollowCamera() const
    {
        return FollowCamera;
    }

    // ================================
    // Input wrappers
    // ================================

    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoMove(float Right, float Forward);

    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoLook(float Yaw, float Pitch);

    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpStart();

    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpEnd();

protected:
    virtual void SetupPlayerInputComponent(
        UInputComponent* PlayerInputComponent
    ) override;

    virtual void OnMovementModeChanged(
        EMovementMode PrevMovementMode,
        uint8 PreviousCustomMode = 0
    ) override;

    // ================================
    // Enhanced Input
    // ================================

    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<UInputAction> MouseLookAction;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

    void StopMove()
    {
        StopMovementInput();
    }

    // ================================
    // Player Components
    // ================================

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Components",
        meta = (AllowPrivateAccess = "true")
    )
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Components",
        meta = (AllowPrivateAccess = "true")
    )
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "LockOn",
        meta = (AllowPrivateAccess = "true")
    )
    TObjectPtr<ULockOnComponent> LockOnComponent;

private:
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadWrite,
        Category = "UI",
        meta = (AllowPrivateAccess = "true")
    )
    TObjectPtr<UWidgetComponent> HealthBarWidget;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadWrite,
        Category = "UI",
        meta = (AllowPrivateAccess = "true")
    )
    TObjectPtr<UWidgetComponent> LockShowWidget;

    FVector CurrentMoveInputDirection =
        FVector::ZeroVector;

    bool bHasMoveInput = false;
};