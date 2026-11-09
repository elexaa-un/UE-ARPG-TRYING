#include "AI/Perception/EnemyVisionComponent.h"

#include "CollisionQueryParams.h"
#include "Components/SceneComponent.h"
#include "EnemyCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyVision, Log, All);

UEnemyVisionComponent::UEnemyVisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyVisionComponent::BeginPlay()
{
    Super::BeginPlay();
    ScheduleNextPerception();
}

void UEnemyVisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PerceptionTimer);
    }
    PreviousVisibleTarget.Reset();
    LatestObservation = FEnemyObservation();
    Super::EndPlay(EndPlayReason);
}

float UEnemyVisionComponent::GetSafeInterval() const
{
    return FMath::IsFinite(PerceptionInterval) ? FMath::Max(0.01f, PerceptionInterval) : 0.1f;
}

void UEnemyVisionComponent::ScheduleNextPerception()
{
    if (UWorld* World = GetWorld())
    {
        // One-shot avoids catch-up bursts and honors runtime interval edits.
        World->GetTimerManager().SetTimer(PerceptionTimer, this,
            &UEnemyVisionComponent::PerformPerception, GetSafeInterval(), false);
    }
}

void UEnemyVisionComponent::GetVisionFrame(FVector& OutEye, FVector& OutForward, FVector& OutRight) const
{
    const AActor* Owner = GetOwner();
    const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Owner);
    const USceneComponent* Point = Enemy ? Enemy->GetVisionPoint() : nullptr;
    if (IsValid(Point))
    {
        OutEye = Point->GetComponentLocation();
        OutForward = Point->GetForwardVector();
        OutRight = Point->GetRightVector();
        return;
    }

    // Preserve the original owner-based, yaw-only fallback.
    OutEye = Owner->GetActorLocation() + Owner->GetActorQuat().RotateVector(EyeOffset);
    const FRotator Facing(0.0, Owner->GetActorRotation().Yaw, 0.0);
    OutForward = Facing.Vector();
    OutRight = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
}

void UEnemyVisionComponent::PerformPerception()
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !IsValid(Owner))
    {
        LatestObservation = FEnemyObservation();
        PreviousVisibleTarget.Reset();
        return;
    }

    const double Now = World->GetTimeSeconds();
    const FEnemyObservation Previous = LatestObservation;
    LatestObservation = FEnemyObservation();
    LatestObservation.ObservationTime = static_cast<float>(Now);
    FVector Eye, Forward, Right;
    GetVisionFrame(Eye, Forward, Right);
    const float HalfFOV = FMath::Clamp(HorizontalFOV, 0.0f, 360.0f) * 0.5f;
    if (bDrawVisionDebug)
    {
        DrawVisionDebug(Eye, Forward, Right);
    }

    AActor* BestTarget = nullptr;
    FVector BestLocation = FVector::ZeroVector;
    float BestScore = -1.0f;
    if (FMath::IsFinite(VisionDistance) && VisionDistance > UE_SMALL_NUMBER && FMath::IsFinite(HorizontalFOV))
    {
        TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = TargetObjectTypes;
        if (ObjectTypes.IsEmpty())
        {
            ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
        }
        TArray<AActor*> IgnoreActors;
        IgnoreActors.Add(Owner);
        TArray<AActor*> Candidates;
        UKismetSystemLibrary::SphereOverlapActors(this, Eye, VisionDistance,
            ObjectTypes, nullptr, IgnoreActors, Candidates);

        for (AActor* Candidate : Candidates)
        {
            if (!IsValid(Candidate) || Candidate == Owner || !Candidate->ActorHasTag(RequiredTargetTag))
            {
                continue;
            }
            // Character actor origin is its capsule/body center.
            const FVector TargetLocation = Candidate->GetActorLocation();
            const FVector ToTarget = TargetLocation - Eye;
            const float Distance = ToTarget.Size();
            if (!FMath::IsFinite(Distance) || Distance > VisionDistance)
            {
                continue;
            }
            // Horizontal means the visual frame's forward/right plane. This also
            // keeps pitch/roll edits consistent with the debug FOV, without a vertical gate.
            const FVector2D Direction = FVector2D(FVector::DotProduct(Forward, ToTarget),
                FVector::DotProduct(Right, ToTarget)).GetSafeNormal();
            const float Yaw = Direction.IsNearlyZero() ? 0.0f : FMath::RadiansToDegrees(
                FMath::Atan2(Direction.Y, Direction.X));
            if (FMath::Abs(Yaw) > HalfFOV || !HasLineOfSight(Candidate, Eye, TargetLocation))
            {
                continue;
            }
            const float Score = CalculateConfidence(Distance, Yaw);
            // Retain current visible identity on equal scores to avoid gratuitous switches.
            if (Score > BestScore || (Score == BestScore && Candidate == PreviousVisibleTarget.Get()))
            {
                BestScore = Score;
                BestTarget = Candidate;
                BestLocation = TargetLocation;
                LatestObservation.bTargetVisible = true;
                LatestObservation.ObservedWorldPosition = TargetLocation;
                LatestObservation.RelativeDirection = Direction;
                LatestObservation.RelativeYaw = Yaw;
                LatestObservation.EstimatedDistance = Distance;
                LatestObservation.Confidence = Score;
            }
        }
    }

    const double Elapsed = Now - PreviousSampleTime;
    if (BestTarget && Previous.bTargetVisible && PreviousVisibleTarget.Get() == BestTarget && Elapsed > UE_SMALL_NUMBER)
    {
        LatestObservation.ClosingSpeed = (Previous.EstimatedDistance - LatestObservation.EstimatedDistance) / Elapsed;
        LatestObservation.AngularVelocity = FMath::FindDeltaAngleDegrees(
            Previous.RelativeYaw, LatestObservation.RelativeYaw) / Elapsed;
    }
    PreviousVisibleTarget = BestTarget;
    PreviousSampleTime = Now;

    if (bDrawVisionDebug && BestTarget)
    {
        DrawDebugLine(World, Eye, BestLocation, FColor::Green, false, GetSafeInterval(), 0, 2.0f);
    }
    // Visible samples log at perception rate; invisible samples only on loss.
    if (bEnableVisionLogs && (BestTarget || Previous.bTargetVisible))
    {
        UE_LOG(LogEnemyVision, Log, TEXT("[EnemyVision] Owner=%s Visible=%d Distance=%.1f Yaw=%.1f ClosingSpeed=%.1f AngularVelocity=%.1f Confidence=%.2f"),
            *GetNameSafe(Owner), LatestObservation.bTargetVisible ? 1 : 0,
            LatestObservation.EstimatedDistance, LatestObservation.RelativeYaw,
            LatestObservation.ClosingSpeed, LatestObservation.AngularVelocity, LatestObservation.Confidence);
    }
    ScheduleNextPerception();
}

bool UEnemyVisionComponent::HasLineOfSight(AActor* Candidate, const FVector& Eye, const FVector& TargetLocation) const
{
    if (!bRequireLineOfSight)
    {
        return true;
    }
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyVision), false, GetOwner());
    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Eye, TargetLocation, ECC_Visibility, Params);
    const bool bVisible = !bHit || Hit.GetActor() == Candidate;
    if (bDrawVisionDebug && !bVisible)
    {
        DrawDebugLine(GetWorld(), Eye, Hit.ImpactPoint, FColor::Red, false, GetSafeInterval(), 0, 2.0f);
    }
    return bVisible;
}

float UEnemyVisionComponent::CalculateConfidence(float Distance, float Yaw) const
{
    const float HalfFOV = FMath::Clamp(HorizontalFOV, 0.0f, 360.0f) * 0.5f;
    const float DistanceFactor = 1.0f - FMath::Clamp(Distance / VisionDistance, 0.0f, 1.0f);
    const float AngleFactor = HalfFOV > UE_SMALL_NUMBER ? 1.0f - FMath::Clamp(FMath::Abs(Yaw) / HalfFOV, 0.0f, 1.0f) : 1.0f;
    // LOS is a binary gate before scoring.
    return FMath::Clamp(0.5f * DistanceFactor + 0.5f * AngleFactor, 0.0f, 1.0f);
}

void UEnemyVisionComponent::DrawVisionDebug(const FVector& Eye, const FVector& Forward, const FVector& Right) const
{
    if (!FMath::IsFinite(VisionDistance) || !FMath::IsFinite(HorizontalFOV))
    {
        return;
    }
    const float Range = FMath::Max(0.0f, VisionDistance);
    const float HalfFOV = FMath::Clamp(HorizontalFOV, 0.0f, 360.0f) * 0.5f;
    const float Lifetime = GetSafeInterval();
    UWorld* World = GetWorld();
    const auto DirectionAtAngle = [&Forward, &Right](float Degrees)
    {
        const float Radians = FMath::DegreesToRadians(Degrees);
        return Forward * FMath::Cos(Radians) + Right * FMath::Sin(Radians);
    };
    DrawDebugDirectionalArrow(World, Eye, Eye + Forward * Range, 30.0f, FColor::Cyan, false, Lifetime);
    for (const float Sign : { -1.0f, 1.0f })
    {
        DrawDebugLine(World, Eye, Eye + DirectionAtAngle(Sign * HalfFOV) * Range,
            FColor::Yellow, false, Lifetime);
    }
    FVector PreviousPoint = Eye + DirectionAtAngle(-HalfFOV) * Range;
    constexpr int32 Segments = 32;
    for (int32 Index = 1; Index <= Segments; ++Index)
    {
        const float Angle = FMath::Lerp(-HalfFOV, HalfFOV, static_cast<float>(Index) / Segments);
        const FVector Point = Eye + DirectionAtAngle(Angle) * Range;
        DrawDebugLine(World, PreviousPoint, Point, FColor::Yellow, false, Lifetime);
        PreviousPoint = Point;
    }
}
