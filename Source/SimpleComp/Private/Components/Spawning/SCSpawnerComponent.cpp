#include "Components/Spawning/SCSpawnerComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

USCSpawnerComponent::USCSpawnerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    LaunchDirectionWidget = FVector(100.f, 0.f, 0.f);
    SpawnRadius = FVector(200.f, 200.f, 200.f);
}

void USCSpawnerComponent::Spawn()
{
    bIsManuallyStopped = false;
    StartActivePhase();
}

void USCSpawnerComponent::StartActivePhase()
{
    UWorld* World = GetWorld();
    if (!World || bIsManuallyStopped)
        return;

    World->GetTimerManager().ClearTimer(RepeatDelayHandle);

    if (bIsFlow)
    {
        World->GetTimerManager().SetTimer(FlowTimerHandle, this, &USCSpawnerComponent::ExecuteSpawning, FlowInterval,
            true, 0.0f);

        if (FlowTimer > 0.0f)
        {
            World->GetTimerManager().SetTimer(FlowDurationHandle, this, &USCSpawnerComponent::OnFlowDurationExpired,
                FlowTimer, false);
        }
    }
    else
    {
        ExecuteSpawning();

        if (AutoRepeatInterval > 0.0f && !bIsManuallyStopped)
        {
            World->GetTimerManager().SetTimer(RepeatDelayHandle, this, &USCSpawnerComponent::StartActivePhase,
                AutoRepeatInterval, false);
        }
    }
}

void USCSpawnerComponent::OnFlowDurationExpired()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    World->GetTimerManager().ClearTimer(FlowTimerHandle);

    if (AutoRepeatInterval > 0.0f && !bIsManuallyStopped)
    {
        World->GetTimerManager().SetTimer(RepeatDelayHandle, this, &USCSpawnerComponent::StartActivePhase,
            AutoRepeatInterval, false);
    }
}

void USCSpawnerComponent::StopSpawn()
{
    bIsManuallyStopped = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FlowTimerHandle);
        World->GetTimerManager().ClearTimer(FlowDurationHandle);
        World->GetTimerManager().ClearTimer(RepeatDelayHandle);
    }
}

void USCSpawnerComponent::ExecuteSpawning()
{
    if (!GetWorld())
        return;

    FVector BaseLocation = GetComponentLocation();
    FTransform CompTransform = GetComponentTransform();

    float BaseSpeed = LaunchDirectionWidget.Size() * LaunchMultiplier;
    FVector WorldWidgetDir = CompTransform.TransformVectorNoScale(LaunchDirectionWidget.GetSafeNormal());

    for (int32 i = 0; i < Count; ++i)
    {
        TSubclassOf<AActor> SelectedClass = GetRandomSpawnClass();
        if (!SelectedClass)
        {
            continue;
        }

        FVector RandomLoc;
        if (SpawnShape == ESCSpawnShape::Box)
        {
            FVector Extent = GetUnscaledBoxExtent();
            FBox LocalBox(-Extent, Extent);
            FVector LocalPoint = FMath::RandPointInBox(LocalBox);
            RandomLoc = CompTransform.TransformPosition(LocalPoint);
        }
        else
        {
            FVector UnitPoint = FMath::VRand() * FMath::Pow(FMath::FRand(), 0.333f);
            FVector LocalPoint = UnitPoint * SpawnRadius;
            RandomLoc = CompTransform.TransformPosition(LocalPoint);
        }

        FVector BaseDir = TargetActor ? (TargetActor->GetActorLocation() - RandomLoc).GetSafeNormal() : WorldWidgetDir;
        FVector RandomDir = FMath::VRandCone(BaseDir, FMath::DegreesToRadians(LaunchSpreadAngle));

        float RandomSpeedMod = FMath::FRandRange(1.0f - VelocityRandomness, 1.0f + VelocityRandomness);
        FVector FinalVelocity = RandomDir * (BaseSpeed * RandomSpeedMod);

        FRotator FinalRotation;
        switch (RotationMode)
        {
            case ESCSpawnerRotationMode::Random:
                FinalRotation = FRotator(FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f),
                    FMath::FRandRange(0.f, 360.f));
                break;
            case ESCSpawnerRotationMode::Range:
                FinalRotation.Pitch = FMath::FRandRange(MinRotation.Pitch, MaxRotation.Pitch);
                FinalRotation.Yaw = FMath::FRandRange(MinRotation.Yaw, MaxRotation.Yaw);
                FinalRotation.Roll = FMath::FRandRange(MinRotation.Roll, MaxRotation.Roll);
                break;
            case ESCSpawnerRotationMode::FaceVelocity:
            default:
                FinalRotation = RandomDir.Rotation();
                break;
        }

        if (bShowDebugLines)
        {
            DrawDebugLine(GetWorld(), RandomLoc, RandomLoc + (BaseDir * 100.f), FColor::Green, false, 1.0f, 0, 1.0f);
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AActor* NewActor = GetWorld()->SpawnActor<AActor>(SelectedClass, RandomLoc, FinalRotation, Params);

        if (NewActor)
        {
            if (NewActor->Implements<USCMessageInterface>())
            {
                FSCMessagePayload Payload;
                Payload.Value = MessageValue;
                Payload.StringMessage = MessageNote;
                Payload.Sender = GetOwner();

                Payload.TargetActor = TargetActor;

                ISCMessageInterface::Execute_OnReceiveSCMessage(NewActor, Payload);
            }

            UPrimitiveComponent* PhysComp = Cast<UPrimitiveComponent>(NewActor->GetRootComponent());
            if (!PhysComp)
                PhysComp = NewActor->FindComponentByClass<UPrimitiveComponent>();

            if (PhysComp)
            {
                PhysComp->SetSimulatePhysics(true);
                PhysComp->SetPhysicsLinearVelocity(FinalVelocity);
            }
        }
    }
}

TSubclassOf<AActor> USCSpawnerComponent::GetRandomSpawnClass() const
{
    if (SpawnClass.Num() == 0)
    {
        return nullptr;
    }

    float TotalWeight = 0.0f;
    for (const FSCWeightedSpawnClass& WeightedClass : SpawnClass)
    {
        if (WeightedClass.ActorClass && WeightedClass.Weight > 0.0f)
        {
            TotalWeight += WeightedClass.Weight;
        }
    }

    if (TotalWeight <= 0.0f)
    {
        return nullptr;
    }

    float RandomValue = FMath::FRandRange(0.0f, TotalWeight);
    float CurrentWeight = 0.0f;

    for (const FSCWeightedSpawnClass& WeightedClass : SpawnClass)
    {
        if (WeightedClass.ActorClass && WeightedClass.Weight > 0.0f)
        {
            CurrentWeight += WeightedClass.Weight;
            if (RandomValue <= CurrentWeight)
            {
                return WeightedClass.ActorClass;
            }
        }
    }

    return nullptr;
}