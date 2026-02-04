#include "Components/Animation/SCAnimAsyncAction.h"
#include "Components/Animation/SCCurveAnimComponent.h"

USCAnimAsyncAction *
USCAnimAsyncAction::CreateProxy(USCCurveAnimComponent *Component,
                                USCAnimSequence *Sequence, float Duration) {
  if (!Component)
    return nullptr;
  USCAnimAsyncAction *Proxy = NewObject<USCAnimAsyncAction>();
  Proxy->TargetComponent = Component;
  Proxy->TargetSequence = Sequence;
  Proxy->TargetDuration = Duration;
  Proxy->RegisterWithGameInstance(Component);
  return Proxy;
}

void USCAnimAsyncAction::Activate() {
  if (!TargetComponent) {
    Cleanup();
    return;
  }

  // Bind to component delegates
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

void USCAnimAsyncAction::Reverse(bool bFromStart) {
  if (TargetComponent)
    TargetComponent->PlayEx(TargetSequence, TargetDuration, bFromStart, true);
}

void USCAnimAsyncAction::ReverseFromCurrent() {
  if (TargetComponent)
    TargetComponent->ReverseFromCurrent();
}

void USCAnimAsyncAction::HandleUpdate(float CurrentTime, float NormalizedTime) {
  Update.Broadcast(NAME_None, CurrentTime, NormalizedTime);
}

void USCAnimAsyncAction::HandleFinished() {
  float FinalTime =
      TargetComponent ? TargetComponent->GetPlaybackPosition() : 0.0f;
  Finished.Broadcast(NAME_None, FinalTime, 1.0f);
  // We DON'T cleanup here if we want to support multiple plays from the same
  // node But for a professional node, usually Finished means it's done.
  // However, Timeline stays alive. So we stay alive.
}

void USCAnimAsyncAction::HandleNotify(FName NotifyName) {
  OnNotify.Broadcast(
      NotifyName,
      TargetComponent ? TargetComponent->GetPlaybackPosition() : 0.0f, 0.0f);
}

void USCAnimAsyncAction::Cleanup() {
  if (TargetComponent) {
    TargetComponent->OnAnimationUpdate.RemoveAll(this);
    TargetComponent->OnAnimationFinished.RemoveAll(this);
    TargetComponent->OnAnimationNotify.RemoveAll(this);
  }
  SetReadyToDestroy();
}
