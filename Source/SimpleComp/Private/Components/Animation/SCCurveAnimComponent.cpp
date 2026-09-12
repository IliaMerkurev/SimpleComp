#include "Components/Animation/SCCurveAnimComponent.h"
#include "Components/Animation/SCAnimSequence.h"
#include "Curves/CurveFloat.h"
#include "Engine/CurveTable.h"

DEFINE_LOG_CATEGORY_STATIC(LogSCCurveAnimation, Log, All);

USCCurveAnimComponent::USCCurveAnimComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USCCurveAnimComponent::CaptureInitialTransforms()
{
    if (!bHasInitialTransforms)
    {
        InitialLocalTransform = GetRelativeTransform();
        InitialWorldTransform = GetComponentTransform();
        bHasInitialTransforms = true;
    }
}

void USCCurveAnimComponent::BeginPlay()
{
    Super::BeginPlay();
    CaptureInitialTransforms();
    if (bAutoPlay && IsValid(AnimSequence))
    {
        Play();
    }
}

void USCCurveAnimComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bShuttingDown = true;
    Stop();
    Super::EndPlay(EndPlayReason);
}

void USCCurveAnimComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    bShuttingDown = true;
    Stop();
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void USCCurveAnimComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bIsPlaying && !bIsPaused)
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
    if (bShuttingDown)
    {
        return;
    }
    USCAnimSequence* SelectedSequence = Sequence ? Sequence : AnimSequence.Get();
    if (!IsValid(SelectedSequence))
    {
        Stop();
        return;
    }
    const uint64 Revision = ++PlaybackRevision;
    OnPlaybackInvalidated.Broadcast();
    if (!IsValid(this) || bShuttingDown || Revision != PlaybackRevision)
    {
        return;
    }
    AnimSequence = SelectedSequence;
    CaptureInitialTransforms();
    PlaybackDuration = FMath::IsFinite(Duration) && Duration > 0.0f ? Duration : GetEffectiveDuration();
    LastPlaybackDuration = PlaybackDuration;
    bIsPlaying = true;
    bIsPaused = false;
    bReversePlayback = bReverse;
    bLoop = bInLoop;
    PlaybackCurrentTime =
        bFromStart ? (bReverse ? PlaybackDuration : 0.0f) : FMath::Clamp(PlaybackCurrentTime, 0.0f, PlaybackDuration);
    bIncludeBoundaryNotify = bFromStart || PlaybackCurrentTime == 0.f || PlaybackCurrentTime == PlaybackDuration;
    SetComponentTickEnabled(true);
    EvaluatePosition();
}

void USCCurveAnimComponent::PlayFromStart()
{
    PlayEx(nullptr, PlaybackDuration, true, false, bLoop);
}

void USCCurveAnimComponent::Stop()
{
    ++PlaybackRevision;
    bIsPlaying = false;
    bIsPaused = false;
    bIncludeBoundaryNotify = false;
    SetComponentTickEnabled(false);
    OnPlaybackInvalidated.Broadcast();
}

void USCCurveAnimComponent::Pause()
{
    ++PlaybackRevision;
    bIsPaused = bIsPlaying;
    SetComponentTickEnabled(false);
}

void USCCurveAnimComponent::Resume()
{
    if (bIsPlaying && bIsPaused)
    {
        ++PlaybackRevision;
        bIsPaused = false;
        SetComponentTickEnabled(true);
    }
}

void USCCurveAnimComponent::ReverseFromEnd()
{
    PlayEx(nullptr, PlaybackDuration, true, true, bLoop);
}

void USCCurveAnimComponent::ReverseFromCurrent()
{
    PlayEx(nullptr, PlaybackDuration, false, !bReversePlayback, bLoop);
}

void USCCurveAnimComponent::SetPlaybackPosition(float NewTime)
{
    if (!FMath::IsFinite(NewTime) || !FMath::IsFinite(PlaybackDuration) || PlaybackDuration <= 0.0f)
    {
        return;
    }
    ++PlaybackRevision;
    CaptureInitialTransforms();
    PlaybackCurrentTime = FMath::Clamp(NewTime, 0.0f, PlaybackDuration);
    LastPlaybackDuration = PlaybackDuration;
    bIncludeBoundaryNotify = false;
    EvaluatePosition();
}

void USCCurveAnimComponent::EvaluatePosition()
{
    CurrentTime = PlaybackCurrentTime;
    if (IsValid(AnimSequence) && FMath::IsFinite(PlaybackDuration) && PlaybackDuration > 0.0f)
    {
        ApplyTransform((PlaybackCurrentTime / PlaybackDuration) * GetEffectiveDuration());
    }
}

float USCCurveAnimComponent::GetEffectiveDuration() const
{
    if (AnimSequence && FMath::IsFinite(AnimSequence->DefaultDuration) && AnimSequence->DefaultDuration > 0.0f)
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
                if (FMath::IsFinite(MaxT))
                {
                    MaxTime = FMath::Max(MaxTime, MaxT);
                }
            }
            else if (Track.CurveTableAsset)
            {
                for (const FName& RowName : {FName("X"), FName("Y"), FName("Z")})
                {
                    if (const FRealCurve* RowCurve = Track.CurveTableAsset->FindCurve(RowName, TEXT(""), false))
                    {
                        float RowMin, RowMax;
                        RowCurve->GetTimeRange(RowMin, RowMax);
                        if (FMath::IsFinite(RowMax))
                        {
                            MaxTime = FMath::Max(MaxTime, RowMax);
                        }
                    }
                }
            }
        }
    }
    return MaxTime;
}

void USCCurveAnimComponent::UpdateAnimation(float DeltaTime)
{
    if (!IsValid(AnimSequence))
    {
        Stop();
        return;
    }
    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f || !FMath::IsFinite(PlayRate) ||
        !FMath::IsFinite(PlaybackDuration) || PlaybackDuration <= 0.0f)
    {
        return;
    }

    // Duration edits preserve normalized progress; explicit seeks use playback seconds.
    if (PlaybackDuration != LastPlaybackDuration && LastPlaybackDuration > 0.0f)
    {
        PlaybackCurrentTime = FMath::Clamp(PlaybackCurrentTime / LastPlaybackDuration, 0.0f, 1.0f) * PlaybackDuration;
    }
    LastPlaybackDuration = PlaybackDuration;
    const uint64 Revision = PlaybackRevision;
    const TObjectPtr<USCAnimSequence> Sequence = AnimSequence;
    const float Duration = PlaybackDuration;
    const float Rate = PlayRate;
    const bool bLooping = bLoop;
    const ESCTransformSpace Space = TransformSpace;
    const double Advance = static_cast<double>(DeltaTime) * Rate * (bReversePlayback ? -1.0 : 1.0);
    const bool bForward = Advance >= 0.0;
    const double Start = PlaybackCurrentTime;
    const double Unwrapped = Start + Advance;
    const bool bComplete = !bLooping && Advance != 0.0 && (bForward ? Unwrapped >= Duration : Unwrapped <= 0.0);
    double FinalTime = FMath::Clamp(Unwrapped, 0.0, static_cast<double>(Duration));
    if (bLooping && Advance != 0.0)
    {
        FinalTime = FMath::Fmod(Unwrapped, static_cast<double>(Duration));
        if (FinalTime < 0.0)
        {
            FinalTime += Duration;
        }
        if (!bForward && FinalTime == 0.0)
        {
            FinalTime = Duration;
        }
    }
    PlaybackCurrentTime = static_cast<float>(FinalTime);
    const bool bIncludeStart = bIncludeBoundaryNotify;
    if (Advance != 0.0)
    {
        bIncludeBoundaryNotify = false;
    }
    EvaluatePosition();

    auto IsCurrent = [&]()
    {
        return IsValid(this) && PlaybackRevision == Revision && AnimSequence == Sequence &&
               PlaybackDuration == Duration && PlayRate == Rate && bLoop == bLooping && TransformSpace == Space;
    };
    if (!IsCurrent())
    {
        return;
    }

    if (Advance != 0.0 && OnAnimationNotify.IsBound() && !Sequence->Notifies.IsEmpty())
    {
        const double SequenceDuration = GetEffectiveDuration();
        const auto ToSampleTime = [Duration, SequenceDuration](double Time)
        {
            return static_cast<float>((Time / Duration) * SequenceDuration);
        };
        double Cursor = Start;
        double Remaining = FMath::Abs(Advance);
        bool bInclude = bIncludeStart;
        while (Remaining > 0.0)
        {
            const double Distance = bForward ? Duration - Cursor : Cursor;
            const double Step = FMath::Min(Remaining, Distance);
            const double Next = Cursor + (bForward ? Step : -Step);
            if (!ProcessNotifies(ToSampleTime(Cursor), ToSampleTime(Next), bInclude, Revision) || !IsCurrent())
            {
                return;
            }
            if (Step > 0.0 && Remaining - Step == Remaining)
            {
                // Reject an unrepresentable traversal instead of hanging on a non-decreasing loop counter.
                UE_LOG(LogSCCurveAnimation, Warning,
                       TEXT("Notify traversal exceeded playback time precision; playback stopped."));
                Stop();
                return;
            }
            Remaining -= Step;
            if (Step < Distance || !bLooping)
            {
                break;
            }
            Cursor = bForward ? 0.0 : Duration;
            bInclude = true;
            if (Remaining == 0.0)
            {
                if (!ProcessNotifies(ToSampleTime(Cursor), ToSampleTime(Cursor), true, Revision) || !IsCurrent())
                {
                    return;
                }
            }
        }
    }
    OnAnimationUpdate.Broadcast(CurrentTime, CurrentTime / Duration);
    if (!IsCurrent())
    {
        return;
    }
    if (bComplete)
    {
        bIsPlaying = false;
        SetComponentTickEnabled(false);
        OnAnimationFinished.Broadcast();
    }
}

void USCCurveAnimComponent::ApplyTransform(float SampleTime)
{
    if (!AnimSequence)
    {
        return;
    }

    const FTransform& Base = TransformSpace == ESCTransformSpace::Local ? InitialLocalTransform : InitialWorldTransform;
    const FVector InitialLocation = Base.GetLocation();
    const FRotator InitialRotation = Base.Rotator();
    const FVector InitialScale = Base.GetScale3D();
    FVector NewLoc = InitialLocation;
    FRotator NewRot = InitialRotation;
    FVector NewScale = InitialScale;

    for (const FSCCurveTrack& Track : AnimSequence->CurveTracks)
    {
        const bool bIsTableTrack = Track.TrackType == ESCCurveTrackType::TableLocation ||
            Track.TrackType == ESCCurveTrackType::TableRotation || Track.TrackType == ESCCurveTrackType::TableScale;

        if (bIsTableTrack && Track.CurveTableAsset)
        {
            auto SampleTableRow = [&](const FName& RowName) -> float
            {
                const FRealCurve* FoundCurve = Track.CurveTableAsset->FindCurve(RowName, TEXT(""), false);
                return FoundCurve ? FoundCurve->Eval(SampleTime) : 0.0f;
            };

            const float X = SampleTableRow(FName("X")) * Track.ScaleVector.X * Track.ScaleCurve;
            const float Y = SampleTableRow(FName("Y")) * Track.ScaleVector.Y * Track.ScaleCurve;
            const float Z = SampleTableRow(FName("Z")) * Track.ScaleVector.Z * Track.ScaleCurve;

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
        else if (UCurveFloat* Curve = Cast<UCurveFloat>(Track.CurveAsset))
        {
            float Value = Curve->GetFloatValue(SampleTime) * Track.ScaleCurve;

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
    }

    if (NewLoc.ContainsNaN() || NewRot.ContainsNaN() || NewScale.ContainsNaN())
    {
        return;
    }
    const uint64 Revision = PlaybackRevision;
    if (TransformSpace == ESCTransformSpace::Local)
    {
        SetRelativeScale3D(NewScale);
        if (Revision == PlaybackRevision && IsValid(this))
        {
            SetRelativeLocationAndRotation(NewLoc, NewRot);
        }
    }
    else
    {
        SetWorldScale3D(NewScale);
        if (Revision == PlaybackRevision && IsValid(this))
        {
            SetWorldLocationAndRotation(NewLoc, NewRot);
        }
    }
}

bool USCCurveAnimComponent::ProcessNotifies(float OldTime, float NewTime, bool bIncludeStart, uint64 Revision)
{
    if (!IsValid(AnimSequence))
    {
        return false;
    }
    const TObjectPtr<USCAnimSequence> Sequence = AnimSequence;
    const float Duration = PlaybackDuration;
    const float Rate = PlayRate;
    const bool bLooping = bLoop;
    const ESCTransformSpace Space = TransformSpace;
    const bool bForward = NewTime >= OldTime;
    const float SequenceDuration = GetEffectiveDuration();
    TArray<FSCAnimNotify> Crossed;
    for (const FSCAnimNotify& Notify : Sequence->Notifies)
    {
        if (Notify.NotifyName.IsNone() || !FMath::IsFinite(Notify.Time) || Notify.Time < 0.0f ||
            Notify.Time > SequenceDuration)
        {
            continue;
        }
        const bool bCrossed = bForward ? Notify.Time > OldTime && Notify.Time <= NewTime
                                       : Notify.Time < OldTime && Notify.Time >= NewTime;
        if (bCrossed || (bIncludeStart && Notify.Time == OldTime))
        {
            Crossed.Add(Notify);
        }
    }
    Crossed.StableSort(
        [bForward](const FSCAnimNotify& A, const FSCAnimNotify& B)
        {
            return bForward ? A.Time < B.Time : A.Time > B.Time;
        });
    // A snapshot prevents Blueprint edits to the asset from invalidating iteration.
    for (const FSCAnimNotify& Notify : Crossed)
    {
        OnAnimationNotify.Broadcast(Notify.NotifyName);
        if (!IsValid(this) || PlaybackRevision != Revision || AnimSequence != Sequence ||
            PlaybackDuration != Duration || PlayRate != Rate || bLoop != bLooping || TransformSpace != Space)
        {
            return false;
        }
    }
    return true;
}
