#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Containers/Queue.h"
#include "SCStackComponent.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;

// ---------------------------------------------------------------------------
// Slot Status Enum
// ---------------------------------------------------------------------------

/** Represents the occupancy state of a single stack slot. */
UENUM(BlueprintType)
enum class ESCSlotStatus : uint8
{
    /** Slot is available for reservation. */
    Free     UMETA(DisplayName = "Free"),
    /** A resource is in flight toward this slot. */
    Reserved UMETA(DisplayName = "Reserved"),
    /** A resource has arrived and the slot displays a HISM instance. */
    Filled   UMETA(DisplayName = "Filled")
};

USTRUCT(BlueprintType)
struct FSCSlotData
{
    GENERATED_BODY()

    UPROPERTY()
    ESCSlotStatus Status = ESCSlotStatus::Free;

    UPROPERTY()
    int32 TicketID = INDEX_NONE;
};

// ---------------------------------------------------------------------------
// Internal animation state (non-reflected, implementation detail)
// ---------------------------------------------------------------------------

struct FSCSlotAnimState
{
    float        Progress    = 0.0f;
    double LastUpdateTime = 0.0;
};

// ---------------------------------------------------------------------------
// Curve Mode Enum
// ---------------------------------------------------------------------------

/** Defines how the stack deforms along the Z axis. */
UENUM(BlueprintType)
enum class ESCStackCurveMode : uint8
{
    /** The stack builds perfectly straight upward. */
    None        UMETA(DisplayName = "None"),
    /** The stack automatically bends based on the actor's acceleration (Spring-Mass physics). */
    Inertia     UMETA(DisplayName = "Inertia"),
    /** The stack bends along custom component references (e.g. Scene Components). */
    ManualCurve UMETA(DisplayName = "Manual Curve")
};

// ---------------------------------------------------------------------------
// Stack Component
// ---------------------------------------------------------------------------

/**
 * USCStackComponent
 * Manages a grid-based stack of objects visualized via a Hierarchical Instanced Static Mesh (HISM).
 * Tracks slot reservation status, drives scale-in animations via FTimerManager,
 * and supports a destruction burst (Explode).
 */
UENUM(BlueprintType)
enum class ESCStackExtractionOrder : uint8
{
    /** Takes the absolute last filled slot (Reverse sequential). */
    FromLastAdded,

    /** Finds the highest layer that has items, and picks the FIRST filled slot on that layer. (Empties the layer in the same direction it was filled). */
    FromFirstOnTopLayer,

    /** Finds the highest layer that has items, and picks a random filled slot from it. */
    RandomOnTopLayer,

    /** Picks a completely random filled slot from the entire stack. */
    RandomFromAll
};

// ---------------------------------------------------------------------------
// Spawn Settings Struct
// ---------------------------------------------------------------------------

/** Configuration controlling how filled slots in the stack are converted and spawned as actors. */
USTRUCT(BlueprintType)
struct SIMPLECOMP_API FSCStackSpawnSettings
{
    GENERATED_BODY()

    /** Delay between spawn waves (in seconds). If <= 0, all slots spawn immediately in a single frame. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Spawning")
    float SpawnInterval = 0.05f;

    /** When true, wave priority starts from the top layer downwards. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Spawning")
    bool bFromTop = false;

    /** When true, wave priority starts from the bottom layer upwards. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Spawning")
    bool bFromBottom = false;

    /** When true, wave priority starts from the outer perimeter (edges) inwards to the center. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Spawning")
    bool bFromEdges = false;

    /** When true, wave priority starts from the center outwards to the perimeter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Spawning")
    bool bFromCenter = false;

    /** Optional numerical value passed in FSCMessagePayload to each spawned actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Spawning")
    float MessageValue = 0.0f;

    /** Optional string note passed in FSCMessagePayload to each spawned actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Spawning")
    FString MessageNote = TEXT("");
};

UCLASS(ClassGroup = (SimpleComp), meta = (BlueprintSpawnableComponent, DisplayName = "Simple Stack Component"))
class SIMPLECOMP_API USCStackComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    USCStackComponent();

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /**
     * Finds the first Free slot, marks it as Reserved, and returns its ID.
     * Returns INDEX_NONE if no Free slots are available.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack")
    int32 RequestSlot();

    /** True while a reservation or filled resource still owns this ticket. */
    UFUNCTION(BlueprintPure, Category = "SimpleComp|Stack")
    bool HasTicket(int32 TicketID) const { return FindIndexByTicket(TicketID) != INDEX_NONE; }

    /**
     * Confirms that a resource has arrived at the given slot.
     * Marks the slot as Filled, makes the HISM instance visible,
     * and starts the scale-in animation.
     *
     * @param TicketID  The ID previously returned by RequestSlot().
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack")
    void ConfirmArrival(int32 TicketID);

    /**
     * Releases a Reserved slot back to Free.
     * Call this on the resource Actor's destruction during flight to prevent slot leaks.
     *
     * @param TicketID  The ID to release.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack")
    void ReleaseSlot(int32 TicketID);

    /**
     * Converts all filled stack slots into spawned actors using default component settings.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack")
    void SpawnActors();

    /**
     * Converts all filled stack slots into spawned actors using custom directional and interval settings.
     *
     * @param Settings Configuration controlling wave directions, stagger interval, and message data.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack")
    void SpawnActorsWithSettings(const FSCStackSpawnSettings& Settings);

    /**
     * Cancels any active staggered actor spawning cycle.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack")
    void CancelSpawning();

    /**
     * Legacy function. Use SpawnActors instead.
     * Spawns SpawnActorClass at each Filled slot's position.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack", meta = (DeprecatedFunction, DeprecationMessage = "Use SpawnActors instead."))
    void Explode();

    /**
     * Returns the world-space transform of the given slot. Useful for the resource
     * Actor to know its destination before confirming arrival.
     *
     * @param TicketID  The target slot ID.
     */
    UFUNCTION(BlueprintPure, Category = "SimpleComp|Stack")
    FTransform GetSlotWorldTransform(int32 TicketID) const;

    /** Returns the current number of slots in the Filled state. */
    UFUNCTION(BlueprintPure, Category = "SimpleComp|Stack")
    int32 GetFilledSlotCount() const;

    /** Returns the total grid capacity (Rows * Columns * Layers). */
    UFUNCTION(BlueprintPure, Category = "SimpleComp|Stack")
    int32 GetTotalCapacity() const;


    /**
     * Fills the stack to the target level [0..1] with staggered scale animation.
     * Values are clamped. Retargets pending fill while preserving existing animation progress and external reservations.
     * Use for cinematics or scenarios where resources don't fly in individually.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack")
    void SetFillLevel(float InFillLevel);

    /** Applies edited settings immediately. Raw runtime property writes are also synchronized periodically. */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack")
    void RefreshSettings();

    /**
     * Fired when a slot animation completes and an element is fully placed in the stack.
     * Override in Blueprint to trigger VFX, SFX, or game logic.
     *
     * @param TicketID  The slot that finished animating.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "SimpleComp|Stack")
    void OnSlotFilled(int32 TicketID);

    /**
     * Fired when an actor is spawned from a stack slot.
     *
     * @param SpawnedActor The newly spawned actor.
     * @param SlotID The index of the slot from which the actor was spawned.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "SimpleComp|Stack")
    void OnActorSpawned(AActor* SpawnedActor, int32 SlotID);

    /**
     * Fired when the scheduled spawn waves finish. Failed spawns remain filled and can be retried.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "SimpleComp|Stack")
    void OnSpawningCompleted();

    /**
     * Finds a filled slot based on the chosen strategy, marks it as Free, hides it in the stack,
     * and returns its precise world transform so you can spawn a flying resource there.
     * 
     * @param Order Strategy for picking which slot to extract.
     * @param OutTicketID The ID of the slot that was freed.
     * @param OutTransform The world transform of the slot before it was hidden.
     * @return True if a slot was found and extracted. False if the stack was completely empty.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack")
    bool ExtractSlot(ESCStackExtractionOrder Order, int32& OutTicketID, FTransform& OutTransform);

    /**
     * Extracts a specific slot (if it is filled), marks it as Free, and hides it.
     * 
     * @param TicketID The ID of the slot to extract.
     * @param OutTransform The world transform of the slot before it was hidden.
     * @return True if the slot was filled and successfully extracted. False otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleComp|Stack")
    bool ExtractSpecificSlot(int32 TicketID, FTransform& OutTransform);

    // -----------------------------------------------------------------------
    // Configuration — Preview
    // -----------------------------------------------------------------------

    /** If true, displays all grid instances at target scale in the editor viewport for preview and layout adjustments. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Preview")
    bool bShowPreview = true;

    // -----------------------------------------------------------------------
    // Configuration — Mesh
    // -----------------------------------------------------------------------

    /** Static mesh rendered for each stack element. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Mesh")
    TObjectPtr<UStaticMesh> ElementMesh;

    // -----------------------------------------------------------------------
    // Configuration — Grid
    // -----------------------------------------------------------------------

    /** Number of rows in the stack grid (Y axis). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Grid",
        meta = (ClampMin = "1", UIMin = "1"))
    int32 Rows = 3;

    /** Number of columns in the stack grid (X axis). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Grid",
        meta = (ClampMin = "1", UIMin = "1"))
    int32 Columns = 3;

    /** Number of layers stacked along the Z axis. Layer 0 is the bottom; fills upward. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Grid",
        meta = (ClampMin = "1", UIMin = "1"))
    int32 Layers = 1;

    /**
     * If true, padding between elements is automatically calculated from the ElementMesh bounds
     * multiplied by TargetElementScale. Manual Padding is ignored.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Grid")
    bool bAutoCalculatePadding = true;

    /** Whether instances in the stack should have collision enabled. Disable when collision is unnecessary, especially for deforming stacks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Grid")
    bool bEnableCollision = false;

    /**
     * Manual spacing between slot centers (cm). Active only when bAutoCalculatePadding is false.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Grid",
        meta = (EditCondition = "!bAutoCalculatePadding"))
    FVector ManualPadding = FVector(100.0f, 100.0f, 100.0f);

    /** Target local scale applied to each element when fully placed in the stack. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Grid")
    FVector TargetElementScale = FVector(1.0f);

    // -----------------------------------------------------------------------
    // Configuration — Curve & Deformation
    // -----------------------------------------------------------------------

    /** Determines how the stack deforms (bends) as it grows higher. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Curve")
    ESCStackCurveMode CurveMode = ESCStackCurveMode::None;

    /**
     * Components defining the curve for ManualCurve mode.
     * Add Scene Components to your actor, and assign them here.
     * The stack base is automatically point 0. Animate these components in Sequencer!
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Curve", meta = (EditCondition = "CurveMode == ESCStackCurveMode::ManualCurve", EditConditionHides))
    TArray<FComponentReference> ControlPointComponents;

    /** 
     * How much the individual elements rotate to match the curve. 
     * 0.0 = Elements stay perfectly upright (staircase effect, zero clipping).
     * 1.0 = Elements fully rotate to match the curve (rigid spine effect).
     * Recommended: 0.1 - 0.4 for hyper-casual style stacks.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Curve", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TiltScale = 0.3f;

    // -----------------------------------------------------------------------
    // Configuration — Inertia Physics
    // -----------------------------------------------------------------------

    /** How stiff the spring is. Higher values make the stack snap back to straight faster. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Physics",
        meta = (EditCondition = "CurveMode == ESCStackCurveMode::Inertia", EditConditionHides, ClampMin = "10.0"))
    float InertiaStiffness = 50.0f;

    /** How much damping to apply. Higher values prevent the stack from wobbling back and forth. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Physics",
        meta = (EditCondition = "CurveMode == ESCStackCurveMode::Inertia", EditConditionHides, ClampMin = "0.0"))
    float InertiaDamping = 10.0f;

    /** Maximum allowed horizontal lag (in cm) for the tip of the stack. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Physics",
        meta = (EditCondition = "CurveMode == ESCStackCurveMode::Inertia", EditConditionHides, ClampMin = "0.0"))
    float MaxTipLag = 150.0f;

    /** Max tilt angle for inertia mode in degrees. Clamps the curve rotation so it doesn't fold onto itself. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Physics", 
        meta = (EditCondition = "CurveMode == ESCStackCurveMode::Inertia", EditConditionHides, ClampMin = "0.0", ClampMax = "90.0"))
    float InertiaMaxTiltDegrees = 45.0f;

    // -----------------------------------------------------------------------
    // Configuration — Randomization
    // -----------------------------------------------------------------------
    
    /** Seed for deterministic randomness. Change to get a different pattern. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Randomization")
    int32 RandomSeed = 42;
    
    /** Enable random rotation per element. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Randomization")
    bool bEnableRandomRotation = false;
    
    /** Minimum rotation offset (applied before deformation). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Randomization", meta = (EditCondition = "bEnableRandomRotation", EditConditionHides))
    FRotator RandomRotationMin = FRotator::ZeroRotator;
    
    /** Maximum rotation offset (applied before deformation). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Randomization", meta = (EditCondition = "bEnableRandomRotation", EditConditionHides))
    FRotator RandomRotationMax = FRotator(0.f, 360.f, 0.f);

    /** Enable random scale per element. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Randomization")
    bool bEnableRandomScale = false;
    
    /** Keep scale uniform (X,Y,Z scaled equally). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Randomization", meta = (EditCondition = "bEnableRandomScale", EditConditionHides))
    bool bUniformRandomScale = true;

    /** Minimum scale multiplier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Randomization", meta = (EditCondition = "bEnableRandomScale", EditConditionHides))
    FVector RandomScaleMin = FVector(0.8f);

    /** Maximum scale multiplier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Randomization", meta = (EditCondition = "bEnableRandomScale", EditConditionHides))
    FVector RandomScaleMax = FVector(1.2f);

    // -----------------------------------------------------------------------
    // Configuration — Actor Spawning
    // -----------------------------------------------------------------------

    /** Actor class spawned at each Filled instance's world position. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Spawning")
    TSubclassOf<AActor> SpawnActorClass;

    /** Default settings used when calling SpawnActors() without arguments. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Spawning")
    FSCStackSpawnSettings DefaultSpawnSettings;

    // -----------------------------------------------------------------------
    // Configuration — Animation
    // -----------------------------------------------------------------------

    /**
     * When true, Tick monitors FillLevel; a shared timer drives the resulting scale animations.
     * Required for Sequencer integration — each change to FillLevel triggers slot animations.
     * When false, fill monitoring does not require Tick; use SetFillLevel() for code-driven animation.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Animation")
    bool bEnableFillAnimation = false;

    /**
     * Current fill level [0..1]. When bEnableFillAnimation is true, changing this property
     * at runtime or via Sequencer animates slots in or out with scale animation.
     * For code-driven stagger animation, use SetFillLevel() instead.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Animation", Interp,
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FillLevel = 0.0f;

    /**
     * Total duration (seconds) of the scale-in animation for a newly placed element.
     * Zero or negative values complete on the next animation update.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Animation",
        meta = (ClampMin = "0.05", UIMin = "0.05"))
    float ScaleAnimationDuration = 0.35f;

    /**
     * Starting fill level [0..1] applied at BeginPlay via SetFillLevel() (stagger animation).
     * Only active when bEnableFillAnimation is false. When bEnableFillAnimation is true,
     * set FillLevel directly as the starting value.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Animation",
        meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0",
                EditCondition = "!bEnableFillAnimation"))
    float InitialFillLevel = 0.0f;

    /**
     * Time (seconds) between successive slot animations when filling via SetFillLevel().
     * Set to 0 to fill all slots simultaneously.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Stack|Animation",
        meta = (ClampMin = "0.0", UIMin = "0.0"))
    float FillStaggerDelay = 0.05f;

    // -----------------------------------------------------------------------
    // Runtime — HISM (read-only access for Blueprint queries)
    // -----------------------------------------------------------------------

    /** The Hierarchical Instanced Static Mesh component rendering the stack. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SimpleComp|Stack")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> StackHISM;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void OnComponentCreated() override;
#endif

private:
    void InitializeRuntimeState();
    void UpdateEditorPreview();

    FTransform CalculateSlotGridTransform(int32 SlotID) const;
    FTransform CalculateDeformedTransform(const FTransform& GridTransform) const;

    void UpdateInertiaSimulation(float DeltaTime);
    void RefreshStackTransforms();
    void BuildCachedCurve();

    int32 FindIndexByTicket(int32 TicketID) const;

    void StartSlotAnimation(int32 SlotIndex);
    void TickSlotAnimations();
    void FreeSlot(int32 SlotIndex);
    void RebuildTicketLookup();
    uint32 CalculateSettingsHash() const;
    void ClearAllAnimations();

    void ProcessNextPendingSlot();
    void CancelPendingFill();

    int32 CalculateSlotWaveIndex(int32 SlotID, const FSCStackSpawnSettings& Settings) const;
    void ProcessNextSpawnWave();

    TArray<TArray<int32>> PendingSpawnWaves;
    FTimerHandle SpawnWaveTimerHandle;
    FSCStackSpawnSettings ActiveSpawnSettings;

    TArray<FSCSlotData> SlotStatuses;
    // Every asynchronous operation uses a ticket, never a physical instance index.
    TMap<int32, int32> TicketToIndex;
    TMap<int32, FSCSlotAnimState> ActiveAnimations;
    TSet<int32> ConvertingTickets;
    FTimerHandle AnimationTimerHandle;
    FTimerHandle SettingsTimerHandle;
    uint64 SpawnRevision = 0;
    uint64 FillRevision = 0;
    uint32 AppliedSettingsHash = 0;
    FIntVector AppliedDimensions = FIntVector(1, 1, 1);
    bool bRuntimeInitialized = false;
    bool bShuttingDown = false;
    bool bSynchronizingSettings = false;
    ESCStackCurveMode AppliedCurveMode = ESCStackCurveMode::None;

    TQueue<int32> PendingFillTickets;
    FTimerHandle FillStaggerTimerHandle;

    int32 NextTicketID = 0;
    float LastAppliedFillLevel = 0.0f;

    // Physics & Curve Simulation State
    FVector CurrentTipLag = FVector::ZeroVector;
    FVector CurrentTipVelocity = FVector::ZeroVector;
    FVector PreviousOwnerLocation = FVector::ZeroVector;
    FVector PreviousOwnerVelocity = FVector::ZeroVector;
    bool bNeedsTransformUpdate = false;
    
    FInterpCurveVector CachedCurve;
    TArray<TPair<float, FQuat>> CachedRotations;

    static constexpr float AnimationTickInterval = 0.016f;
};
