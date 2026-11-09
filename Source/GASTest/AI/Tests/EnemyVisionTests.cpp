#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Perception/EnemyVisionComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyVisionAcceptanceTest,
    "GASTest.AI.EnemyVision.Acceptance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyVisionAcceptanceTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    auto SpawnBody = [World](const FVector& Location, ECollisionChannel ObjectType, const FVector& Extent)
    {
        AActor* Actor = World->SpawnActor<AActor>();
        UBoxComponent* Body = NewObject<UBoxComponent>(Actor);
        Actor->AddInstanceComponent(Body);
        Actor->SetRootComponent(Body);
        Body->SetBoxExtent(Extent);
        Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Body->SetCollisionObjectType(ObjectType);
        Body->SetCollisionResponseToAllChannels(ECR_Block);
        Body->RegisterComponent();
        Actor->SetActorLocation(Location);
        return Actor;
    };

    AActor* Owner = SpawnBody(FVector::ZeroVector, ECC_Pawn, FVector(20.0));
    UEnemyVisionComponent* Vision = NewObject<UEnemyVisionComponent>(Owner);
    Owner->AddInstanceComponent(Vision);
    Vision->bDrawVisionDebug = false;
    Vision->bEnableVisionLogs = false;
    Vision->RegisterComponent();
    AActor* Target = SpawnBody(FVector(600.0, 0.0, 0.0), ECC_Pawn, FVector(30.0, 30.0, 90.0));
    Target->Tags.Add(TEXT("Player"));
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    auto Sample = [&Fixture, Vision]()
    {
        // One frame exceeds the configured interval; one-shot timer samples once.
        Fixture.TickTestWorld(0.11f);
        return Vision->GetLatestObservation();
    };
    Sample(); // Prime timer scheduling.
    FEnemyObservation Observation = Sample();
    TestTrue(TEXT("1: Ahead is visible (target's own blocking body accepted)"), Observation.bTargetVisible);
    TestTrue(TEXT("1: Forward bearing and confidence"), FMath::IsNearlyZero(Observation.RelativeYaw)
        && Observation.RelativeDirection.X > 0.99 && Observation.EstimatedDistance > 0.0f
        && Observation.Confidence > 0.0f && Observation.Confidence <= 1.0f);

    Target->SetActorLocation(FVector(600.0, 300.0, 0.0));
    Observation = Sample();
    TestTrue(TEXT("2: Right is positive"), Observation.bTargetVisible && Observation.RelativeYaw > 0.0f && Observation.RelativeDirection.Y > 0.0);
    Target->SetActorLocation(FVector(600.0, -300.0, 0.0));
    Observation = Sample();
    TestTrue(TEXT("3: Left is negative"), Observation.bTargetVisible && Observation.RelativeYaw < 0.0f && Observation.RelativeDirection.Y < 0.0);

    Target->SetActorLocation(FVector(800.0, 0.0, 0.0));
    const float PreviousDistance = Sample().EstimatedDistance;
    Target->SetActorLocation(FVector(400.0, 0.0, 0.0));
    Observation = Sample();
    TestTrue(TEXT("4: Approach"), Observation.EstimatedDistance < PreviousDistance && Observation.ClosingSpeed > 0.0f);
    Target->SetActorLocation(FVector(800.0, 0.0, 0.0));
    TestTrue(TEXT("5: Retreat"), Sample().ClosingSpeed < 0.0f);
    Target->SetActorLocation(FVector(800.0, 100.0, 0.0));
    TestTrue(TEXT("6: Right strafe"), Sample().AngularVelocity > 0.0f);

    auto CheckInvisible = [this](const TCHAR* Label, const FEnemyObservation& Value)
    {
        TestTrue(Label, !Value.bTargetVisible && Value.ObservedWorldPosition.IsZero() && Value.RelativeDirection.IsNearlyZero()
            && Value.RelativeYaw == 0.0f && Value.EstimatedDistance == 0.0f
            && Value.ClosingSpeed == 0.0f && Value.AngularVelocity == 0.0f && Value.Confidence == 0.0f);
    };
    Target->SetActorLocation(FVector(-600.0, 0.0, 0.0));
    CheckInvisible(TEXT("7: Behind resets visual data"), Sample());
    Target->SetActorLocation(FVector(600.0, 0.0, 0.0));
    AActor* Wall = SpawnBody(FVector(300.0, 0.0, 0.0), ECC_WorldStatic, FVector(20.0, 200.0, 200.0));
    CheckInvisible(TEXT("8: Wall blocks visibility"), Sample());
    Vision->bRequireLineOfSight = false;
    TestTrue(TEXT("LOS can be disabled"), Sample().bTargetVisible);
    Vision->bRequireLineOfSight = true;
    Wall->Destroy();
    Target->SetActorLocation(FVector(1600.0, 0.0, 0.0));
    CheckInvisible(TEXT("9: Out of range"), Sample());
    Target->SetActorLocation(FVector(600.0, 0.0, 0.0));
    Observation = Sample();
    TestTrue(TEXT("10: Reacquisition resets rates"), Observation.bTargetVisible
        && Observation.ClosingSpeed == 0.0f && Observation.AngularVelocity == 0.0f);

    AActor* Second = SpawnBody(FVector(300.0, 0.0, 0.0), ECC_Pawn, FVector(20.0));
    Second->Tags.Add(TEXT("Player"));
    Observation = Sample();
    TestTrue(TEXT("Closer centered target selected without cross-target rates"), Observation.bTargetVisible
        && Observation.EstimatedDistance < 400.0f && Observation.ClosingSpeed == 0.0f && Observation.AngularVelocity == 0.0f);
    Second->Destroy();
    Target->Tags.Reset();
    CheckInvisible(TEXT("Actor tag is required"), Sample());
    Target->Tags.Add(TEXT("Player"));
    Vision->HorizontalFOV = 360.0f;
    Target->SetActorLocation(FVector(-600.0, 10.0, 0.0));
    Sample();
    Target->SetActorLocation(FVector(-600.0, -10.0, 0.0));
    Observation = Sample();
    TestTrue(TEXT("Yaw wraps through rear with small positive rate"), Observation.bTargetVisible
        && Observation.AngularVelocity > 0.0f && Observation.AngularVelocity < 100.0f);

    Owner->Destroy();
    Fixture.TickTestWorld(0.2f);

    AEnemyCharacter* Enemy = World->SpawnActor<AEnemyCharacter>();
    Enemy->GetCharacterMovement()->DisableMovement();
    UEnemyVisionComponent* PointVision = Enemy->GetEnemyVisionComponent();
    PointVision->bDrawVisionDebug = false;
    PointVision->bEnableVisionLogs = false;
    PointVision->EyeOffset = FVector(9000.0); // Must be ignored with a valid point.
    USceneComponent* Point = Enemy->GetVisionPoint();
    TestNotNull(TEXT("Enemy owns VisionPoint"), Point);
    if (!Point)
    {
        return false;
    }
    Point->SetRelativeLocation(FVector(100.0, 50.0, 120.0));
    Point->SetRelativeRotation(FRotator(15.0, 75.0, 10.0));
    auto PointSample = [&Fixture, PointVision]()
    {
        Fixture.TickTestWorld(0.11f);
        return PointVision->GetLatestObservation();
    };
    Target->SetActorLocation(Point->GetComponentLocation() + Point->GetForwardVector() * 600.0);
    PointSample();
    Observation = PointSample();
    TestTrue(TEXT("VisionPoint world location and full forward determine center"), Observation.bTargetVisible
        && FMath::IsNearlyZero(Observation.RelativeYaw, 0.01f)
        && FMath::IsNearlyEqual(Observation.EstimatedDistance, 600.0f, 0.1f)
        && Observation.RelativeDirection.X > 0.99);
    TestTrue(TEXT("Observed point preserves height with tilted VisionPoint"),
        Observation.ObservedWorldPosition.Equals(Target->GetActorLocation(), 0.01));
    Target->SetActorLocation(Point->GetComponentLocation() + Point->GetForwardVector() * 600.0
        + Point->GetRightVector() * 300.0);
    Observation = PointSample();
    TestTrue(TEXT("VisionPoint right determines yaw and relative direction"), Observation.bTargetVisible
        && FMath::IsNearlyEqual(Observation.RelativeYaw, 26.565f, 0.01f)
        && Observation.RelativeDirection.Y > 0.0);
    Enemy->GetMesh()->SetRelativeRotation(FRotator(0.0, -135.0, 0.0));
    const FEnemyObservation AfterMeshRotation = PointSample();
    TestTrue(TEXT("Mesh rotation does not alter perception"), AfterMeshRotation.bTargetVisible
        && FMath::IsNearlyEqual(AfterMeshRotation.RelativeYaw, Observation.RelativeYaw, 0.01f));
    Target->SetActorLocation(Point->GetComponentLocation() - Point->GetForwardVector() * 600.0);
    CheckInvisible(TEXT("VisionPoint rear rejected independently of actor forward"), PointSample());
    Enemy->Destroy();
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
