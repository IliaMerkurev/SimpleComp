#pragma once

#include "Components/SceneComponent.h"
#include "Core/SCTypes.h"
#include "CoreMinimal.h"
#include "SCRotationComponent.generated.h"

/**
 * SCRotationComponent: A professional-grade SceneComponent for handling complex
 * rotations. Features: Target tracking, Velocity alignment, Forward Delta
 * tracking, and Axis Constraints. Optimized for VR (Quest 3) using Quaternion
 * math to prevent Gimbal Lock.
 */
UCLASS(ClassGroup = (SimpleComp), meta = (BlueprintSpawnableComponent))
class SIMPLECOMP_API USCRotationComponent : public USceneComponent {
  GENERATED_BODY()

public:
  USCRotationComponent();

protected:
  virtual void BeginPlay() override;
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  // --- General Settings ---

  /** Master toggle. When false, the component smoothly returns to local zero
   * rotation. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|General",
            Interp)
  bool bLookAtTarget = true;

  /** Selection of the primary rotation calculation mode. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|General",
            Interp)
  ESCRotationMode RotationMode = ESCRotationMode::ToTarget;

  /** Standard interpolation speed. Used for steady tracking. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|General",
            Interp, meta = (ClampMin = "0.0"))
  float DefaultInterpSpeed = 15.0f;

  /** Faster interpolation speed used when switching targets or states. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|General",
            Interp, meta = (ClampMin = "0.0"))
  float SwitchInterpSpeed = 5.0f;

  /** Precision threshold (degrees) to stop using SwitchInterpSpeed. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|General",
            meta = (ClampMin = "0.0"))
  float SwitchThreshold = 1.0f;

  // --- Axis Control ---

  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "SimpleComp|Axis Control")
  FSCAxisSettings PitchSettings;

  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "SimpleComp|Axis Control")
  FSCAxisSettings YawSettings;

  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "SimpleComp|Axis Control")
  FSCAxisSettings RollSettings;

  // --- Mode Specific Settings ---

  /** The target actor to look at (ToTarget mode only). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Target",
            meta = (EditCondition = "RotationMode == ESCRotationMode::ToTarget",
                    EditConditionHides),
            Interp)
  TWeakObjectPtr<AActor> TargetActor;

  /** Offset added to TargetActor position or used as static world location if
   * actor is null. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Target",
            meta = (EditCondition = "RotationMode == ESCRotationMode::ToTarget",
                    EditConditionHides),
            Interp)
  FVector TargetLocationOffset;

  /** Degrees per second to rotate in Constant mode (Applied in Local Space). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Constant",
            meta = (EditCondition = "RotationMode == ESCRotationMode::Constant",
                    EditConditionHides, ForceUnits = "deg/s"),
            Interp)
  FRotator RotationRate = FRotator(0.f, 90.f, 0.f);

  /** Minimum movement distance (cm) to trigger rotation update in Forward Delta
   * mode. */
  UPROPERTY(
      EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Forward Delta",
      meta = (EditCondition = "RotationMode == ESCRotationMode::ToForwardDelta",
              EditConditionHides, ForceUnits = "cm"))
  float MinDistanceThreshold = 0.1f;

  /** Minimum velocity magnitude required to update rotation. Prevents jittering
   * when nearly stationary. */
  UPROPERTY(
      EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Velocity",
      meta = (EditCondition = "RotationMode == ESCRotationMode::ToVelocity",
              EditConditionHides),
      Interp)
  float VelocityThreshold = 10.0f;

private:
  /** Clamps or locks an axis based on user settings. */
  float ProcessAxis(float TargetAngle, const FSCAxisSettings &Settings);

  /** Returns world orientation toward a target or offset. */
  FQuat ComputeTargetQuat();

  /** Returns world orientation toward the owner's velocity. */
  FQuat ComputeVelocityQuat();

  /** Returns world orientation toward current movement direction. */
  FQuat ComputeForwardDeltaQuat();

  /** Manages the bIsSwitchingTarget state based on angular distance. */
  void UpdateTargetSwitching(const FQuat &CurrentQuat, const FQuat &TargetQuat);

  /** Location stored from previous frame for delta calculation. */
  FVector LastLocation;

  /** Tracking variable to detect when the target actor changes. */
  TWeakObjectPtr<AActor> LastTargetActor;

  /** True if we are in the process of rapid rotation to a new target. */
  bool bIsSwitchingTarget = false;

  /** Tracking variable for the bLookAtTarget toggle. */
  bool bLastLookAtTarget = true;
};