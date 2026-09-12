#include "Components/Animation/SCAnimAsyncAction.h"
#include "Components/Animation/SCCurveAnimComponent.h"
#include "Components/Animation/SCAnimSequence.h"

USCAnimAsyncAction* USCAnimAsyncAction::CreateProxy(USCCurveAnimComponent* Component, USCAnimSequence* Sequence,
    double Duration, bool bLoop)
{
    if (!IsValid(Component))
    {
        return nullptr;
    }
    USCAnimAsyncAction* Proxy = NewObject<USCAnimAsyncAction>();
    Proxy->TargetComponent = Component;
    Proxy->TargetSequence = Sequence;
    Proxy->TargetDuration =
        FMath::IsFinite(Duration) && Duration > 0.0 && Duration <= MAX_flt ? static_cast<float>(Duration) : -1.0f;
    Proxy->TargetLoop = bLoop;
    return Proxy;
}

void USCAnimAsyncAction::Activate()
{
    // The K2 node issues an explicit command after activation. Activation must not rewind playback.
}

void USCAnimAsyncAction::BeginDestroy()
{
    Cleanup();
    Super::BeginDestroy();
}

void USCAnimAsyncAction::BindToPlayback()
{
    USCCurveAnimComponent* Component = TargetComponent.Get();
    if (!IsValid(Component) || !Component->IsPlaying())
    {
        Cleanup();
        return;
    }
    TargetSequence = Component->AnimSequence;
    RegisterWithGameInstance(Component);
    Component->OnAnimationUpdate.AddUniqueDynamic(this, &USCAnimAsyncAction::HandleUpdate);
    Component->OnAnimationFinished.AddUniqueDynamic(this, &USCAnimAsyncAction::HandleFinished);
    Component->OnAnimationNotify.AddUniqueDynamic(this, &USCAnimAsyncAction::HandleNotify);
    Component->OnPlaybackInvalidated.AddUObject(this, &USCAnimAsyncAction::Cleanup);
    bBound = true;
}

void USCAnimAsyncAction::Play(bool bFromStart)
{
    if (USCCurveAnimComponent* Component = TargetComponent.Get())
    {
        Cleanup();
        const uint64 ExpectedRevision = Component->GetPlaybackRevision() + 1;
        Component->PlayEx(TargetSequence, TargetDuration, bFromStart, false, TargetLoop);
        if (IsValid(Component) && Component->GetPlaybackRevision() == ExpectedRevision)
        {
            BindToPlayback();
        }
    }
}

void USCAnimAsyncAction::Stop()
{
    if (USCCurveAnimComponent* Component = TargetComponent.Get())
    {
        Component->Stop();
    }
    Cleanup();
}

void USCAnimAsyncAction::Pause()
{
    if (USCCurveAnimComponent* Component = TargetComponent.Get())
    {
        Component->Pause();
    }
    if (!bBound)
    {
        SetReadyToDestroy();
    }
}

void USCAnimAsyncAction::Resume()
{
    if (USCCurveAnimComponent* Component = TargetComponent.Get())
    {
        Component->Resume();
    }
    if (!bBound)
    {
        SetReadyToDestroy();
    }
}

void USCAnimAsyncAction::ReverseFromEnd(bool bFromStart)
{
    if (USCCurveAnimComponent* Component = TargetComponent.Get())
    {
        Cleanup();
        const uint64 ExpectedRevision = Component->GetPlaybackRevision() + 1;
        Component->PlayEx(TargetSequence, TargetDuration, bFromStart, true, TargetLoop);
        if (IsValid(Component) && Component->GetPlaybackRevision() == ExpectedRevision)
        {
            BindToPlayback();
        }
    }
}

void USCAnimAsyncAction::ReverseFromCurrent()
{
    if (USCCurveAnimComponent* Component = TargetComponent.Get())
    {
        Cleanup();
        const uint64 ExpectedRevision = Component->GetPlaybackRevision() + 1;
        Component->ReverseFromCurrent();
        if (IsValid(Component) && Component->GetPlaybackRevision() == ExpectedRevision)
        {
            BindToPlayback();
        }
    }
}

bool USCAnimAsyncAction::IsPlaybackCurrent()
{
    USCCurveAnimComponent* Component = TargetComponent.Get();
    if (!bBound || !IsValid(Component) || Component->AnimSequence != TargetSequence)
    {
        Cleanup();
        return false;
    }
    return true;
}

void USCAnimAsyncAction::HandleUpdate(float CurrentTime, float NormalizedTime)
{
    if (IsPlaybackCurrent())
    {
        Update.Broadcast(NAME_None, CurrentTime, NormalizedTime);
    }
}

void USCAnimAsyncAction::HandleFinished()
{
    if (!IsPlaybackCurrent())
    {
        return;
    }
    const USCCurveAnimComponent* Component = TargetComponent.Get();
    const double Time = Component->GetPlaybackPosition();
    const double Normalized = Component->PlaybackDuration > 0.0f ? Time / Component->PlaybackDuration : 0.0;
    Cleanup();
    Finished.Broadcast(NAME_None, Time, Normalized);
}

void USCAnimAsyncAction::HandleNotify(FName NotifyName)
{
    if (!IsPlaybackCurrent())
    {
        return;
    }
    const USCCurveAnimComponent* Component = TargetComponent.Get();
    const double Time = Component->GetPlaybackPosition();
    const double Normalized = Component->PlaybackDuration > 0.0f ? Time / Component->PlaybackDuration : 0.0;
    OnNotify.Broadcast(NotifyName, Time, Normalized);
}

void USCAnimAsyncAction::Cleanup()
{
    if (USCCurveAnimComponent* Component = TargetComponent.Get())
    {
        Component->OnAnimationUpdate.RemoveAll(this);
        Component->OnAnimationFinished.RemoveAll(this);
        Component->OnAnimationNotify.RemoveAll(this);
        Component->OnPlaybackInvalidated.RemoveAll(this);
    }
    bBound = false;
    SetReadyToDestroy();
}
