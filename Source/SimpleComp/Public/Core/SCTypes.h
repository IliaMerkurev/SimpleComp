#pragma once

#include "CoreMinimal.h"
#include "SCTypes.generated.h"

/**
 * Defines constraint behavior for a specific axis (Location or Rotation).
 */
UENUM(BlueprintType)
enum class ESCAxisMode : uint8 {
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
enum class ESCRotationMode : uint8 {
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
enum class ESCCurveTrackType : uint8 {
  LocationX,
  LocationY,
  LocationZ,
  RotationP,
  RotationY,
  RotationR,
  ScaleX,
  ScaleY,
  ScaleZ,
  VectorLocation,
  VectorRotation,
  VectorScale,
  CustomFloat
};

/**
 * Defines the space in which transformations are applied.
 */
UENUM(BlueprintType)
enum class ESCTransformSpace : uint8 {
  Local UMETA(DisplayName = "Local Space"),
  World UMETA(DisplayName = "World Space")
};

/**
 * Universal settings for individual axis constraints and limits.
 * Used for both Location and Rotation.
 */
USTRUCT(BlueprintType)
struct FSCAxisSettings {
  GENERATED_BODY()

  /** How this axis should behave. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  ESCAxisMode Mode = ESCAxisMode::Free;

  /** Minimum allowed value (angle for rotation, cm for location). Used only in
   * Limited mode. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
            meta = (EditCondition = "Mode == ESCAxisMode::Limited"))
  float Min = -90.0f;

  /** Maximum allowed value (angle for rotation, cm for location). Used only in
   * Limited mode. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
            meta = (EditCondition = "Mode == ESCAxisMode::Limited"))
  float Max = 90.0f;
};
