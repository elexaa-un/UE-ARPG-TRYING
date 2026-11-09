#include "LockOnComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/SceneComponent.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (!OwnerCharacter)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[LockOn] Owner is not Character")
		);

		return;
	}

	PlayerController =
		Cast<APlayerController>(
			OwnerCharacter->GetController()
		);

	/*
	 * 找 CameraComponent
	 */
	CameraComponent =
		OwnerCharacter->FindComponentByClass<UCameraComponent>();

	if (!CameraComponent)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[LockOn] CameraComponent not found")
		);
	}

	/*
	 * 找 ASC
	 */
	if (IAbilitySystemInterface* ASI =
		Cast<IAbilitySystemInterface>(OwnerCharacter))
	{
		AbilitySystemComponent =
			ASI->GetAbilitySystemComponent();
	}
	else
	{
		AbilitySystemComponent =
			OwnerCharacter
			->FindComponentByClass<UAbilitySystemComponent>();
	}

	/*
	 * 如果蓝图没有配置 ObjectType，
	 * 默认使用 Pawn。
	 */
	if (TargetObjectTypes.IsEmpty())
	{
		TargetObjectTypes.Add(
			UEngineTypes::ConvertToObjectType(
				ECollisionChannel::ECC_Pawn
			)
		);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[LockOn] Component initialized | Owner=%s"),
		*GetNameSafe(OwnerCharacter)
	);
}

void ULockOnComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(
		DeltaTime,
		TickType,
		ThisTickFunction
	);

	if (!CurrentTarget)
	{
		return;
	}

	UpdateLockOn(DeltaTime);
}

/* =========================================================
 * Toggle
 * ========================================================= */

void ULockOnComponent::ToggleLockOn()
{
	if (CurrentTarget)
	{
		ClearTarget();
	}
	else
	{
		TryLockOn();
	}
}

/* =========================================================
 * Try Lock
 * ========================================================= */

bool ULockOnComponent::TryLockOn()
{
	if (!OwnerCharacter)
	{
		return false;
	}

	AActor* BestTarget = FindBestTarget();

	if (!BestTarget)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[LockOn] No valid target found")
		);

		return false;
	}

	CurrentTarget = BestTarget;

	LostLineOfSightTime = 0.0f;

	/*
	 * 监听目标销毁
	 */
	CurrentTarget->OnDestroyed.AddDynamic(
		this,
		&ULockOnComponent::OnTargetDestroyed
	);

	AddLockOnTag();

	OnLockOnTargetChanged.Broadcast(CurrentTarget,nullptr);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[LockOn] Locked -> %s"),
		*GetNameSafe(CurrentTarget)
	);

	return true;
}

/* =========================================================
 * Clear Target
 * ========================================================= */

void ULockOnComponent::ClearTarget()
{
	if (CurrentTarget)
	{
		CurrentTarget->OnDestroyed.RemoveDynamic(
			this,
			&ULockOnComponent::OnTargetDestroyed
		);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[LockOn] Unlock -> %s"),
			*GetNameSafe(CurrentTarget)
		);
	}



	LostLineOfSightTime = 0.0f;

	OnLockOnTargetChanged.Broadcast(nullptr,CurrentTarget);

	CurrentTarget = nullptr;

	RemoveLockOnTag();
}

/* =========================================================
 * Find Best Target
 * ========================================================= */

AActor* ULockOnComponent::FindBestTarget()
{
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(OwnerCharacter);

	TArray<AActor*> FoundActors;

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		OwnerCharacter->GetActorLocation(),
		MaxLockDistance,
		TargetObjectTypes,
		AActor::StaticClass(),
		IgnoreActors,
		FoundActors
	);

	AActor* BestTarget = nullptr;

	float BestScore =
		TNumericLimits<float>::Max();

	for (AActor* Target : FoundActors)
	{
		if (!IsValidTarget(Target))
		{
			continue;
		}

		if (!IsTargetInsideView(Target))
		{
			continue;
		}

		if (bCheckLineOfSight &&
			!HasLineOfSightToTarget(Target))
		{
			continue;
		}

		const float Score =
			CalculateTargetScore(Target);

		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"[LockOn] Candidate=%s | Score=%.3f"
			),
			*GetNameSafe(Target),
			Score
		);

		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Target;
		}
	}

	return BestTarget;
}

/* =========================================================
 * Valid Target
 * ========================================================= */

bool ULockOnComponent::IsValidTarget(
	AActor* Target
) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	if (Target == OwnerCharacter)
	{
		return false;
	}

	if (!RequiredActorTag.IsNone())
	{
		if (!Target->ActorHasTag(RequiredActorTag))
		{
			return false;
		}
	}

	const float Distance =
		FVector::Dist(
			OwnerCharacter->GetActorLocation(),
			Target->GetActorLocation()
		);

	if (Distance > MaxLockDistance)
	{
		return false;
	}

	return true;
}

/* =========================================================
 * View Check
 * ========================================================= */

bool ULockOnComponent::IsTargetInsideView(
	AActor* Target
) const
{
	if (!Target)
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;

	if (PlayerController)
	{
		PlayerController->GetPlayerViewPoint(
			ViewLocation,
			ViewRotation
		);
	}
	else if (CameraComponent)
	{
		ViewLocation =
			CameraComponent->GetComponentLocation();

		ViewRotation =
			CameraComponent->GetComponentRotation();
	}
	else
	{
		return false;
	}

	const FVector CameraForward =
		ViewRotation.Vector();

	const FVector ToTarget =
		(
			Target->GetActorLocation()
			- ViewLocation
			).GetSafeNormal();

	const float Dot =
		FVector::DotProduct(
			CameraForward,
			ToTarget
		);

	return Dot >= MinimumViewDot;
}

/* =========================================================
 * Score
 * ========================================================= */

float ULockOnComponent::CalculateTargetScore(
	AActor* Target
) const
{
	if (!Target || !OwnerCharacter)
	{
		return TNumericLimits<float>::Max();
	}

	FVector ViewLocation;
	FRotator ViewRotation;

	if (PlayerController)
	{
		PlayerController->GetPlayerViewPoint(
			ViewLocation,
			ViewRotation
		);
	}
	else if (CameraComponent)
	{
		ViewLocation =
			CameraComponent->GetComponentLocation();

		ViewRotation =
			CameraComponent->GetComponentRotation();
	}
	else
	{
		return TNumericLimits<float>::Max();
	}

	/*
	 * 距离
	 */
	const float Distance =
		FVector::Dist(
			OwnerCharacter->GetActorLocation(),
			Target->GetActorLocation()
		);

	const float DistanceScore =
		FMath::Clamp(
			Distance / MaxLockDistance,
			0.0f,
			1.0f
		);

	/*
	 * 屏幕中心偏差
	 */
	const FVector ToTarget =
		(
			Target->GetActorLocation()
			- ViewLocation
			).GetSafeNormal();

	const float Dot =
		FVector::DotProduct(
			ViewRotation.Vector(),
			ToTarget
		);

	const float AngleScore =
		1.0f -
		FMath::Clamp(
			Dot,
			-1.0f,
			1.0f
		);

	return
		DistanceScore * DistanceWeight +
		AngleScore * AngleWeight;
}

/* =========================================================
 * Update Lock
 * ========================================================= */

void ULockOnComponent::UpdateLockOn(
	float DeltaTime
)
{
	if (!CurrentTarget)
	{
		return;
	}

	if (ShouldBreakLock())
	{
		ClearTarget();
		return;
	}

	/*
	 * Line Of Sight
	 */
	if (bCheckLineOfSight)
	{
		if (!HasLineOfSightToTarget(CurrentTarget))
		{
			LostLineOfSightTime += DeltaTime;

			if (LostLineOfSightTime >=
				LostLineOfSightBreakTime)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT(
						"[LockOn] Lost line of sight"
					)
				);

				ClearTarget();
				return;
			}
		}
		else
		{
			LostLineOfSightTime = 0.0f;
		}
	}

	UpdateCameraRotation(DeltaTime);
}

/* =========================================================
 * Camera Rotation
 * ========================================================= */

void ULockOnComponent::UpdateCameraRotation(
	float DeltaTime
)
{
	if (!CurrentTarget ||
		!PlayerController)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;

	PlayerController->GetPlayerViewPoint(
		ViewLocation,
		ViewRotation
	);

	const FVector TargetLocation =
		GetTargetLocation();

	const FRotator CurrentRotation =
		PlayerController->GetControlRotation();

	// 正常计算目标方向
	FRotator DesiredRotation =
		(TargetLocation - ViewLocation).Rotation();

	DesiredRotation.Roll = 0.0f;

	// ================================
	// Vertical Pitch Dead Zone
	// ================================

	const float HeightDifference =
		TargetLocation.Z -
		OwnerCharacter->GetActorLocation().Z;

	if (FMath::Abs(HeightDifference) <=
		VerticalPitchDeadZone)
	{
		// 死区内完全保持当前Pitch
		DesiredRotation.Pitch =
			CurrentRotation.Pitch;
	}
	else
	{
		// 只追踪超出死区的高度
		const float ExcessHeight =
			HeightDifference -
			FMath::Sign(HeightDifference) *
			VerticalPitchDeadZone;

		FVector PitchTarget = TargetLocation;

		PitchTarget.Z =
			ViewLocation.Z + ExcessHeight;

		DesiredRotation.Pitch =
			(PitchTarget - ViewLocation)
			.Rotation()
			.Pitch;
	}

	// ================================
	// Smooth Rotation
	// ================================

	const FRotator NewRotation =
		FMath::RInterpTo(
			CurrentRotation,
			DesiredRotation,
			DeltaTime,
			CameraRotationSpeed
		);

	PlayerController->SetControlRotation(
		NewRotation
	);
}

/* =========================================================
 * Target Location
 * ========================================================= */

FVector ULockOnComponent::GetTargetLocation() const
{
	if (!CurrentTarget)
	{
		return FVector::ZeroVector;
	}

	/*
	 * 查找名字为 TargetLockPoint 的 SceneComponent
	 */
	TArray<USceneComponent*> SceneComponents;

	CurrentTarget->GetComponents<USceneComponent>(
		SceneComponents
	);

	for (USceneComponent* Component :
		SceneComponents)
	{
		if (Component &&
			Component->GetFName() ==
			TEXT("TargetLockPoint"))
		{
			return
				Component->GetComponentLocation();
		}
	}

	/*
	 * 没有 LockPoint 就使用默认偏移
	 */
	return
		CurrentTarget->GetActorLocation()
		+ TargetLocationOffset;
}

/* =========================================================
 * Break Lock
 * ========================================================= */

bool ULockOnComponent::ShouldBreakLock() const
{
	if (!IsValid(CurrentTarget))
	{
		return true;
	}

	if (!OwnerCharacter)
	{
		return true;
	}

	const float Distance =
		FVector::Dist(
			OwnerCharacter->GetActorLocation(),
			CurrentTarget->GetActorLocation()
		);

	if (Distance > BreakLockDistance)
	{
		return true;
	}

	return false;
}

/* =========================================================
 * Line Of Sight
 * ========================================================= */

bool ULockOnComponent::HasLineOfSightToTarget(
	AActor* Target
) const
{
	if (!Target || !GetWorld())
	{
		return false;
	}

	FVector Start;

	if (CameraComponent)
	{
		Start =
			CameraComponent->GetComponentLocation();
	}
	else if (OwnerCharacter)
	{
		Start =
			OwnerCharacter->GetActorLocation();
	}
	else
	{
		return false;
	}

	FVector End =
		Target->GetActorLocation()
		+ TargetLocationOffset;

	FCollisionQueryParams Params;

	Params.AddIgnoredActor(OwnerCharacter);

	FHitResult HitResult;

	const bool bHit =
		GetWorld()->LineTraceSingleByChannel(
			HitResult,
			Start,
			End,
			ECC_Visibility,
			Params
		);

	/*
	 * 什么都没撞到
	 */
	if (!bHit)
	{
		return true;
	}

	/*
	 * 第一个撞到的是目标本身
	 */
	if (HitResult.GetActor() == Target)
	{
		return true;
	}

	return false;
}

/* =========================================================
 * Target Destroyed
 * ========================================================= */

void ULockOnComponent::OnTargetDestroyed(
	AActor* DestroyedActor
)
{
	if (DestroyedActor == CurrentTarget)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[LockOn] Target destroyed")
		);

		ClearTarget();
	}
}

/* =========================================================
 * GAS Tag
 * ========================================================= */

void ULockOnComponent::AddLockOnTag()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (!LockedGameplayTag.IsValid())
	{
		return;
	}

	AbilitySystemComponent
		->AddLooseGameplayTag(
			LockedGameplayTag
		);
}

void ULockOnComponent::RemoveLockOnTag()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (!LockedGameplayTag.IsValid())
	{
		return;
	}

	AbilitySystemComponent
		->RemoveLooseGameplayTag(
			LockedGameplayTag
		);
}