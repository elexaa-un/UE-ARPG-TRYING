#include "AI/EnemyBrainComponent.h"

#include "AI/Perception/EnemyVisionComponent.h"
#include "AI/EnemyNavigationComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "EnemyCharacter.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyBrain, Log, All);

UEnemyBrainComponent::UEnemyBrainComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UEnemyBrainComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerEnemy = Cast<AEnemyCharacter>(GetOwner());
    VisionComponent = OwnerEnemy ? OwnerEnemy->GetEnemyVisionComponent() : nullptr;
    NavigationComponent = OwnerEnemy ? OwnerEnemy->GetEnemyNavigationComponent() : nullptr;
    if (!IsValid(OwnerEnemy) || !IsValid(VisionComponent)||!IsValid(NavigationComponent))
    {
        UE_LOG(LogEnemyBrain, Warning, TEXT("[EnemyBrain] Requires EnemyCharacter and EnemyVisionComponent"));
        SetComponentTickEnabled(false);
        return;
    }
    // Submit this frame's input before CharacterMovement consumes it.
    OwnerEnemy->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
}

void UEnemyBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(OwnerEnemy))
    {
        OwnerEnemy->StopMovementInput();
        OwnerEnemy->GetCharacterMovement()->RemoveTickPrerequisiteComponent(this);
    }
    MoveDirection = FVector::ZeroVector;
    Memory = FEnemyMemory();
    bWasTargetVisible = false;
    Super::EndPlay(EndPlayReason);
}

void UEnemyBrainComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!IsValid(OwnerEnemy) || !IsValid(VisionComponent))
    {
        if (IsValid(OwnerEnemy))
        {
            OwnerEnemy->StopMovementInput();
        }
        SetState(EEnemyAIState::Idle);
        Memory = FEnemyMemory();
        SetComponentTickEnabled(false);
        return;
    }

    // Capture every perception sample, even when Vision runs faster than Think.
    // This is bookkeeping only; full decisions and attack requests stay at 5 Hz.
    UpdateMemory(VisionComponent->GetLatestObservation());
    ThinkElapsed += DeltaTime;
    const float Interval = FMath::IsFinite(ThinkInterval) ? FMath::Max(0.01f, ThinkInterval) : 0.2f;
    if (ThinkElapsed >= Interval)
    {
        ThinkElapsed = 0.0f; // No catch-up bursts after a long frame.
        Think();
    }

    if (CurrentState == EEnemyAIState::Approach)
    {
        OwnerEnemy->MoveInWorldDirection(MoveDirection);
    }
    else if (CurrentState == EEnemyAIState::Search)
    {
        ExecuteSearch(DeltaTime);
    }

    if (bDrawMemoryDebug && Memory.bHasLastSeenTarget)
    {
        DrawDebugSphere(GetWorld(), Memory.LastSeenPosition, 25.0f, 12, FColor::Orange, false, 0.0f);
        DrawDebugLine(GetWorld(), OwnerEnemy->GetActorLocation(), Memory.LastSeenPosition,
            FColor::Orange, false, 0.0f, 0, 2.0f);
    }
}

void UEnemyBrainComponent::Think()
{
    const FEnemyObservation Observation = VisionComponent->GetLatestObservation();
    // 正在执行不可中断的攻击
    if (OwnerEnemy->IsCombatActionActive())
    {
        // Perception / Memory仍然继续工作
        // 但是Brain暂时不切换行为
        return;
    }
    MoveDirection = FVector::ZeroVector;
    if (!Observation.bTargetVisible)
    {
        SetState(Memory.bHasLastSeenTarget ? EEnemyAIState::Search : EEnemyAIState::Idle);
        if (CurrentState == EEnemyAIState::Idle)
        {
            OwnerEnemy->StopMovementInput();
        }
        return;
    }

    if (Observation.EstimatedDistance > AttackRange)
    {
        SetState(EEnemyAIState::Approach);
        // Observation uses the visual frame, including editable VisionPoint rotation.
        if (Observation.EstimatedDistance > AttackRange)
        {
            SetState(EEnemyAIState::Approach);

            if (!IsValid(NavigationComponent))
            {
                MoveDirection = FVector::ZeroVector;
                return;
            }

            MoveDirection =
                NavigationComponent->FindBestDirection(
                    OwnerEnemy->GetActorLocation(),
                    Observation.ObservedWorldPosition
                );

            return;
        }
    }

    SetState(EEnemyAIState::Attack);
    OwnerEnemy->StopMovementInput();
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now >= NextAttackTime)
    {
        // Throttle failed attempts too (e.g. active ability or missing configuration).
        // This is request pacing; GAS still owns activation/commit and physical hits.
        const float Cooldown = FMath::IsFinite(AttackCooldown) ? FMath::Max(0.01f, AttackCooldown) : 1.5f;
        NextAttackTime = Now + Cooldown;
        if (OwnerEnemy->RequestLightAttack())
        {
            UE_LOG(LogEnemyBrain, Log, TEXT("[EnemyBrain] Attack Requested | Owner=%s"), *GetNameSafe(OwnerEnemy));
        }
    }
}

void UEnemyBrainComponent::UpdateMemory(const FEnemyObservation& Observation)
{
    if (Observation.bTargetVisible)
    {
        if (!bWasTargetVisible && Memory.bHasLastSeenTarget)
        {
            UE_LOG(LogEnemyBrain, Log, TEXT("[EnemyMemory] Target Reacquired | Owner=%s"), *GetNameSafe(OwnerEnemy));
        }
        Memory.bHasLastSeenTarget = true;
        Memory.LastSeenPosition = Observation.ObservedWorldPosition;
        Memory.TimeSinceLastSeen = 0.0f;
        // Re-reading a sample never advances the time of the actual sighting.
        LastSeenObservationTime = Observation.ObservationTime;
    }
    else if (Memory.bHasLastSeenTarget)
    {
        if (bWasTargetVisible)
        {
            UE_LOG(LogEnemyBrain, Log, TEXT("[EnemyMemory] Target Lost | LastSeen=%s | Owner=%s"),
                *Memory.LastSeenPosition.ToString(), *GetNameSafe(OwnerEnemy));
        }
        Memory.TimeSinceLastSeen = static_cast<float>(FMath::Max(0.0,
            GetWorld()->GetTimeSeconds() - LastSeenObservationTime));
        const float ForgetTime = FMath::IsFinite(MemoryForgetTime) ? FMath::Max(0.0f, MemoryForgetTime) : 3.0f;
        if (Memory.TimeSinceLastSeen >= ForgetTime &&
            CurrentState != EEnemyAIState::Search)
        {
            Memory = FEnemyMemory();

            UE_LOG(
                LogEnemyBrain,
                Log,
                TEXT("[EnemyMemory] Target Forgotten | Owner=%s"),
                *GetNameSafe(OwnerEnemy)
            );
        }
    }
    bWasTargetVisible = Observation.bTargetVisible;
}

void UEnemyBrainComponent::ExecuteSearch(float DeltaTime)
{
    if (!Memory.bHasLastSeenTarget)
    {
        SetState(EEnemyAIState::Idle);
        OwnerEnemy->StopMovementInput();
        return;
    }

    // =========================
    // Phase 1: 前往最后目击位置
    // =========================
    if (SearchPhase == EEnemySearchPhase::MoveToLastSeen)
    {
        FVector ToLastSeen =
            Memory.LastSeenPosition -
            OwnerEnemy->GetActorLocation();

        ToLastSeen.Z = 0.0f;

        const float ArrivalDistance =
            FMath::Max(0.0f, SearchArrivalDistance);

        if (ToLastSeen.SizeSquared2D()
            <= FMath::Square(ArrivalDistance))
        {
            OwnerEnemy->StopMovementInput();

            // 一旦进入观察阶段，就不再重新判断到达距离
            SearchPhase = EEnemySearchPhase::LookAround;
            SearchLookTimer = 0.0f;

            return;
        }

        if (!IsValid(NavigationComponent))
        {
            OwnerEnemy->StopMovementInput();
            return;
        }

        const FVector Direction =
            NavigationComponent->FindBestDirection(
                OwnerEnemy->GetActorLocation(),
                Memory.LastSeenPosition
            );

        OwnerEnemy->MoveInWorldDirection(Direction);

        return;
    }

    // =========================
    // Phase 2: 原地观察
    // =========================
    if (SearchPhase == EEnemySearchPhase::LookAround)
    {
        OwnerEnemy->StopMovementInput();

        ExecuteLookAround(DeltaTime);

        if (SearchLookTimer >= LookAroundDuration)
        {
            OwnerEnemy->ResetVisionRotation();

            Memory = FEnemyMemory();

            SetState(EEnemyAIState::Idle);

            UE_LOG(
                LogEnemyBrain,
                Log,
                TEXT("[EnemySearch] Search finished -> Idle")
            );

            return;
        }

        return;
    }
}
void UEnemyBrainComponent::ExecuteLookAround(float DeltaTime)
{
    SearchLookTimer += DeltaTime;

    const float LookYaw =
        FMath::Sin(SearchLookTimer * LookSpeed)
        * MaxLookYaw;

    UE_LOG(
        LogEnemyBrain,
        Warning,
        TEXT("[LookAround] Time=%.2f Yaw=%.2f"),
        SearchLookTimer,
        LookYaw
    );

    OwnerEnemy->SetVisionYaw(LookYaw);
}

void UEnemyBrainComponent::SetState(EEnemyAIState NewState)
{
    if (CurrentState != NewState)
    {
        UE_LOG(LogEnemyBrain, Log, TEXT("[EnemyBrain] %s -> %s | Owner=%s"),
            *UEnum::GetValueAsString(CurrentState), *UEnum::GetValueAsString(NewState), *GetNameSafe(OwnerEnemy));
        CurrentState = NewState;
        if (NewState == EEnemyAIState::Search)
        {
            SearchPhase = EEnemySearchPhase::MoveToLastSeen;
            SearchLookTimer = 0.0f;
        }
    }
}