#include "Components/Animation/SCCurveAnimComponent.h"
#include "Components/Animation/SCAnimSequence.h"
#include "Curves/CurveFloat.h"

USCCurveAnimComponent::USCCurveAnimComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;

  InitialLocation = FVector::ZeroVector;
  InitialRotation = FRotator::ZeroRotator;
  InitialScale = FVector::OneVector;
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

  if (bIsPlaying) {
    UpdateAnimation(DeltaTime);
  }
}

void USCCurveAnimComponent::Play() {
  PlayEx(nullptr, PlaybackDuration, true, false, bLoop);
}

void USCCurveAnimComponent::PlayEx(USCAnimSequence *Sequence, float Duration,
                                   bool bFromStart, bool bReverse,
                                   bool bInLoop) {
  if (Sequence) {
    AnimSequence = Sequence;
  }

  if (!AnimSequence) {
    return;
  }

  PlaybackDuration = (Duration > 0.0f) ? Duration : GetEffectiveDuration();
  bIsPlaying = true;
  bIsPaused = false;
  bFinished = false;
  bReversePlayback = bReverse;
  bLoop = bInLoop;

  if (bFromStart) {
    PlaybackCurrentTime = bReversePlayback ? PlaybackDuration : 0.0f;
    CurrentTime = PlaybackCurrentTime;
    FiredNotifyIndices.Empty();
  }
}

void USCCurveAnimComponent::PlayFromStart() {
  PlayEx(nullptr, PlaybackDuration, true, false, bLoop);
}

void USCCurveAnimComponent::Stop() {
  bIsPlaying = false;
  bIsPaused = false;
}

void USCCurveAnimComponent::Pause() { bIsPaused = true; }

void USCCurveAnimComponent::Resume() { bIsPaused = false; }

void USCCurveAnimComponent::ReverseFromEnd() {
  PlayEx(nullptr, PlaybackDuration, true, true, bLoop);
}

void USCCurveAnimComponent::ReverseFromCurrent() {
  bReversePlayback = !bReversePlayback;
  bIsPlaying = true;
  bIsPaused = false;
}

void USCCurveAnimComponent::SetPlaybackPosition(float NewTime) {
  PlaybackCurrentTime = FMath::Clamp(NewTime, 0.0f, PlaybackDuration);
  CurrentTime = PlaybackCurrentTime;
  UpdateAnimation(0.0f);
}

float USCCurveAnimComponent::GetEffectiveDuration() const {
  if (AnimSequence && AnimSequence->DefaultDuration > 0.0f) {
    return AnimSequence->DefaultDuration;
  }

  float MaxTime = 0.01f;
  if (AnimSequence) {
    for (const FSCCurveTrack &Track : AnimSequence->CurveTracks) {
      if (Track.CurveAsset) {
        float MinT, MaxT;
        Track.CurveAsset->GetTimeRange(MinT, MaxT);
        MaxTime = FMath::Max(MaxTime, MaxT);
      }
    }
  }
  return MaxTime;
}

void USCCurveAnimComponent::UpdateAnimation(float DeltaTime) {
  if (bIsPaused || PlaybackDuration <= 0.0f) {
    return;
  }

  float PrevTime = PlaybackCurrentTime;
  float Direction = bReversePlayback ? -1.0f : 1.0f;
  PlaybackCurrentTime += DeltaTime * PlayRate * Direction;

  bFinished = false;
  if (!bReversePlayback && PlaybackCurrentTime >= PlaybackDuration) {
    if (bLoop) {
      PlaybackCurrentTime -= PlaybackDuration;
      FiredNotifyIndices.Empty();
    } else {
      PlaybackCurrentTime = PlaybackDuration;
      bFinished = true;
    }
  } else if (bReversePlayback && PlaybackCurrentTime <= 0.0f) {
    if (bLoop) {
      PlaybackCurrentTime += PlaybackDuration;
      FiredNotifyIndices.Empty();
    } else {
      PlaybackCurrentTime = 0.0f;
      bFinished = true;
    }
  }

  CurrentTime = PlaybackCurrentTime;

  ApplyTransform();

  ProcessNotifies(PrevTime, PlaybackCurrentTime);

  float NormalizedTime =
      PlaybackDuration > 0.0f ? PlaybackCurrentTime / PlaybackDuration : 0.0f;

  // Calculate Reference Time (time within the original animation asset)
  // If we are time-stretching, we map PlaybackCurrentTime (0..PlaybackDuration)
  // to ReferenceTime (0..EffectiveDuration)
  float EffectiveDuration = GetEffectiveDuration();
  float ReferenceTime = NormalizedTime * EffectiveDuration;
  float PrevReferenceTime =
      (PlaybackDuration > 0.0f)
          ? (PrevTime / PlaybackDuration) * EffectiveDuration
          : 0.0f;

  CurrentTime = ReferenceTime;

  ApplyTransform(ReferenceTime);

  ProcessNotifies(PrevReferenceTime, ReferenceTime);

  OnAnimationUpdate.Broadcast(PlaybackCurrentTime, NormalizedTime);

  if (bFinished) {
    bIsPlaying = false;
    OnAnimationFinished.Broadcast();
  }
}

void USCCurveAnimComponent::ApplyTransform() {
  // Overload for internal use without arguments if needed,
  // but better to refactor checking call sites.
  // For now, let's keep the signature compatible or update usage.
  // The previous code used member vars, but we need to pass ReferenceTime.
  // Let's update the signature in header next if needed, or use a member var.
  // Since CurrentTime is now updated to ReferenceTime above, we can use
  // CurrentTime!
  ApplyTransform(CurrentTime);
}

void USCCurveAnimComponent::ApplyTransform(float SampleTime) {
  if (!AnimSequence) {
    return;
  }

  // NormalizedTime was used before, but curve sampling usually expects absolute
  // time unless the curve is explicitly 0..1. Standard UCurveFloat expects
  // time. If we assume curves are authored in seconds, we should use SampleTime
  // (ReferenceTime).

  FVector NewLoc = InitialLocation;
  FRotator NewRot = InitialRotation;
  FVector NewScale = InitialScale;

  for (const FSCCurveTrack &Track : AnimSequence->CurveTracks) {
    if (UCurveFloat *Curve = Cast<UCurveFloat>(Track.CurveAsset)) {
      float Value = Curve->GetFloatValue(SampleTime);

      switch (Track.TrackType) {
      case ESCCurveTrackType::LocationX:
        NewLoc.X = Track.bAddBaseValue ? InitialLocation.X + Value : Value;
        break;
      case ESCCurveTrackType::LocationY:
        NewLoc.Y = Track.bAddBaseValue ? InitialLocation.Y + Value : Value;
        break;
      case ESCCurveTrackType::LocationZ:
        NewLoc.Z = Track.bAddBaseValue ? InitialLocation.Z + Value : Value;
        break;
      case ESCCurveTrackType::RotationP:
        NewRot.Pitch =
            Track.bAddBaseValue ? InitialRotation.Pitch + Value : Value;
        break;
      case ESCCurveTrackType::RotationY:
        NewRot.Yaw = Track.bAddBaseValue ? InitialRotation.Yaw + Value : Value;
        break;
      case ESCCurveTrackType::RotationR:
        NewRot.Roll =
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
      default:
        break;
      }
    }
  }

  if (TransformSpace == ESCTransformSpace::Local) {
    SetRelativeLocationAndRotation(NewLoc, NewRot);
    SetRelativeScale3D(NewScale);
  } else {
    SetWorldLocationAndRotation(NewLoc, NewRot);
    SetWorldScale3D(NewScale);
  }
}

void USCCurveAnimComponent::ProcessNotifies(float OldTime, float NewTime) {
  if (!AnimSequence) {
    return;
  }

  bool bIsForward = NewTime >= OldTime;
  float MinTime = FMath::Min(OldTime, NewTime);
  float MaxTime = FMath::Max(OldTime, NewTime);

  for (int32 i = 0; i < AnimSequence->Notifies.Num(); ++i) {
    const FSCAnimNotify &Notify = AnimSequence->Notifies[i];

    // If we are looping/resetting, we might want to handle that differently,
    // but basic range check works for linear playback.

    if (FiredNotifyIndices.Contains(i))
      continue;

    bool bShouldTrigger = false;
    // Check inclusive/exclusive correctly to avoid double firing on boundaries
    if (bIsForward) {
      bShouldTrigger = (Notify.Time > MinTime && Notify.Time <= MaxTime);
    } else {
      bShouldTrigger = (Notify.Time >= MinTime && Notify.Time < MaxTime);
    }

    if (bShouldTrigger) {
      OnAnimationNotify.Broadcast(Notify.NotifyName);
      FiredNotifyIndices.Add(i);
    }
  }
}
