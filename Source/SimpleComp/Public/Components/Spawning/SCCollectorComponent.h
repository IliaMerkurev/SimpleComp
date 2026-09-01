#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "Core/SCTypes.h"
#include "SCCollectorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceCollectedSignature, AActor*, CollectedResource);

class USCStackComponent;
class UPrimitiveComponent;

// ---------------------------------------------------------------------------
// Collector Component
// ---------------------------------------------------------------------------

/**
 * USCCollectorComponent
 * Detects nearby collectable resource Actors and initiates their flight toward a USCStackComponent.
 * Creates a runtime collision shape (Sphere or Box) and filters overlaps by Actor class.
 * Communicates with resources via ISCCollectableInterface.
 */
UCLASS(ClassGroup = (SimpleComp), meta = (BlueprintSpawnableComponent, DisplayName = "Simple Collector Component"))
class SIMPLECOMP_API USCCollectorComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    USCCollectorComponent();

    // -----------------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------------

    /** Fired when a valid collectable resource enters the trigger volume and is successfully assigned a slot. */
    UPROPERTY(BlueprintAssignable, Category = "SimpleComp|Collector|Events")
    FOnResourceCollectedSignature OnResourceCollected;

    /** Enable debug logs and on-screen messages for this collector. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Collector|Debug")
    bool bEnableDebug = false;

    // -----------------------------------------------------------------------
    // Configuration — Trigger
    // -----------------------------------------------------------------------

    /** Geometric shape of the overlap trigger volume. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Collector|Trigger")
    ESCCollectorShape CollisionShape = ESCCollectorShape::Sphere;

    // -----------------------------------------------------------------------
    // Configuration — Collection
    // -----------------------------------------------------------------------

    /**
     * The Stack Component that collected resources will be sent to.
     * If left completely empty (both Actor and Component), the collector will 
     * automatically search for a USCStackComponent on this same Actor.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Collector|Collection")
    FComponentReference StackComponentRef;

    /**
     * Only Actors of this class (or a subclass) that also implement ISCCollectableInterface
     * will be collected. Leave empty to accept any ISCCollectableInterface implementor.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Collector|Collection")
    TSubclassOf<AActor> CollectableClass;

    // -----------------------------------------------------------------------
    // Runtime — Trigger (read-only)
    // -----------------------------------------------------------------------

    /** The sphere trigger volume. Active when CollisionShape is Sphere. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SimpleComp|Collector")
    TObjectPtr<class USphereComponent> SphereVolume;

    /** The box trigger volume. Active when CollisionShape is Box. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SimpleComp|Collector")
    TObjectPtr<class UBoxComponent> BoxVolume;

protected:
    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void UpdateVolumeState();
    UFUNCTION()
    void OnTriggerBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor*              OtherActor,
        UPrimitiveComponent* OtherComp,
        int32                OtherBodyIndex,
        bool                 bFromSweep,
        const FHitResult&    SweepResult);
};
