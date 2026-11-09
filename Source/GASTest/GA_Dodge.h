#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Dodge.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

UENUM(BlueprintType)
enum class EDodgeDirection : uint8
{
	Front,
	Back,
	Left,
	Right
};

UCLASS()
class GASTEST_API UGA_Dodge : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dodge();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	/**
	 * Character 在 Perfect Window 中检测到伤害时调用。
	 *
	 * ImpactPoint:
	 * 本次攻击碰撞发生的位置。
	 */
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void TriggerPerfectDodge(const FVector& ImpactPoint);

	/**
	 * 当前是否处于 Perfect Dodge 判定窗口。
	 */
	UFUNCTION(BlueprintPure, Category = "Dodge")
	bool IsPerfectWindowActive() const
	{
		return bPerfectWindowActive;
	}

protected:

	// =========================
	// Montage
	// =========================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeMontage;

	// =========================
	// Perfect Dodge
	// =========================

	/**
	 * 完美闪避窗口。
	 * 默认：闪避开始后的 0.1 秒。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Perfect")
	float PerfectWindowDuration = 0.1f;

	// =========================
	// Internal
	// =========================

	UPROPERTY(BlueprintReadOnly, Category = "Dodge")
	EDodgeDirection DodgeDirection = EDodgeDirection::Back;

	bool bPerfectWindowActive = false;

	bool bPerfectDodgeTriggered = false;

	FTimerHandle PerfectWindowTimer;

	/**
	 * 根据玩家当前移动输入判断普通闪避方向。
	 */
	EDodgeDirection CalculateDodgeDirection() const;

	/**
	 * 将方向转换为 Montage Section。
	 */
	FName GetSectionName(EDodgeDirection Direction) const;

	/**
	 * 关闭 Perfect Window。
	 */
	void ClosePerfectWindow();

	/**
	 * 切换到 PerfectLeft / PerfectRight。
	 */
	void PlayPerfectSection(FName SectionName);

	// =========================
	// Montage callbacks
	// =========================

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();
};