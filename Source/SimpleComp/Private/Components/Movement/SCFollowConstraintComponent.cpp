#include "Components/Movement/SCFollowConstraintComponent.h"
#include "GameFramework/Actor.h"

USCFollowConstraintComponent::USCFollowConstraintComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

  // Default to XY behavior (Z Locked)
  ZAxisSettings.Mode = ESCAxisMode::Locked;

  // Default to Yaw-only rotation
  PitchSettings.Mode = ESCAxisMode::Locked;
  RollSettings.Mode = ESCAxisMode::Locked;
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

  // 1. Calculate Target XY/Z based on constraints
  // We only care about direction if the distance is exceeded
  FVector Direction = CurrentLoc - TargetLoc;

  // Apply per-axis locking/limiting to the relative direction vector
  FVector ClampedDir;
  ClampedDir.X =
      (XAxisSettings.Mode == ESCAxisMode::Locked) ? 0.0f : Direction.X;
  ClampedDir.Y =
      (YAxisSettings.Mode == ESCAxisMode::Locked) ? 0.0f : Direction.Y;
  ClampedDir.Z =
      (ZAxisSettings.Mode == ESCAxisMode::Locked) ? 0.0f : Direction.Z;

  const float CurrentDistance = ClampedDir.Size();

  if (CurrentDistance > RopeLength) {
    // 1. Calculate and set the new position based on RopeLength constraint
    FVector NewRelativeLoc = ClampedDir.GetSafeNormal() * RopeLength;

    FVector NewLocation;
    NewLocation.X = (XAxisSettings.Mode == ESCAxisMode::Locked)
                        ? CurrentLoc.X
                        : TargetLoc.X + NewRelativeLoc.X;
    NewLocation.Y = (YAxisSettings.Mode == ESCAxisMode::Locked)
                        ? CurrentLoc.Y
                        : TargetLoc.Y + NewRelativeLoc.Y;
    NewLocation.Z = (ZAxisSettings.Mode == ESCAxisMode::Locked)
                        ? CurrentLoc.Z
                        : TargetLoc.Z + NewRelativeLoc.Z;

    Owner->SetActorLocation(NewLocation);

    // 2. Calculate rotation based on movement delta
    FVector MoveDelta = NewLocation - LastLocation;

    if (MoveDelta.SizeSquared() > KINDA_SMALL_NUMBER) {
      // Create a target Quaternion from the movement direction
      FQuat TargetQuat = MoveDelta.ToOrientationQuat();
      FRotator TargetRotator = TargetQuat.Rotator();

      // Apply rotation constraints
      FRotator FinalRotator;
      FinalRotator.Pitch = (PitchSettings.Mode == ESCAxisMode::Locked)
                               ? Owner->GetActorRotation().Pitch
                               : TargetRotator.Pitch;
      FinalRotator.Yaw = (YawSettings.Mode == ESCAxisMode::Locked)
                             ? Owner->GetActorRotation().Yaw
                             : TargetRotator.Yaw;
      FinalRotator.Roll = (RollSettings.Mode == ESCAxisMode::Locked)
                              ? Owner->GetActorRotation().Roll
                              : TargetRotator.Roll;

      // Handle 'Limited' mode if needed (could be expanded)
      if (PitchSettings.Mode == ESCAxisMode::Limited)
        FinalRotator.Pitch = FMath::Clamp(FinalRotator.Pitch, PitchSettings.Min,
                                          PitchSettings.Max);
      if (YawSettings.Mode == ESCAxisMode::Limited)
        FinalRotator.Yaw =
            FMath::Clamp(FinalRotator.Yaw, YawSettings.Min, YawSettings.Max);
      if (RollSettings.Mode == ESCAxisMode::Limited)
        FinalRotator.Roll =
            FMath::Clamp(FinalRotator.Roll, RollSettings.Min, RollSettings.Max);

      FQuat FinalQuat = FinalRotator.Quaternion();
      FQuat CurrentQuat = Owner->GetActorQuat();

      // Slerp for smooth rotation
      float LerpAlpha =
          FMath::Clamp(DeltaTime * RotationSmoothness, 0.0f, 1.0f);
      FQuat NewQuat = FQuat::Slerp(CurrentQuat, FinalQuat, LerpAlpha);

      Owner->SetActorRotation(NewQuat);
    }
  }

  LastLocation = Owner->GetActorLocation();
}
