#pragma once

#include "Components/BoxComponent.h"
#include "Core/Interfaces/SCMessageInterface.h"
#include "Core/SCTypes.h"
#include "CoreMinimal.h"
#include "SCSpawnerComponent.generated.h"



        /**
         * SCSpawnerComponent: A high-performance spawning tool for Motion Design and
         * Prototyping. Supports box and ellipsoid volumes, flow control, and physical
         * launching.
         */
        UCLASS(ClassGroup = (SimpleComp),
            meta =
                (BlueprintSpawnableComponent, PrioritizeCategories = "!Test", DisplayName = "Simple Spawner Component"))
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

                /** Select the volume shape: Box uses Component Extent, Radius uses custom
                 * Axis-based radii. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Volume")
                ESCSpawnShape SpawnShape = ESCSpawnShape::Box;

                /** Independent radius for each axis. Set an axis to 0 to spawn in a flat Disc
                 * or Line. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Volume",
                    meta = (EditCondition = "SpawnShape == ESCSpawnShape::Radius", EditConditionHides))
                FVector SpawnRadius = FVector(100.f, 100.f, 100.f);

                /** List of classes to spawn with their respective weights. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Spawner|Settings")
                TArray<FSCWeightedSpawnClass> SpawnClass;

                /** Number of actors to spawn in a single execution or flow step. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Settings")
                int32 Count = 10;

                // --- Launch & Velocity ---

                /** If set, spawned actors will aim toward this actor instead of the Widget
                 * direction. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Velocity")
                AActor* TargetActor = nullptr;

                /** Visual widget for setting the launch direction and base speed (length of
                 * the vector). */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Velocity",
                    meta = (MakeEditWidget = true))
                FVector LaunchDirectionWidget = FVector(100.f, 0.f, 0.f);

                /** Overall multiplier for the launch velocity. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Velocity")
                float LaunchMultiplier = 1.f;

                /** Maximum angle (in degrees) for random deviation from the base launch
                 * direction. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Velocity")
                float LaunchSpreadAngle = 0.0f;

                /** Percentage of randomness added to the launch speed (0.0 to 1.0). */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Velocity",
                    meta = (ClampMin = "0.0", ClampMax = "1.0"))
                float VelocityRandomness = 0.f;

                // --- Rotation Settings ---

                /** How the spawned actor should be oriented. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Rotation")
                ESCSpawnerRotationMode RotationMode = ESCSpawnerRotationMode::FaceVelocity;

                /** Minimum random rotation (Range mode only). */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Rotation",
                    meta = (EditCondition = "RotationMode == ESCSpawnerRotationMode::Range"))
                FRotator MinRotation = FRotator::ZeroRotator;

                /** Maximum random rotation (Range mode only). */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Rotation",
                    meta = (EditCondition = "RotationMode == ESCSpawnerRotationMode::Range"))
                FRotator MaxRotation = FRotator::ZeroRotator;

                // --- Messaging ---

                /** Data value to pass to the spawned actor. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Message")
                float MessageValue = 0.0f;

                /** Note string to pass to the spawned actor. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Message")
                FString MessageNote = TEXT("");

                // --- Flow Control ---

                /** If true, spawning happens over time rather than all at once. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Flow")
                bool bIsFlow = false;

                /** Total duration of the spawning cycle. If 0, it continues until manually
                 * stopped. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Flow",
                    meta = (ToolTip = "Duration of one active spawn cycle. If 0, runs indefinitely."))
                float FlowTimer = 0.f;

                /** Delay between individual spawn bursts within a cycle. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Flow",
                    meta = (ToolTip = "Delay between individual spawns during active cycle."))
                float FlowInterval = 1.f;

                /** After a flow cycle ends, wait this long before automatically restarting
                 * it. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Spawner|Flow")
                float AutoRepeatInterval = 0.f;

                /** Visualize launch direction vectors in the viewport. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Spawner|Debug")
                bool bShowDebugLines = false;

            private:
                /** The core spawning logic. Calculates positions and applies physics. */
                virtual void ExecuteSpawning();

                /** Starts a new spawning cycle. */
                void StartActivePhase();

                /** Handles the end of a Flow cycle and triggers Repeat logic. */
                void OnFlowDurationExpired();

                /** 
                 * Selects a random class based on weights. 
                 * Returns nullptr if the array is empty or contains no valid classes.
                 */
                TSubclassOf<AActor> GetRandomSpawnClass() const;

                bool bIsManuallyStopped = false;

                FTimerHandle FlowTimerHandle;
                FTimerHandle FlowDurationHandle;
                FTimerHandle RepeatDelayHandle;
            };