#include "Components/Animation/SCCurveAnimComponent.h"
#include "Components/Animation/SCAnimSequence.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"

USCCurveAnimComponent::USCCurveAnimComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USCCurveAnimComponent::BeginPlay() {
  Super::BeginPlay();

  InitialLocation = GetRelativeLocation();
  InitialRotation = GetRelativeRotation();
  InitialScale = GetRelativeScale3D();

  if (bAutoPlay && AnimSequence) {
    Play();
  }
}

void USCCurveAnimComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (bIsPlaying && !bIsPaused) {
    UpdateAnimation(DeltaTime);
  }
}

void USCCurveAnimComponent::Play() { PlayEx(nullptr, 0.0f, false, false); }

void USCCurveAnimComponent::PlayEx(USCAnimSequence *Sequence, float Duration,
                                   bool bFromStart, bool bInReverse) {
  if (Sequence) {
    AnimSequence = Sequence;
  }

  if (!AnimSequence)
    return;

  if (Duration > 0.0f) {
    PlaybackDuration = Duration;
  }

  bIsPlaying = true;
  bIsPaused = false;
  bReverse = bInReverse;
  FiredNotifyIndices.Empty();

  if (bFromStart) {
    CurrentTime = bReverse ? GetEffectiveDuration() : 0.0f;
  }
}

void USCCurveAnimComponent::PlayFromStart() {
  PlayEx(nullptr, 0.0f, true, false);
}

void USCCurveAnimComponent::Stop() {
  bIsPlaying = false;
  bIsPaused = false;
  FiredNotifyIndices.Empty();
}

void USCCurveAnimComponent::Pause() { bIsPaused = true; }

void USCCurveAnimComponent::Resume() { bIsPaused = false; }

void USCCurveAnimComponent::Reverse() {
  if (!AnimSequence)
    return;
  bIsPlaying = true;
  bIsPaused = false;
  bReverse = true;
  CurrentTime = GetEffectiveDuration();
  FiredNotifyIndices.Empty();
}

void USCCurveAnimComponent::ReverseFromCurrent() { bReverse = !bReverse; }

void USCCurveAnimComponent::SetPlaybackPosition(float NewTime) {
  CurrentTime = FMath::Clamp(NewTime, 0.0f, GetEffectiveDuration());
  ApplyTransform();
}

float USCCurveAnimComponent::GetEffectiveDuration() const {
  if (PlaybackDuration > 0.0f)
    return PlaybackDuration;
  return AnimSequence ? AnimSequence->DefaultDuration : 1.0f;
}

void USCCurveAnimComponent::UpdateAnimation(float DeltaTime) {
  if (!AnimSequence)
    return;

  float Duration = GetEffectiveDuration();
  float OldTime = CurrentTime;

  float TimeStep = DeltaTime * PlayRate;
  if (bReverse) {
    CurrentTime -= TimeStep;
    if (CurrentTime <= 0.0f) {
      if (bLoop) {
        CurrentTime = Duration;
      } else {
        CurrentTime = 0.0f;
        bIsPlaying = false;
        OnAnimationFinished.Broadcast();
      }
    }
  } else {
    CurrentTime += TimeStep;
    if (CurrentTime >= Duration) {
      if (bLoop) {
        CurrentTime = 0.0f;
      } else {
        CurrentTime = Duration;
        bIsPlaying = false;
        OnAnimationFinished.Broadcast();
      }
    }
  }

  ApplyTransform();

  OnAnimationUpdate.Broadcast(CurrentTime,
                              Duration > 0 ? CurrentTime / Duration : 0.0f);

  ProcessNotifies(OldTime, CurrentTime);
}

void USCCurveAnimComponent::ApplyTransform() {
  if (!AnimSequence)
    return;

  float Duration = GetEffectiveDuration();
  float AssetDuration =
      AnimSequence->DefaultDuration > 0 ? AnimSequence->DefaultDuration : 1.0f;

  // Normalize time to asset time
  float EvalTime = (CurrentTime / Duration) * AssetDuration;

  FVector NewLocation = InitialLocation;
  FRotator NewRotation = InitialRotation;
  FVector NewScale = InitialScale;

  for (const FSCCurveTrack &Track : AnimSequence->CurveTracks) {
    if (!Track.CurveAsset)
      continue;

    float Value = 0.0f;
    FVector VectorValue = FVector::ZeroVector;

    UCurveFloat *FloatCurve = Cast<UCurveFloat>(Track.CurveAsset);
    UCurveVector *VectorCurve = Cast<UCurveVector>(Track.CurveAsset);

    if (FloatCurve) {
      Value = FloatCurve->GetFloatValue(EvalTime);
    } else if (VectorCurve) {
      VectorValue = VectorCurve->GetVectorValue(EvalTime);
    }

    switch (Track.TrackType) {
    case ESCCurveTrackType::LocationX:
      NewLocation.X = Track.bAddBaseValue ? InitialLocation.X + Value : Value;
      break;
    case ESCCurveTrackType::LocationY:
      NewLocation.Y = Track.bAddBaseValue ? InitialLocation.Y + Value : Value;
      break;
    case ESCCurveTrackType::LocationZ:
      NewLocation.Z = Track.bAddBaseValue ? InitialLocation.Z + Value : Value;
      break;
    case ESCCurveTrackType::RotationP:
      NewRotation.Pitch =
          Track.bAddBaseValue ? InitialRotation.Pitch + Value : Value;
      break;
    case ESCCurveTrackType::RotationY:
      NewRotation.Yaw =
          Track.bAddBaseValue ? InitialRotation.Yaw + Value : Value;
      break;
    case ESCCurveTrackType::RotationR:
      NewRotation.Roll =
          Track.bAddBaseValue ? InitialRotation.Roll + Value : Value;
      break;
    case ESCCurveTrackType::ScaleX:
      NewScale.X = Track.bAddBaseValue ? InitialScale.X + Value : Value;
      break;
    case ESCCurveTrackType::ScaleY:
      NewScale.Y = Track.bAddBaseValue ? InitialScale.Y + Value : Value;
      break;
    case ESCCurveTrackType::ScaleZ:
      NewScale.Z = Track.bAddBaseValue ? InitialScale.Z + Value : Value;
      break;
    case ESCCurveTrackType::VectorLocation:
      NewLocation =
          Track.bAddBaseValue ? InitialLocation + VectorValue : VectorValue;
      break;
    case ESCCurveTrackType::VectorRotation:
      if (Track.bAddBaseValue) {
        NewRotation = InitialRotation + FRotator::MakeFromEuler(VectorValue);
      } else {
        NewRotation = FRotator::MakeFromEuler(VectorValue);
      }
      break;
    case ESCCurveTrackType::VectorScale:
      NewScale = Track.bAddBaseValue ? InitialScale + VectorValue : VectorValue;
      break;
    default:
      break;
    }
  }

  if (TransformSpace == ESCTransformSpace::Local) {
    SetRelativeLocation(NewLocation);
    SetRelativeRotation(NewRotation);
    SetRelativeScale3D(NewScale);
  } else {
    SetWorldLocation(NewLocation);
    SetWorldRotation(NewRotation);
    SetWorldScale3D(NewScale);
  }
}

void USCCurveAnimComponent::ProcessNotifies(float OldTime, float NewTime) {
  if (!AnimSequence)
    return;

  float Duration = GetEffectiveDuration();

  // Check each notify to see if we've crossed its time threshold
  for (int32 i = 0; i < AnimSequence->Notifies.Num(); ++i) {
    const FSCAnimNotify &Notify = AnimSequence->Notifies[i];

    // Skip if already fired
    if (FiredNotifyIndices.Contains(i)) {
      continue;
    }

    float NotifyTime = Notify.Time;
    bool bCrossedThreshold = false;

    if (bReverse) {
      // In reverse: check if we went from after to before the notify time
      bCrossedThreshold = (OldTime >= NotifyTime && NewTime < NotifyTime);
    } else {
      // Forward: check if we went from before to after the notify time
      bCrossedThreshold = (OldTime < NotifyTime && NewTime >= NotifyTime);
    }

    if (bCrossedThreshold) {
      OnAnimationNotify.Broadcast(Notify.NotifyName);
      FiredNotifyIndices.Add(i);
    }
  }

  // Reset fired notifies when looping
  if (bLoop) {
    if (!bReverse && NewTime < OldTime) {
      // Forward loop detected
      FiredNotifyIndices.Empty();
    } else if (bReverse && NewTime > OldTime) {
      // Reverse loop detected
      FiredNotifyIndices.Empty();
    }
  }
}
