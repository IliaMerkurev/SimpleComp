#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "SCCollectorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceCollectedSignature, AActor*, CollectedResource);

class USCStackComponent;
class UPrimitiveComponent;

// ---------------------------------------------------------------------------
// Collision Shape Enum
// ---------------------------------------------------------------------------

/** The geometric shape of the collection trigger volume. */
UENUM(BlueprintType)
enum class ESCCollectorShape : uint8
{
    /** Spherical trigger volume. */
    Sphere UMETA(DisplayName = "Sphere"),
    /** Box trigger volume. */
    Box    UMETA(DisplayName = "Box")
};

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

    // -----------------------------------------------------------------------
    // Configuration — Trigger
    // -----------------------------------------------------------------------

    /** Geometric shape of the overlap trigger volume. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Collector|Trigger")
    ESCCollectorShape CollisionShape = ESCCollectorShape::Sphere;

    /** Radius of the sphere trigger (cm). Active when CollisionShape is Sphere. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Collector|Trigger",
        meta = (EditCondition = "CollisionShape == ESCCollectorShape::Sphere", ClampMin = "1.0", UIMin = "1.0"))
    float SphereRadius = 200.0f;

    /** Half-extents of the box trigger (cm). Active when CollisionShape is Box. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Collector|Trigger",
        meta = (EditCondition = "CollisionShape == ESCCollectorShape::Box"))
    FVector BoxExtent = FVector(200.0f);

    // -----------------------------------------------------------------------
    // Configuration — Collection
    // -----------------------------------------------------------------------

    /**
     * The Stack Component that collected resources will be sent to.
     * If left empty, the collector will automatically search for a USCStackComponent on this same Actor.
     * Can point to a stack in the same Blueprint or on a different Actor in the level.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Collector|Collection")
    TObjectPtr<USCStackComponent> TargetStackComponent;

    /**
     * Only Actors of this class (or a subclass) that also implement ISCCollectableInterface
     * will be collected. Leave empty to accept any ISCCollectableInterface implementor.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Collector|Collection")
    TSubclassOf<AActor> CollectableClass;

    // -----------------------------------------------------------------------
    // Runtime — Trigger (read-only)
    // -----------------------------------------------------------------------

    /** The dynamically created collision primitive. Created at BeginPlay. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SimpleComp|Collector")
    TObjectPtr<UPrimitiveComponent> TriggerVolume;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void OnTriggerBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor*              OtherActor,
        UPrimitiveComponent* OtherComp,
        int32                OtherBodyIndex,
        bool                 bFromSweep,
        const FHitResult&    SweepResult);

    void CreateTriggerVolume();
};
