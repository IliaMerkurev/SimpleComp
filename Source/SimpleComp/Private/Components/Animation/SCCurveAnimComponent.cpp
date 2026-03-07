#include "Components/Animation/SCCurveAnimComponent.h"
#include "Components/Animation/SCAnimSequence.h"
#include "Curves/CurveFloat.h"
#include "Engine/CurveTable.h"

USCCurveAnimComponent::USCCurveAnimComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    InitialLocation = FVector::ZeroVector;
    InitialRotation = FRotator::ZeroRotator;
    InitialScale = FVector::OneVector;
}

void USCCurveAnimComponent::BeginPlay()
{
    Super::BeginPlay();
    ensure(GetOwner() != nullptr);

    InitialLocation = GetRelativeLocation();
    InitialRotation = GetRelativeRotation();
    InitialScale = GetRelativeScale3D();

    if (bAutoPlay && AnimSequence)
    {
        Play();
    }
}

void USCCurveAnimComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsPlaying)
    {
        UpdateAnimation(DeltaTime);
    }
}

void USCCurveAnimComponent::Play()
{
    PlayEx(nullptr, PlaybackDuration, true, false, bLoop);
}

void USCCurveAnimComponent::PlayEx(USCAnimSequence* Sequence, float Duration, bool bFromStart, bool bReverse,
    bool bInLoop)
{
    if (Sequence)
    {
        AnimSequence = Sequence;
    }

    if (!AnimSequence)
    {
        return;
    }

    PlaybackDuration = (Duration > 0.0f) ? Duration : GetEffectiveDuration();
    bIsPlaying = true;
    bIsPaused = false;
    bFinished = false;
    bReversePlayback = bReverse;
    bLoop = bInLoop;

    if (bFromStart)
    {
        PlaybackCurrentTime = bReversePlayback ? PlaybackDuration : 0.0f;
        CurrentTime = PlaybackCurrentTime;
        FiredNotifyIndices.Empty();
    }

    SetComponentTickEnabled(true);
}

void USCCurveAnimComponent::PlayFromStart()
{
    PlayEx(nullptr, PlaybackDuration, true, false, bLoop);
}

void USCCurveAnimComponent::Stop()
{
    bIsPlaying = false;
    bIsPaused = false;
    SetComponentTickEnabled(false);
}

void USCCurveAnimComponent::Pause()
{
    bIsPaused = true;
    SetComponentTickEnabled(false);
}

void USCCurveAnimComponent::Resume()
{
    bIsPaused = false;
    SetComponentTickEnabled(true);
}

void USCCurveAnimComponent::ReverseFromEnd()
{
    PlayEx(nullptr, PlaybackDuration, true, true, bLoop);
}

void USCCurveAnimComponent::ReverseFromCurrent()
{
    bReversePlayback = !bReversePlayback;
    bIsPlaying = true;
    bIsPaused = false;
    SetComponentTickEnabled(true);
}

void USCCurveAnimComponent::SetPlaybackPosition(float NewTime)
{
    PlaybackCurrentTime = FMath::Clamp(NewTime, 0.0f, PlaybackDuration);
    CurrentTime = PlaybackCurrentTime;
    UpdateAnimation(0.0f);
}

float USCCurveAnimComponent::GetEffectiveDuration() const
{
    if (AnimSequence && AnimSequence->DefaultDuration > 0.0f)
    {
        return AnimSequence->DefaultDuration;
    }

    float MaxTime = 0.01f;
    if (AnimSequence)
    {
        for (const FSCCurveTrack& Track : AnimSequence->CurveTracks)
        {
            if (Track.CurveAsset)
            {
                float MinT, MaxT;
                Track.CurveAsset->GetTimeRange(MinT, MaxT);
                MaxTime = FMath::Max(MaxTime, MaxT);
            }
            else if (Track.CurveTableAsset)
            {
                for (const auto& RowPair : Track.CurveTableAsset->GetRowMap())
                {
                    if (const FRealCurve* RowCurve = reinterpret_cast<const FRealCurve*>(RowPair.Value))
                    {
                        float RowMin, RowMax;
                        RowCurve->GetTimeRange(RowMin, RowMax);
                        MaxTime = FMath::Max(MaxTime, RowMax);
                    }
                }
            }
        }
    }
    return MaxTime;
}

void USCCurveAnimComponent::UpdateAnimation(float DeltaTime)
{
    if (bIsPaused || PlaybackDuration <= 0.0f)
    {
        return;
    }

    float PrevTime = PlaybackCurrentTime;
    float Direction = bReversePlayback ? -1.0f : 1.0f;
    PlaybackCurrentTime += DeltaTime * PlayRate * Direction;

    bFinished = false;
    if (!bReversePlayback && PlaybackCurrentTime >= PlaybackDuration)
    {
        if (bLoop)
        {
            PlaybackCurrentTime -= PlaybackDuration;
            FiredNotifyIndices.Empty();
        }
        else
        {
            PlaybackCurrentTime = PlaybackDuration;
            bFinished = true;
        }
    }
    else if (bReversePlayback && PlaybackCurrentTime <= 0.0f)
    {
        if (bLoop)
        {
            PlaybackCurrentTime += PlaybackDuration;
            FiredNotifyIndices.Empty();
        }
        else
        {
            PlaybackCurrentTime = 0.0f;
            bFinished = true;
        }
    }

    CurrentTime = PlaybackCurrentTime;

    ApplyTransform();

    ProcessNotifies(PrevTime, PlaybackCurrentTime);

    float NormalizedTime = PlaybackDuration > 0.0f ? PlaybackCurrentTime / PlaybackDuration : 0.0f;

    float EffectiveDuration = GetEffectiveDuration();
    float ReferenceTime = NormalizedTime * EffectiveDuration;
    float PrevReferenceTime = (PlaybackDuration > 0.0f) ? (PrevTime / PlaybackDuration) * EffectiveDuration : 0.0f;

    CurrentTime = ReferenceTime;

    ApplyTransform(ReferenceTime);

    ProcessNotifies(PrevReferenceTime, ReferenceTime);

    OnAnimationUpdate.Broadcast(PlaybackCurrentTime, NormalizedTime);

    if (bFinished)
    {
        bIsPlaying = false;
        OnAnimationFinished.Broadcast();
        SetComponentTickEnabled(false);
    }
}

void USCCurveAnimComponent::ApplyTransform()
{
    ApplyTransform(CurrentTime);
}

void USCCurveAnimComponent::ApplyTransform(float SampleTime)
{
    if (!AnimSequence)
    {
        return;
    }

    FVector NewLoc = InitialLocation;
    FRotator NewRot = InitialRotation;
    FVector NewScale = InitialScale;

    for (const FSCCurveTrack& Track : AnimSequence->CurveTracks)
    {
        if (UCurveFloat* Curve = Cast<UCurveFloat>(Track.CurveAsset))
        {
            float Value = Curve->GetFloatValue(SampleTime) * Track.ScaleFloat;

            switch (Track.TrackType)
            {
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
                    NewRot.Pitch = Track.bAddBaseValue ? InitialRotation.Pitch + Value : Value;
                    break;
                case ESCCurveTrackType::RotationY:
                    NewRot.Yaw = Track.bAddBaseValue ? InitialRotation.Yaw + Value : Value;
                    break;
                case ESCCurveTrackType::RotationR:
                    NewRot.Roll = Track.bAddBaseValue ? InitialRotation.Roll + Value : Value;
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
        else if (Track.CurveTableAsset)
        {
            auto SampleTableRow = [&](const FName& RowName) -> float
            {
                const FRealCurve* FoundCurve = Track.CurveTableAsset->FindCurve(RowName, TEXT(""), false);
                return FoundCurve ? FoundCurve->Eval(SampleTime) : 0.0f;
            };

            const float X = SampleTableRow(FName("X")) * Track.ScaleVector.X;
            const float Y = SampleTableRow(FName("Y")) * Track.ScaleVector.Y;
            const float Z = SampleTableRow(FName("Z")) * Track.ScaleVector.Z;

            switch (Track.TrackType)
            {
                case ESCCurveTrackType::TableLocation:
                    NewLoc.X = Track.bAddBaseValue ? InitialLocation.X + X : X;
                    NewLoc.Y = Track.bAddBaseValue ? InitialLocation.Y + Y : Y;
                    NewLoc.Z = Track.bAddBaseValue ? InitialLocation.Z + Z : Z;
                    break;
                case ESCCurveTrackType::TableRotation:
                    NewRot.Pitch = Track.bAddBaseValue ? InitialRotation.Pitch + X : X;
                    NewRot.Yaw = Track.bAddBaseValue ? InitialRotation.Yaw + Y : Y;
                    NewRot.Roll = Track.bAddBaseValue ? InitialRotation.Roll + Z : Z;
                    break;
                case ESCCurveTrackType::TableScale:
                    NewScale.X = Track.bAddBaseValue ? InitialScale.X + X : X;
                    NewScale.Y = Track.bAddBaseValue ? InitialScale.Y + Y : Y;
                    NewScale.Z = Track.bAddBaseValue ? InitialScale.Z + Z : Z;
                    break;
                default:
                    break;
            }
        }
    }

    if (TransformSpace == ESCTransformSpace::Local)
    {
        SetRelativeLocationAndRotation(NewLoc, NewRot);
        SetRelativeScale3D(NewScale);
    }
    else
    {
        SetWorldLocationAndRotation(NewLoc, NewRot);
        SetWorldScale3D(NewScale);
    }
}

void USCCurveAnimComponent::ProcessNotifies(float OldTime, float NewTime)
{
    if (!AnimSequence)
    {
        return;
    }

    bool bIsForward = NewTime >= OldTime;
    float MinTime = FMath::Min(OldTime, NewTime);
    float MaxTime = FMath::Max(OldTime, NewTime);

    for (int32 i = 0; i < AnimSequence->Notifies.Num(); ++i)
    {
        const FSCAnimNotify& Notify = AnimSequence->Notifies[i];

        if (FiredNotifyIndices.Contains(i))
            continue;

        bool bShouldTrigger = false;
        if (bIsForward)
        {
            bShouldTrigger = (Notify.Time > MinTime && Notify.Time <= MaxTime);
        }
        else
        {
            bShouldTrigger = (Notify.Time >= MinTime && Notify.Time < MaxTime);
        }

        if (bShouldTrigger)
        {
            OnAnimationNotify.Broadcast(Notify.NotifyName);
            FiredNotifyIndices.Add(i);
        }
    }
}
