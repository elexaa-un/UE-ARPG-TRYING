#if WITH_DEV_AUTOMATION_TESTS

#include "AI/EnemyBrainComponent.h"
#include "AI/Perception/EnemyVisionComponent.h"
#include "Components/BoxComponent.h"
#include "EnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyMemoryAcceptanceTest,
    "GASTest.AI.EnemyMemory.Acceptance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyMemoryAcceptanceTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    AEnemyCharacter* Enemy = World->SpawnActor<AEnemyCharacter>();
    // Keep the observer fixed while exercising real Vision timers and Brain ticks.
    Enemy->GetCharacterMovement()->DisableMovement();
    UEnemyBrainComponent* Brain = Enemy->FindComponentByClass<UEnemyBrainComponent>();
    UEnemyVisionComponent* Vision = Enemy->GetEnemyVisionComponent();
    Vision->bDrawVisionDebug = false;
    Vision->bEnableVisionLogs = false;

    auto SpawnBox = [World](FVector Position, ECollisionChannel Channel, FVector Extent)
    {
        AActor* Actor = World->SpawnActor<AActor>();
        UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
        Actor->AddInstanceComponent(Box);
        Actor->SetRootComponent(Box);
        Box->SetBoxExtent(Extent);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionObjectType(Channel);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent();
        Actor->SetActorLocation(Position);
        return Actor;
    };
    AActor* Target = SpawnBox(FVector(-600, 0, 0), ECC_Pawn, FVector(25, 25, 90));
    Target->Tags.Add(TEXT("Player"));
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    // BeginPlay initializes CharacterMovement's default mode; freeze it afterwards.
    Enemy->GetCharacterMovement()->DisableMovement();
    Enemy->GetCharacterMovement()->SetComponentTickEnabled(false);
    auto Advance = [&Fixture](int32 Frames)
    {
        for (int32 Index = 0; Index < Frames; ++Index)
        {
            Fixture.TickTestWorld(0.05f);
        }
    };
    Advance(10);
    TestTrue(TEXT("Never seen: Idle with no memory"), Brain->CurrentState == EEnemyAIState::Idle
        && !Brain->Memory.bHasLastSeenTarget && !Enemy->HasMoveInput());

    Target->SetActorLocation(FVector(600, 0, 0));
    Advance(10);
    TestTrue(TEXT("Visible: Approach and remember accepted world point"), Brain->CurrentState == EEnemyAIState::Approach
        && Brain->Memory.bHasLastSeenTarget && Brain->Memory.LastSeenPosition.Equals(Target->GetActorLocation())
        && Brain->Memory.TimeSinceLastSeen == 0.0f);
    Target->SetActorLocation(FVector(650, 80, 0));
    Advance(10);
    TestTrue(TEXT("Visible memory follows latest observation"),
        Brain->Memory.LastSeenPosition.Equals(Vision->GetLatestObservation().ObservedWorldPosition)
        && Brain->Memory.LastSeenPosition.Equals(Target->GetActorLocation()));
    const FVector LastSeen = Brain->Memory.LastSeenPosition;
    const double SeenTime = Vision->GetLatestObservation().ObservationTime;

    AActor* Wall = SpawnBox(FVector(300, 0, 0), ECC_WorldStatic, FVector(20, 250, 250));
    Advance(10);
    TestTrue(TEXT("Occlusion: Search and no live observed position"), Brain->CurrentState == EEnemyAIState::Search
        && !Vision->GetLatestObservation().bTargetVisible
        && Vision->GetLatestObservation().ObservedWorldPosition.IsZero());
    Target->SetActorLocation(FVector(-700, 400, 0));
    Advance(10);
    TestTrue(TEXT("Hidden movement cannot change memory"), Brain->Memory.LastSeenPosition.Equals(LastSeen));
    TestTrue(TEXT("Search input points at memory, not hidden target"),
        Enemy->GetLastMoveInputDirection().Equals((LastSeen - Enemy->GetActorLocation()).GetSafeNormal2D(), 0.001));
    TestTrue(TEXT("Age uses actual last visible sample time"),
        FMath::IsNearlyEqual(Brain->Memory.TimeSinceLastSeen, static_cast<float>(World->GetTimeSeconds() - SeenTime), 0.06f));

    Enemy->SetActorLocation(LastSeen - FVector(50, 0, 0));
    Advance(5);
    TestTrue(TEXT("Arrival stops but keeps Search and memory"), Brain->CurrentState == EEnemyAIState::Search
        && Brain->Memory.bHasLastSeenTarget && !Enemy->HasMoveInput());
    // A long frame must advance the memory clock by real elapsed time, not ThinkInterval.
    Fixture.TickTestWorld(0.3f);
    TestTrue(TEXT("Long frame preserves sample-based age"),
        FMath::IsNearlyEqual(Brain->Memory.TimeSinceLastSeen,
            static_cast<float>(World->GetTimeSeconds() - SeenTime), 0.06f));
    while (World->GetTimeSeconds() - SeenTime < Brain->MemoryForgetTime + 0.1)
    {
        Fixture.TickTestWorld(0.05f);
    }
    Advance(2);
    TestTrue(TEXT("Forget clears every field and stops"), Brain->CurrentState == EEnemyAIState::Idle
        && !Brain->Memory.bHasLastSeenTarget && Brain->Memory.LastSeenPosition.IsZero()
        && Brain->Memory.TimeSinceLastSeen == 0.0f && !Enemy->HasMoveInput());

    Wall->Destroy();
    Enemy->SetActorLocation(FVector::ZeroVector);
    Target->SetActorLocation(FVector(600, 0, 0));
    Advance(10);
    Target->SetActorLocation(FVector(-600, 0, 0));
    Advance(6);
    TestTrue(TEXT("Loss enters Search again"), Brain->CurrentState == EEnemyAIState::Search);
    Target->SetActorLocation(FVector(450, 50, 0));
    Advance(6);
    TestTrue(TEXT("Reacquisition restores Approach and resets age"), Brain->CurrentState == EEnemyAIState::Approach
        && Brain->Memory.LastSeenPosition.Equals(Target->GetActorLocation()) && Brain->Memory.TimeSinceLastSeen == 0.0f);
    Target->SetActorLocation(FVector(-600, 0, 0));
    Advance(6);
    Target->SetActorLocation(FVector(140, 0, 0));
    Advance(6);
    TestTrue(TEXT("Near reacquisition restores Attack and stops input"), Brain->CurrentState == EEnemyAIState::Attack
        && Brain->Memory.TimeSinceLastSeen == 0.0f && !Enemy->HasMoveInput());
    // This test has no granted attack ability; physical montage/hit validation remains PIE work.
    Enemy->Destroy();
    Advance(2);
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
