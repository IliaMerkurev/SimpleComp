#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Core/Interfaces/SCMessageInterface.h"
#include "SCSpawnerComponent.generated.h"

/** Defines the volume shape for spawning actors. */
UENUM(BlueprintType)
enum class ESCSpawnShape : uint8
{
    /** Use the standard Box Extent of the component. */
    Box      UMETA(DisplayName = "Box"),
    /** Use independent X, Y, Z radii to form an Ellipsoid or Disc. */
    Radius   UMETA(DisplayName = "Radius (Ellipsoid)")
};

/** Defines how spawned actors are initially rotated. */
UENUM(BlueprintType)
enum class ESCSpawnerRotationMode : uint8
{
    /** Actor's forward vector faces its initial velocity direction. */
    FaceVelocity UMETA(DisplayName = "Face Velocity"),
    /** Completely random rotation on all axes. */
    Random       UMETA(DisplayName = "Random"),
    /** Random rotation constrained within Min/Max rotator ranges. */
    Range        UMETA(DisplayName = "Range")
};

/**
 * SCSpawnerComponent: A high-performance spawning tool for Motion Design and Prototyping.
 * Supports box and ellipsoid volumes, flow control, and physical launching.
 */
UCLASS(ClassGroup = (SimpleComp), meta = (BlueprintSpawnableComponent, PrioritizeCategories = "!Test"))
class SIMPLECOMP_API USCSpawnerComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    USCSpawnerComponent();

    /** Starts the spawning process (handles both single burst and flow modes). */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "!Test", meta = (DisplayPriority = "0"))
    virtual void Spawn();
    
    /** Stops all active spawning cycles and repeat timers. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "!Test", meta = (DisplayPriority = "0"))
    virtual void StopSpawn();

protected:
    // --- Spawner Settings ---

    /** Select the volume shape: Box uses Component Extent, Radius uses custom Axis-based radii. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SC Spawner | Volume")
    ESCSpawnShape SpawnShape = ESCSpawnShape::Box;

    /** Independent radius for each axis. Set an axis to 0 to spawn in a flat Disc or Line. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SC Spawner | Volume", meta = (EditCondition = "SpawnShape == ESCSpawnShape::Radius", EditConditionHides))
    FVector SpawnRadius = FVector(100.f, 100.f, 100.f);

    /** The class of Actor to be spawned. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SC Spawner | Settings")
    TSubclassOf<AActor> SpawnClass = nullptr;

    /** Number of actors to spawn in a single execution or flow step. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Settings")
    int32 Count = 10;

    // --- Launch & Velocity ---

    /** If set, spawned actors will aim toward this actor instead of the Widget direction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Velocity")
    AActor* TargetActor = nullptr;

    /** Visual widget for setting the launch direction and base speed (length of the vector). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Velocity", meta = (MakeEditWidget = true))
    FVector LaunchDirectionWidget = FVector(100.f, 0.f, 0.f);

    /** Overall multiplier for the launch velocity. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Velocity")
    float LaunchMultiplier = 1.f;

    /** Maximum angle (in degrees) for random deviation from the base launch direction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Velocity")
    float LaunchSpreadAngle = 0.0f;

    /** Percentage of randomness added to the launch speed (0.0 to 1.0). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Velocity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float VelocityRandomness = 0.f;

    // --- Rotation Settings ---

    /** How the spawned actor should be oriented. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Rotation")
    ESCSpawnerRotationMode RotationMode = ESCSpawnerRotationMode::FaceVelocity;

    /** Minimum random rotation (Range mode only). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Rotation", meta = (EditCondition = "RotationMode == ESCSpawnerRotationMode::Range"))
    FRotator MinRotation = FRotator::ZeroRotator;

    /** Maximum random rotation (Range mode only). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Rotation", meta = (EditCondition = "RotationMode == ESCSpawnerRotationMode::Range"))
    FRotator MaxRotation = FRotator::ZeroRotator;

    // --- Messaging ---

    /** Data value to pass to the spawned actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Message")
    float MessageValue = 0.0f;

    /** Note string to pass to the spawned actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Message")
    FString MessageNote = TEXT("");

    // --- Flow Control ---

    /** If true, spawning happens over time rather than all at once. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Flow")
    bool bIsFlow = false;

    /** Total duration of the spawning cycle. If 0, it continues until manually stopped. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Flow", meta = (ToolTip = "Duration of one active spawn cycle. If 0, runs indefinitely."))
    float FlowTimer = 0.f;

    /** Delay between individual spawn bursts within a cycle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Flow", meta = (ToolTip = "Delay between individual spawns during active cycle."))
    float FlowInterval = 1.f;

    /** After a flow cycle ends, wait this long before automatically restarting it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SC Spawner | Flow")
    float AutoRepeatInterval = 0.f;

    /** Visualize launch direction vectors in the viewport. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SC Spawner | Debug")
    bool bShowDebugLines = false;

private:
    /** The core spawning logic. Calculates positions and applies physics. */
    virtual void ExecuteSpawning();
    
    /** Starts a new spawning cycle. */
    void StartActivePhase();

    /** Handles the end of a Flow cycle and triggers Repeat logic. */
    void OnFlowDurationExpired();
    
    bool bIsManuallyStopped = false;

    FTimerHandle FlowTimerHandle;
    FTimerHandle FlowDurationHandle;
    FTimerHandle RepeatDelayHandle;
};