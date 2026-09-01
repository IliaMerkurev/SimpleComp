#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "SCTypes.generated.h"

/**
 * Defines constraint behavior for a specific axis (Location or Rotation).
 */
UENUM(BlueprintType)
    enum class ESCAxisMode : uint8
    {
        /** Component logic fully controls this axis. */
        Free UMETA(DisplayName = "Free"),
        /** Axis is clamped between Min and Max values. */
        Limited UMETA(DisplayName = "Limited"),
        /** Axis is locked to its initial or zero value. */
        Locked UMETA(DisplayName = "Locked")
    };

    /**
     * Defines the logic used to calculate the target rotation vector for rotation
     * components.
     */
    UENUM(BlueprintType)
        enum class ESCRotationMode : uint8
        {
            /** Traditional look-at logic targeting an Actor or Location. */
            ToTarget UMETA(DisplayName = "Rotate to Target"),
            /** Orients the component toward the Owner's current Velocity vector. */
            ToVelocity UMETA(DisplayName = "Rotate to Velocity"),
            /** Orients toward the movement direction (CurrentPos - LastPos). Ideal for
               Lerp/Spline movement. */
            ToForwardDelta UMETA(DisplayName = "Rotate to Forward Delta"),
            /** Continuous local rotation at a fixed rate (e.g., for propellers or idle
               spin). */
            Constant UMETA(DisplayName = "Constant Rotation")
        };

        /**
         * Defines what transform property a curve track should drive.
         */
        UENUM(BlueprintType)
            enum class ESCCurveTrackType : uint8
            {
                LocationX UMETA(DisplayName = "Location X"),
                LocationY UMETA(DisplayName = "Location Y"),
                LocationZ UMETA(DisplayName = "Location Z"),
                RotationP UMETA(DisplayName = "Rotation Pitch"),
                RotationY UMETA(DisplayName = "Rotation Yaw"),
                RotationR UMETA(DisplayName = "Rotation Roll"),
                ScaleX UMETA(DisplayName = "Scale X"),
                ScaleY UMETA(DisplayName = "Scale Y"),
                ScaleZ UMETA(DisplayName = "Scale Z"),
                CustomFloat UMETA(DisplayName = "Custom Float"),
                /** Uses a CurveTable asset. Rows must be named X, Y, Z. */
                TableLocation UMETA(DisplayName = "Table Location"),
                /** Uses a CurveTable asset. Rows must be named X (Pitch), Y (Yaw), Z (Roll). */
                TableRotation UMETA(DisplayName = "Table Rotation"),
                /** Uses a CurveTable asset. Rows must be named X, Y, Z. */
                TableScale UMETA(DisplayName = "Table Scale"),
            };

            /**
             * Defines the space in which transformations are applied.
             */
            UENUM(BlueprintType)
                enum class ESCTransformSpace : uint8
                {
                    Local UMETA(DisplayName = "Local Space"),
                    World UMETA(DisplayName = "World Space")
                };

                /**
                 * Universal settings for individual axis constraints and limits.
                 * Used for both Location and Rotation.
                 */
                USTRUCT(BlueprintType)
                    struct FSCAxisSettings
                    {
                        GENERATED_BODY()

                        /** How this axis should behave. */
                        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Settings")
                        ESCAxisMode Mode = ESCAxisMode::Free;

                        /** Minimum allowed value (angle for rotation, cm for location). Used only in
                         * Limited mode. */
                        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Settings",
                            meta = (EditCondition = "Mode == ESCAxisMode::Limited"))
                        float Min = -90.0f;

                        /** Maximum allowed value (angle for rotation, cm for location). Used only in
                         * Limited mode. */
                        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Settings",
                            meta = (EditCondition = "Mode == ESCAxisMode::Limited"))
                        float Max = 90.0f;
                    };

    /** The geometric shape of the collection trigger volume. */
    UENUM(BlueprintType)
    enum class ESCCollectorShape : uint8
    {
        /** Spherical trigger volume. */
        Sphere UMETA(DisplayName = "Sphere"),
        /** Box trigger volume. */
        Box    UMETA(DisplayName = "Box")
    };

    /** Defines the volume shape for spawning actors. */
    UENUM(BlueprintType)
    enum class ESCSpawnShape : uint8
    {
        /** Use the standard Box Extent of the component. */
        Box UMETA(DisplayName = "Box"),
        /** Use independent X, Y, Z radii to form an Ellipsoid or Disc. */
        Radius UMETA(DisplayName = "Radius (Ellipsoid)")
    };

    /** Defines how spawned actors are initially rotated. */
    UENUM(BlueprintType)
    enum class ESCSpawnerRotationMode : uint8
    {
        /** Actor's forward vector faces its initial velocity direction. */
        FaceVelocity UMETA(DisplayName = "Face Velocity"),
        /** Completely random rotation on all axes. */
        Random UMETA(DisplayName = "Random"),
        /** Random rotation constrained within Min/Max rotator ranges. */
        Range UMETA(DisplayName = "Range")
    };

    /** Defines a class to spawn along with its relative weight (probability). */
    USTRUCT(BlueprintType)
    struct FSCWeightedSpawnClass
    {
        GENERATED_BODY()

        /** The class of Actor to be spawned. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Spawner|Settings")
        TSubclassOf<class AActor> ActorClass = nullptr;

        /** The relative weight of this class. Higher weight increases the chance of being selected. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Spawner|Settings", meta = (ClampMin = "0.0", UIMin = "0.0"))
        float Weight = 1.0f;
    };
