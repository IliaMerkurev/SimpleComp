#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SCFollowConstraintComponent.generated.h"


/**
 * USCFollowConstraintComponent: Constrains the owner actor to stay within a
 * specified distance of a target actor. It also provides smooth rotation toward
 * the direction of movement.
 *
 * Ideal for carts, trailers, or characters being pulled on a leash.
 */
UCLASS(ClassGroup = (SimpleComp), meta = (BlueprintSpawnableComponent))
class SIMPLECOMP_API USCFollowConstraintComponent : public UActorComponent {
  GENERATED_BODY()

public:
  USCFollowConstraintComponent();

  /** The actor to follow and maintain distance from. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Movement")
  TObjectPtr<AActor> FollowTarget;

  /** Maximum allowed distance from the FollowTarget before the owner starts
   * following. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Movement",
            meta = (ClampMin = "0.0", Units = "cm"))
  float RopeLength = 500.0f;

  /** How smoothly the actor rotates toward its movement direction (0 = instant,
   * higher = smoother). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Movement",
            meta = (ClampMin = "0.0"))
  float RotationSmoothness = 8.0f;

protected:
  virtual void BeginPlay() override;
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

private:
  /** Tracks the location from the previous frame to calculate movement delta
   * for rotation. */
  FVector LastLocation;
};
