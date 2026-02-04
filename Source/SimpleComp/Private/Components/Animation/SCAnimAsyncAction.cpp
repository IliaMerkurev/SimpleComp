#include "Components/Animation/SCAnimAsyncAction.h"
#include "Components/Animation/SCCurveAnimComponent.h"

USCAnimAsyncAction *
USCAnimAsyncAction::CreateProxy(USCCurveAnimComponent *Component,
                                USCAnimSequence *Sequence, double Duration) {
  if (!Component)
    return nullptr;
  USCAnimAsyncAction *Proxy = NewObject<USCAnimAsyncAction>();
  Proxy->TargetComponent = Component;
  Proxy->TargetSequence = Sequence;
  Proxy->TargetDuration = static_cast<float>(Duration);
  Proxy->RegisterWithGameInstance(Component);
  return Proxy;
}

void USCAnimAsyncAction::Activate() {
  if (!TargetComponent) {
    Cleanup();
    return;
  }

  TargetComponent->OnAnimationUpdate.AddDynamic(
      this, &USCAnimAsyncAction::HandleUpdate);
  TargetComponent->OnAnimationFinished.AddDynamic(
      this, &USCAnimAsyncAction::HandleFinished);
  TargetComponent->OnAnimationNotify.AddDynamic(
      this, &USCAnimAsyncAction::HandleNotify);
}

void USCAnimAsyncAction::Play(bool bFromStart) {
  if (TargetComponent)
    TargetComponent->PlayEx(TargetSequence, TargetDuration, bFromStart, false);
}

void USCAnimAsyncAction::Stop() {
  if (TargetComponent)
    TargetComponent->Stop();
}

void USCAnimAsyncAction::Pause() {
  if (TargetComponent)
    TargetComponent->Pause();
}

void USCAnimAsyncAction::Resume() {
  if (TargetComponent)
    TargetComponent->Resume();
}

void USCAnimAsyncAction::ReverseFromEnd(bool bFromStart) {
  if (TargetComponent && TargetSequence) {
    TargetComponent->PlayEx(TargetSequence, TargetDuration, bFromStart, true);
  }
}

void USCAnimAsyncAction::ReverseFromCurrent() {
  if (TargetComponent)
    TargetComponent->ReverseFromCurrent();
}

void USCAnimAsyncAction::HandleUpdate(float CurrentTime, float NormalizedTime) {
  Update.Broadcast(NAME_None, static_cast<double>(CurrentTime),
                   static_cast<double>(NormalizedTime));
}

void USCAnimAsyncAction::HandleFinished() {
  float FinalTime =
      TargetComponent ? TargetComponent->GetPlaybackPosition() : 0.0f;
  Finished.Broadcast(NAME_None, static_cast<double>(FinalTime), 1.0);
}

void USCAnimAsyncAction::HandleNotify(FName NotifyName) {
  OnNotify.Broadcast(
      NotifyName,
      static_cast<double>(
          TargetComponent ? TargetComponent->GetPlaybackPosition() : 0.0f),
      0.0);
}

void USCAnimAsyncAction::Cleanup() {
  if (TargetComponent) {
    TargetComponent->OnAnimationUpdate.RemoveAll(this);
    TargetComponent->OnAnimationFinished.RemoveAll(this);
    TargetComponent->OnAnimationNotify.RemoveAll(this);
  }
  SetReadyToDestroy();
}
