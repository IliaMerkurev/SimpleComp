#include "Components/Spawning/SCCollectorComponent.h"

#include "Components/Spawning/SCStackComponent.h"
#include "Core/Interfaces/SCCollectableInterface.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogSCCollector, Log, All);

USCCollectorComponent::USCCollectorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    SphereVolume = CreateDefaultSubobject<USphereComponent>(TEXT("SphereVolume"));
    SphereVolume->SetupAttachment(this);
    SphereVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    SphereVolume->SetGenerateOverlapEvents(true);

    BoxVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxVolume"));
    BoxVolume->SetupAttachment(this);
    BoxVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    BoxVolume->SetGenerateOverlapEvents(false);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void USCCollectorComponent::OnRegister()
{
    Super::OnRegister();
    UpdateVolumeState();
}

void USCCollectorComponent::BeginPlay()
{
    Super::BeginPlay();

    ensure(GetOwner() != nullptr);
    
    if (IsValid(SphereVolume))
    {
        SphereVolume->OnComponentBeginOverlap.AddDynamic(this, &USCCollectorComponent::OnTriggerBeginOverlap);
    }
    
    if (IsValid(BoxVolume))
    {
        BoxVolume->OnComponentBeginOverlap.AddDynamic(this, &USCCollectorComponent::OnTriggerBeginOverlap);
    }
}

void USCCollectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(SphereVolume))
    {
        SphereVolume->OnComponentBeginOverlap.RemoveAll(this);
    }

    if (IsValid(BoxVolume))
    {
        BoxVolume->OnComponentBeginOverlap.RemoveAll(this);
    }

    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void USCCollectorComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    UpdateVolumeState();
}
#endif

// ---------------------------------------------------------------------------
// Trigger State
// ---------------------------------------------------------------------------

void USCCollectorComponent::UpdateVolumeState()
{
    if (!IsValid(SphereVolume) || !IsValid(BoxVolume))
    {
        return;
    }

    if (CollisionShape == ESCCollectorShape::Sphere)
    {
        SphereVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        SphereVolume->SetGenerateOverlapEvents(true);
        SphereVolume->SetHiddenInGame(false);
        SphereVolume->SetVisibility(true);

        BoxVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BoxVolume->SetGenerateOverlapEvents(false);
        BoxVolume->SetHiddenInGame(true);
        BoxVolume->SetVisibility(false);
    }
    else
    {
        BoxVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        BoxVolume->SetGenerateOverlapEvents(true);
        BoxVolume->SetHiddenInGame(false);
        BoxVolume->SetVisibility(true);

        SphereVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SphereVolume->SetGenerateOverlapEvents(false);
        SphereVolume->SetHiddenInGame(true);
        SphereVolume->SetVisibility(false);
    }
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

    USCStackComponent* TargetStackComponent = Cast<USCStackComponent>(StackComponentRef.GetComponent(GetOwner()));
    
    // Fallback: If StackComponentRef is completely empty, try to find a stack on the owner
    if (!IsValid(TargetStackComponent) && StackComponentRef.OtherActor == nullptr && StackComponentRef.ComponentProperty == NAME_None)
    {
        TargetStackComponent = GetOwner()->FindComponentByClass<USCStackComponent>();
    }

    if (!IsValid(TargetStackComponent))
    {
        UE_LOG(LogSCCollector, Warning,
            TEXT("USCCollectorComponent on '%s': StackComponentRef is not valid and no Stack was found on Owner."),
            *GetOwner()->GetName());
        
        if (bEnableDebug && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("Collector [%s]: Overlapped %s, but Stack Reference is missing!"), *GetOwner()->GetName(), *OtherActor->GetName()));
        }
        return;
    }

    if (CollectableClass && !OtherActor->IsA(CollectableClass))
    {
        if (bEnableDebug && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Collector [%s]: Ignored %s (Failed Class Filter)"), *GetOwner()->GetName(), *OtherActor->GetName()));
        }
        return;
    }

    if (!OtherActor->Implements<USCCollectableInterface>())
    {
        if (bEnableDebug && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Collector [%s]: Ignored %s (No SCCollectableInterface)"), *GetOwner()->GetName(), *OtherActor->GetName()));
        }
        return;
    }

    const int32 SlotID = TargetStackComponent->RequestSlot();
    if (SlotID == INDEX_NONE)
    {
        UE_LOG(LogSCCollector, Verbose,
            TEXT("USCCollectorComponent on '%s': Stack is full. Ignoring overlap with '%s'."),
            *GetOwner()->GetName(), *OtherActor->GetName());
            
        if (bEnableDebug && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, FString::Printf(TEXT("Collector [%s]: Ignored %s (Stack is FULL)"), *GetOwner()->GetName(), *OtherActor->GetName()));
        }
        return;
    }

    // Resource logic (physics/collision) is handled on the Blueprint side by the user.
    ISCCollectableInterface::Execute_InitFlight(OtherActor, TargetStackComponent, SlotID);
    
    if (bEnableDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("Collector [%s]: Collected %s! Sent to Slot %d"), *GetOwner()->GetName(), *OtherActor->GetName(), SlotID));
    }
    
    OnResourceCollected.Broadcast(OtherActor);
}
