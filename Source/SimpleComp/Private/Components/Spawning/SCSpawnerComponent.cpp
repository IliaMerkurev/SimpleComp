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
    if (bShuttingDown)
    {
        return;
    }
    StopSpawn();
    bIsManuallyStopped = false;
    bAppliedFlow = bIsFlow;
    AppliedFlowInterval = FMath::IsFinite(FlowInterval) ? FMath::Max(0.f, FlowInterval) : 0.f;
    AppliedFlowDuration = FMath::IsFinite(FlowTimer) ? FMath::Max(0.f, FlowTimer) : 0.f;
    AppliedRepeatInterval = FMath::IsFinite(AutoRepeatInterval) ? FMath::Max(0.f, AutoRepeatInterval) : 0.f;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SettingsTimerHandle, this, &USCSpawnerComponent::RefreshFlowSettings, 0.1f,
                                          true);
    }
    StartActivePhase();
}

void USCSpawnerComponent::RefreshFlowSettings()
{
    if (bIsManuallyStopped || bShuttingDown)
    {
        return;
    }
    const float Interval = FMath::IsFinite(FlowInterval) ? FMath::Max(0.f, FlowInterval) : 0.f;
    const float Duration = FMath::IsFinite(FlowTimer) ? FMath::Max(0.f, FlowTimer) : 0.f;
    const float Repeat = FMath::IsFinite(AutoRepeatInterval) ? FMath::Max(0.f, AutoRepeatInterval) : 0.f;
    if (bAppliedFlow != bIsFlow || AppliedFlowInterval != Interval || AppliedFlowDuration != Duration ||
        AppliedRepeatInterval != Repeat)
    {
        Spawn();
    }
}

void USCSpawnerComponent::StartActivePhase()
{
    UWorld* World = GetWorld();
    if (!World || bIsManuallyStopped || bShuttingDown)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(RepeatDelayHandle);
    World->GetTimerManager().ClearTimer(FlowTimerHandle);
    World->GetTimerManager().ClearTimer(FlowDurationHandle);
    const uint64 Revision = SpawnRevision;

    if (bIsFlow && FMath::IsFinite(FlowInterval) && FlowInterval > 0.f)
    {
        World->GetTimerManager().SetTimer(FlowTimerHandle, this, &USCSpawnerComponent::ExecuteSpawning, FlowInterval,
            true, 0.0f);

        if (FMath::IsFinite(FlowTimer) && FlowTimer > 0.0f)
        {
            World->GetTimerManager().SetTimer(FlowDurationHandle, this, &USCSpawnerComponent::OnFlowDurationExpired,
                FlowTimer, false);
        }
    }
    else
    {
        ExecuteSpawning();

        if (IsValid(this) && !bShuttingDown && Revision == SpawnRevision && FMath::IsFinite(AutoRepeatInterval) &&
            AutoRepeatInterval > 0.0f && !bIsManuallyStopped)
        {
            World->GetTimerManager().SetTimer(RepeatDelayHandle, this, &USCSpawnerComponent::StartActivePhase,
                AutoRepeatInterval, false);
        }
        else if (Revision == SpawnRevision)
        {
            StopSpawn();
        }
    }
}

void USCSpawnerComponent::OnFlowDurationExpired()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    World->GetTimerManager().ClearTimer(FlowTimerHandle);

    if (!bShuttingDown && FMath::IsFinite(AutoRepeatInterval) && AutoRepeatInterval > 0.0f && !bIsManuallyStopped)
    {
        World->GetTimerManager().SetTimer(RepeatDelayHandle, this, &USCSpawnerComponent::StartActivePhase,
            AutoRepeatInterval, false);
    }
    else
    {
        StopSpawn();
    }
}

void USCSpawnerComponent::StopSpawn()
{
    bIsManuallyStopped = true;
    ++SpawnRevision;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FlowTimerHandle);
        World->GetTimerManager().ClearTimer(FlowDurationHandle);
        World->GetTimerManager().ClearTimer(RepeatDelayHandle);
        World->GetTimerManager().ClearTimer(SettingsTimerHandle);
    }
}

void USCSpawnerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bShuttingDown = true;
    StopSpawn();
    Super::EndPlay(EndPlayReason);
}

void USCSpawnerComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    bShuttingDown = true;
    StopSpawn();
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void USCSpawnerComponent::ExecuteSpawning()
{
    if (!GetWorld() || bIsManuallyStopped || bShuttingDown)
    {
        return;
    }

    const uint64 Revision = SpawnRevision;
    const int32 BurstCount = FMath::Max(0, Count);
    FTransform CompTransform = GetComponentTransform();

    float BaseSpeed = LaunchDirectionWidget.Size() * LaunchMultiplier;
    FVector WorldWidgetDir = CompTransform.TransformVectorNoScale(LaunchDirectionWidget.GetSafeNormal());

    for (int32 i = 0; i < BurstCount; ++i)
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

        FVector BaseDir =
            IsValid(TargetActor) ? (TargetActor->GetActorLocation() - RandomLoc).GetSafeNormal() : WorldWidgetDir;
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

        if (!IsValid(this) || bShuttingDown || bIsManuallyStopped || Revision != SpawnRevision)
        {
            return;
        }
        if (IsValid(NewActor))
        {
            if (NewActor->Implements<USCMessageInterface>())
            {
                FSCMessagePayload Payload;
                Payload.Value = MessageValue;
                Payload.StringMessage = MessageNote;
                Payload.Sender = GetOwner();
                Payload.TargetActor = TargetActor;
                Payload.TransformData = FTransform(FinalRotation, RandomLoc);

                ISCMessageInterface::Execute_OnReceiveSCMessage(NewActor, Payload);
            }

            if (!IsValid(this) || bShuttingDown || bIsManuallyStopped || Revision != SpawnRevision)
            {
                return;
            }
            if (!IsValid(NewActor))
            {
                continue;
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

    double TotalWeight = 0.0;
    for (const FSCWeightedSpawnClass& WeightedClass : SpawnClass)
    {
        if (IsValid(WeightedClass.ActorClass) && FMath::IsFinite(WeightedClass.Weight) && WeightedClass.Weight > 0.0f)
        {
            TotalWeight += WeightedClass.Weight;
        }
    }

    if (TotalWeight <= 0.0f)
    {
        return nullptr;
    }

    const double RandomValue = FMath::FRand() * TotalWeight;
    double CurrentWeight = 0.0;

    for (const FSCWeightedSpawnClass& WeightedClass : SpawnClass)
    {
        if (IsValid(WeightedClass.ActorClass) && FMath::IsFinite(WeightedClass.Weight) && WeightedClass.Weight > 0.0f)
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