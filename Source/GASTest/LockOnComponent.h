#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LockOnComponent.generated.h"

class ACharacter;
class APlayerController;
class UCameraComponent;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnLockOnTargetChanged,
	AActor*,
	NewTarget,
	AActor*,
	OldTarget
);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GASTEST_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULockOnComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	/* =========================
	 * Lock On
	 * ========================= */

	 /**
	  * 如果当前没有目标，则寻找目标。
	  * 如果已经有目标，则解除锁定。
	  */
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void ToggleLockOn();

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	bool TryLockOn();

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void ClearTarget();

	/**
	 * 当前锁定目标
	 */
	UFUNCTION(BlueprintPure, Category = "LockOn")
	AActor* GetCurrentTarget() const
	{
		return CurrentTarget;
	}

	UFUNCTION(BlueprintPure, Category = "LockOn")
	bool IsLockedOn() const
	{
		return IsValid(CurrentTarget);
	}



	/**
	 * 获得目标锁定点。
	 * 如果目标身上存在 TargetLockPoint，则优先使用。
	 * 否则使用 ActorLocation + Offset。
	 */
	UFUNCTION(BlueprintPure, Category = "LockOn")
	FVector GetTargetLocation() const;

	/**
	 * 锁定目标变化事件
	 */
	UPROPERTY(BlueprintAssignable, Category = "LockOn")
	FOnLockOnTargetChanged OnLockOnTargetChanged;

protected:

	/* =========================
	 * Target Search
	 * ========================= */

	AActor* FindBestTarget();

	bool IsValidTarget(AActor* Target) const;

	bool IsTargetInsideView(AActor* Target) const;

	bool HasLineOfSightToTarget(AActor* Target) const;

	float CalculateTargetScore(AActor* Target) const;

	/* =========================
	 * Camera
	 * ========================= */

	void UpdateLockOn(float DeltaTime);

	void UpdateCameraRotation(float DeltaTime);

	bool ShouldBreakLock() const;

	/* =========================
	 * Gameplay Tag
	 * ========================= */

	void AddLockOnTag();

	void RemoveLockOnTag();

	/* =========================
	 * Target Event
	 * ========================= */

	UFUNCTION()
	void OnTargetDestroyed(AActor* DestroyedActor);

private:

	/* =========================
	 * Runtime References
	 * ========================= */

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY()
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

public:

	/* =========================
	 * Search Settings
	 * ========================= */

	 /**
	  * 最大搜索距离
	  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	float MaxLockDistance = 2000.0f;

	/**
	 * 自动解除锁定距离。
	 *
	 * 建议略大于 MaxLockDistance，
	 * 防止敌人在搜索边缘反复锁定/解除。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	float BreakLockDistance = 2500.0f;

	/**
	 * 摄像机与目标方向 Dot Product 最低值。
	 *
	 * 0 = 前方180度
	 * 0.5 = 前方约120度
	 * 0.707 = 前方约90度
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search",
		meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinimumViewDot = 0.35f;

	/**
	 * 用于搜索敌人的 Object Types。
	 *
	 * 建议在蓝图中配置成 Pawn。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

	/**
	 * 目标必须拥有这个 Actor Tag。
	 *
	 * 注意：
	 * 这是 Actor Tag，不是 GameplayTag。
	 *
	 * Enemy 蓝图：
	 * Actor -> Tags -> Enemy
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	FName RequiredActorTag = TEXT("Enemy");

	/* =========================
	 * Score Settings
	 * ========================= */

	 /**
	  * 距离权重
	  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Score")
	float DistanceWeight = 0.4f;

	/**
	 * 摄像机中心权重
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Score")
	float AngleWeight = 0.6f;

	/* =========================
	 * Camera Settings
	 * ========================= */

	 /**
	  * 摄像机跟随旋转速度
	  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
	float CameraRotationSpeed = 8.0f;

	/**
	 * 默认目标位置偏移。
	 *
	 * 如果没有 TargetLockPoint，
	 * 摄像机看 ActorLocation + Offset。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
	FVector TargetLocationOffset = FVector(0.0f, 0.0f, 80.0f);

	/**
	 * 是否检查墙体遮挡。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
	bool bCheckLineOfSight = true;

	/**
	 * 被墙挡住多久以后解除锁定。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera",
		meta = (EditCondition = "bCheckLineOfSight"))
	float LostLineOfSightBreakTime = 1.0f;

	/**
 * 垂直锁定死区。
 *
 * 当目标锁定点与玩家之间的 Z 高度差
 * 小于这个值时，不调整摄像机 Pitch。
 *
 * 单位：cm
 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "LockOn|Camera",
		meta = (ClampMin = "0.0")
	)
	float VerticalPitchDeadZone = 150.0f;

	/* =========================
	 * GAS
	 * ========================= */

	 /**
	  * 锁定状态 GameplayTag
	  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|GAS")
	FGameplayTag LockedGameplayTag;

private:

	float LostLineOfSightTime = 0.0f;
};