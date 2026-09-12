#include "Components/Spawning/SCCollectorComponent.h"

#include "Components/Spawning/SCStackComponent.h"
#include "Core/Interfaces/SCCollectableInterface.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

    bRuntimeActive = true;
    bRefreshInitialOverlaps = true;

    if (IsValid(SphereVolume))
    {
        SphereVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &USCCollectorComponent::OnTriggerBeginOverlap);
    }
    
    if (IsValid(BoxVolume))
    {
        BoxVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &USCCollectorComponent::OnTriggerBeginOverlap);
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SettingsTimer, this, &USCCollectorComponent::RefreshSettings, 0.1f, true);
    }
}

void USCCollectorComponent::Cleanup()
{
    bRuntimeActive = false;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SettingsTimer);
    }
    if (IsValid(SphereVolume))
    {
        SphereVolume->OnComponentBeginOverlap.RemoveAll(this);
    }
    if (IsValid(BoxVolume))
    {
        BoxVolume->OnComponentBeginOverlap.RemoveAll(this);
    }
    // Living resources retain responsibility for flights already handed off through InitFlight.
    Collections.Empty();
}

void USCCollectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Cleanup();
    Super::EndPlay(EndPlayReason);
}

void USCCollectorComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Cleanup();
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void USCCollectorComponent::RefreshSettings()
{
    if (!bRuntimeActive || bRefreshingSettings)
    {
        return;
    }
    TGuardValue<bool> Guard(bRefreshingSettings, true);
    for (auto It = Collections.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            if (It.Value().Stack.IsValid())
            {
                It.Value().Stack->ReleaseSlot(It.Value().Ticket);
            }
            It.RemoveCurrent();
        }
        else if (!It.Value().Stack.IsValid() || !It.Value().Stack->HasTicket(It.Value().Ticket))
        {
            It.RemoveCurrent();
        }
    }
    const bool bShapeChanged = AppliedShape != ActiveShapeType;
    if (bShapeChanged)
    {
        UpdateVolumeState();
    }
    if (!IsValid(this) || !bRuntimeActive)
    {
        return;
    }
    if (bShapeChanged || bRefreshInitialOverlaps)
    {
        bRefreshInitialOverlaps = false;
        UPrimitiveComponent* Volume = ActiveShapeType == ESCCollectorShape::Sphere
                                          ? static_cast<UPrimitiveComponent*>(SphereVolume.Get())
                                          : static_cast<UPrimitiveComponent*>(BoxVolume.Get());
        if (!IsValid(Volume))
        {
            return;
        }
        TArray<AActor*> Actors;
        Volume->GetOverlappingActors(Actors);
        for (AActor* Actor : Actors)
        {
            OnTriggerBeginOverlap(Volume, Actor, nullptr, INDEX_NONE, false, FHitResult());
            if (!IsValid(this) || !bRuntimeActive)
            {
                return;
            }
        }
    }
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

    AppliedShape = ActiveShapeType;
    UPrimitiveComponent* Active = ActiveShapeType == ESCCollectorShape::Sphere
                                      ? static_cast<UPrimitiveComponent*>(SphereVolume.Get())
                                      : static_cast<UPrimitiveComponent*>(BoxVolume.Get());
    UPrimitiveComponent* Inactive = ActiveShapeType == ESCCollectorShape::Sphere
                                        ? static_cast<UPrimitiveComponent*>(BoxVolume.Get())
                                        : static_cast<UPrimitiveComponent*>(SphereVolume.Get());
    Inactive->SetGenerateOverlapEvents(false);
    if (!IsValid(this) || !IsValid(Inactive) || !IsValid(Active))
    {
        return;
    }
    Inactive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Inactive->SetHiddenInGame(true);
    Inactive->SetVisibility(false);
    Active->SetHiddenInGame(false);
    Active->SetVisibility(true);
    Active->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    if (!IsValid(this) || !IsValid(Active))
    {
        return;
    }
    Active->SetGenerateOverlapEvents(true);
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
    if (!bRuntimeActive || !IsValid(GetOwner()) || !IsValid(OtherActor) || OtherActor == GetOwner())
    {
        return;
    }

    UPrimitiveComponent* ActiveVolume = ActiveShapeType == ESCCollectorShape::Sphere
                                            ? static_cast<UPrimitiveComponent*>(SphereVolume.Get())
                                            : static_cast<UPrimitiveComponent*>(BoxVolume.Get());
    if (OverlappedComponent != ActiveVolume)
    {
        return;
    }
    if (const FCollection* Existing = Collections.Find(OtherActor))
    {
        if (Existing->Stack.IsValid() && Existing->Stack->HasTicket(Existing->Ticket))
        {
            return;
        }
    }

    USCStackComponent* TargetStackComponent = Cast<USCStackComponent>(StackComponentRef.GetComponent(GetOwner()));
    
    // Fallback: If StackComponentRef is completely empty, try to find a stack on the owner
    if (!IsValid(TargetStackComponent) && StackComponentRef.OtherActor == nullptr &&
        StackComponentRef.ComponentProperty == NAME_None && StackComponentRef.PathToComponent.IsEmpty())
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

    Collections.Add(OtherActor, FCollection{TargetStackComponent, SlotID});
    // Record the assignment before calling resource code, which can trigger further overlaps.
    ISCCollectableInterface::Execute_InitFlight(OtherActor, TargetStackComponent, SlotID);
    if (!IsValid(OtherActor))
    {
        if (IsValid(TargetStackComponent))
        {
            TargetStackComponent->ReleaseSlot(SlotID);
        }
        return;
    }
    if (!IsValid(this) || !bRuntimeActive)
    {
        return;
    }

    if (bEnableDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("Collector [%s]: Collected %s! Sent to Slot %d"), *GetOwner()->GetName(), *OtherActor->GetName(), SlotID));
    }
    
    OnResourceCollected.Broadcast(OtherActor);
}
