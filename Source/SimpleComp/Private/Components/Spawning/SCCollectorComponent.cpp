#include "Components/Spawning/SCCollectorComponent.h"

#include "Components/Spawning/SCStackComponent.h"
#include "Core/Interfaces/SCCollectableInterface.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogSCCollector, Log, All);

USCCollectorComponent::USCCollectorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void USCCollectorComponent::BeginPlay()
{
    Super::BeginPlay();

    ensure(GetOwner() != nullptr);

    CreateTriggerVolume();
}

void USCCollectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(TriggerVolume))
    {
        TriggerVolume->OnComponentBeginOverlap.RemoveAll(this);
    }

    Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Trigger Creation
// ---------------------------------------------------------------------------

void USCCollectorComponent::CreateTriggerVolume()
{
    AActor* Owner = GetOwner();
    if (!ensure(IsValid(Owner)))
    {
        return;
    }

    USceneComponent* OwnerRoot = Owner->GetRootComponent();
    if (!ensure(IsValid(OwnerRoot)))
    {
        return;
    }

    UPrimitiveComponent* NewVolume = nullptr;

    if (CollisionShape == ESCCollectorShape::Sphere)
    {
        USphereComponent* Sphere = NewObject<USphereComponent>(
            Owner,
            USphereComponent::StaticClass(),
            TEXT("SCCollectorSphere"));

        Sphere->SetSphereRadius(SphereRadius);
        NewVolume = Sphere;
    }
    else
    {
        UBoxComponent* Box = NewObject<UBoxComponent>(
            Owner,
            UBoxComponent::StaticClass(),
            TEXT("SCCollectorBox"));

        Box->SetBoxExtent(BoxExtent);
        NewVolume = Box;
    }

    NewVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    NewVolume->SetGenerateOverlapEvents(true);
    NewVolume->RegisterComponent();
    NewVolume->AttachToComponent(OwnerRoot, FAttachmentTransformRules::KeepRelativeTransform);

    TriggerVolume = NewVolume;

    TriggerVolume->OnComponentBeginOverlap.AddDynamic(
        this,
        &USCCollectorComponent::OnTriggerBeginOverlap);
}

// ---------------------------------------------------------------------------
// Overlap Handler
// ---------------------------------------------------------------------------

void USCCollectorComponent::OnTriggerBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor*              OtherActor,
    UPrimitiveComponent* OtherComp,
    int32                OtherBodyIndex,
    bool                 bFromSweep,
    const FHitResult&    SweepResult)
{
    if (!IsValid(OtherActor))
    {
        return;
    }

    if (!IsValid(TargetStackComponent))
    {
        UE_LOG(LogSCCollector, Warning,
            TEXT("USCCollectorComponent on '%s': TargetStackComponent is not set."),
            *GetOwner()->GetName());
        return;
    }

    if (CollectableClass && !OtherActor->IsA(CollectableClass))
    {
        return;
    }

    if (!OtherActor->Implements<USCCollectableInterface>())
    {
        return;
    }

    const int32 SlotID = TargetStackComponent->RequestSlot();
    if (SlotID == INDEX_NONE)
    {
        UE_LOG(LogSCCollector, Verbose,
            TEXT("USCCollectorComponent on '%s': Stack is full. Ignoring overlap with '%s'."),
            *GetOwner()->GetName(), *OtherActor->GetName());
        return;
    }

    TArray<UPrimitiveComponent*> ResourcePrimitives;
    OtherActor->GetComponents<UPrimitiveComponent>(ResourcePrimitives);

    for (UPrimitiveComponent* Primitive : ResourcePrimitives)
    {
        if (IsValid(Primitive))
        {
            Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    ISCCollectableInterface::Execute_InitFlight(OtherActor, TargetStackComponent, SlotID);
}
