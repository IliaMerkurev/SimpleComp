#pragma once

#include "Components/ActorComponent.h"
#include "Core/SCTypes.h"
#include "CoreMinimal.h"
#include "SCFollowConstraintComponent.generated.h"


/**
 * USCFollowConstraintComponent: Constrains the owner actor to stay within a
 * specified distance of a target actor. It also provides smooth rotation toward
 * the direction of movement.
 *
 * Featuring per-axis control for both Location constraint and Rotation
 * behavior.
 */
UCLASS(ClassGroup = (SimpleComp), meta = (BlueprintSpawnableComponent))
class SIMPLECOMP_API USCFollowConstraintComponent : public UActorComponent {
  GENERATED_BODY()

public:
  USCFollowConstraintComponent();

  // --- Core Settings ---

  /** The actor to follow and maintain distance from. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|CORE")
  TObjectPtr<AActor> FollowTarget;

  /** Maximum allowed distance from the FollowTarget before the owner starts
   * following. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|CORE",
            meta = (ClampMin = "0.0", Units = "cm"))
  float RopeLength = 500.0f;

  /** How smoothly the actor rotates toward its movement direction (0 = instant,
   * higher = smoother). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|CORE",
            meta = (ClampMin = "0.0"))
  float RotationSmoothness = 8.0f;

  // --- Axis Control (Location) ---

  /** Settings for X axis movement. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "SimpleComp|Axis Control (Location)")
  FSCAxisSettings XAxisSettings;

  /** Settings for Y axis movement. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "SimpleComp|Axis Control (Location)")
  FSCAxisSettings YAxisSettings;

  /** Settings for Z axis movement. Locking this prevents vertical following. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "SimpleComp|Axis Control (Location)")
  FSCAxisSettings ZAxisSettings;

  // --- Axis Control (Rotation) ---

  /** Settings for Pitch rotation. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "SimpleComp|Axis Control (Rotation)")
  FSCAxisSettings PitchSettings;

  /** Settings for Yaw rotation. Usually Free for trailers/carts. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "SimpleComp|Axis Control (Rotation)")
  FSCAxisSettings YawSettings;

  /** Settings for Roll rotation. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "SimpleComp|Axis Control (Rotation)")
  FSCAxisSettings RollSettings;

protected:
  virtual void BeginPlay() override;
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

private:
  /** Helper to process a single axis based on settings. */
  float ProcessAxis(float CurrentVal, float TargetVal,
                    const FSCAxisSettings &Settings);

  /** Tracks the location from the previous frame to calculate movement delta
   * for rotation. */
  FVector LastLocation;
};
