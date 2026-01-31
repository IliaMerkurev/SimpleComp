#include "Components/Movement/SCFollowConstraintComponent.h"
#include "GameFramework/Actor.h"

USCFollowConstraintComponent::USCFollowConstraintComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void USCFollowConstraintComponent::BeginPlay() {
  Super::BeginPlay();

  if (AActor *Owner = GetOwner()) {
    LastLocation = Owner->GetActorLocation();
  }
}

void USCFollowConstraintComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  AActor *Owner = GetOwner();
  if (!FollowTarget || !Owner) {
    return;
  }

  const FVector TargetLoc = FollowTarget->GetActorLocation();
  const FVector CurrentLoc = Owner->GetActorLocation();

  // Project positions to XY plane to maintain vertical height if needed
  FVector TargetXY = FVector(TargetLoc.X, TargetLoc.Y, 0.0f);
  FVector CurrentXY = FVector(CurrentLoc.X, CurrentLoc.Y, 0.0f);

  FVector Direction = CurrentXY - TargetXY;
  const float CurrentDistance = Direction.Size();

  if (CurrentDistance > RopeLength) {
    // 1. Calculate and set the new position based on RopeLength constraint
    FVector NewXY = TargetXY + (Direction.GetSafeNormal() * RopeLength);
    FVector NewLocation = FVector(NewXY.X, NewXY.Y, CurrentLoc.Z);
    Owner->SetActorLocation(NewLocation);

    // 2. Calculate rotation based on movement delta
    FVector MoveDelta = NewLocation - LastLocation;

    if (MoveDelta.SizeSquared() > KINDA_SMALL_NUMBER) {
      // Create a target Quaternion from the movement direction
      FQuat TargetQuat = MoveDelta.ToOrientationQuat();

      // Force the Quaternion to only represent Yaw rotation (Zero out Pitch and
      // Roll)
      FRotator TargetRotator = TargetQuat.Rotator();
      TargetQuat = FRotator(0.0f, TargetRotator.Yaw, 0.0f).Quaternion();

      FQuat CurrentQuat = Owner->GetActorQuat();

      // Slerp (Spherical Linear Interpolation) for perfectly smooth rotation
      // transition
      float LerpAlpha =
          FMath::Clamp(DeltaTime * RotationSmoothness, 0.0f, 1.0f);
      FQuat NewQuat = FQuat::Slerp(CurrentQuat, TargetQuat, LerpAlpha);

      Owner->SetActorRotation(NewQuat);
    }
  }

  LastLocation = Owner->GetActorLocation();
}
