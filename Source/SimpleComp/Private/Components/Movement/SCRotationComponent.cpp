#include "Components/Movement/SCRotationComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"

USCRotationComponent::USCRotationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USCRotationComponent::BeginPlay()
{
    Super::BeginPlay();
    ensure(GetOwner() != nullptr);
    LastLocation = GetComponentLocation();
    bLastLookAtTarget = bLookAtTarget;
}

void USCRotationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (RotationMode == ESCRotationMode::Constant && bLookAtTarget)
    {
        AddLocalRotation(FQuat(RotationRate * DeltaTime));
        return;
    }

    FQuat TargetWorldQuat = GetComponentQuat();
    if (bLookAtTarget != bLastLookAtTarget)
    {
        bIsSwitchingTarget = true;
        bLastLookAtTarget = bLookAtTarget;
    }
    if (bLookAtTarget)
    {
        switch (RotationMode)
        {
            case ESCRotationMode::ToTarget:
                TargetWorldQuat = ComputeTargetQuat();
                break;
            case ESCRotationMode::ToVelocity:
                TargetWorldQuat = ComputeVelocityQuat();
                break;
            case ESCRotationMode::ToForwardDelta:
                TargetWorldQuat = ComputeForwardDeltaQuat();
                break;
            default:
                break;
        }
    }
    else
    {
        if (GetAttachParent())
        {
            TargetWorldQuat = GetAttachParent()->GetComponentQuat();
        }
        else
        {
            TargetWorldQuat = FQuat::Identity;
        }
        LastTargetActor = nullptr;
    }
    FRotator LocalTargetRot = FRotator::ZeroRotator;
    if (GetAttachParent())
    {
        FQuat LocalQuat = GetAttachParent()->GetComponentQuat().Inverse() * TargetWorldQuat;
        LocalTargetRot = LocalQuat.Rotator();
    }
    else
    {
        LocalTargetRot = TargetWorldQuat.Rotator();
    }
    FRotator FinalLocalTargetRot;
    FinalLocalTargetRot.Pitch = ProcessAxis(LocalTargetRot.Pitch, PitchSettings);
    FinalLocalTargetRot.Yaw = ProcessAxis(LocalTargetRot.Yaw, YawSettings);
    FinalLocalTargetRot.Roll = ProcessAxis(LocalTargetRot.Roll, RollSettings);

    const FQuat CurrentQuat = GetRelativeRotation().Quaternion();
    const FQuat TargetQuat = FQuat(FinalLocalTargetRot);
    UpdateTargetSwitching(CurrentQuat, TargetQuat);
    float ActiveSpeed = bIsSwitchingTarget ? SwitchInterpSpeed : DefaultInterpSpeed;
    const FQuat ResultQuat = FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, ActiveSpeed);

    SetRelativeRotation(ResultQuat);
}

FQuat USCRotationComponent::ComputeTargetQuat()
{
    if (TargetActor.IsValid())
    {
        if (TargetActor != LastTargetActor)
        {
            bIsSwitchingTarget = true;
            LastTargetActor = TargetActor;
        }
        FVector Direction = (TargetActor->GetActorLocation() + TargetLocationOffset) - GetComponentLocation();
        return Direction.IsNearlyZero() ? GetComponentQuat() : Direction.ToOrientationQuat();
    }
    return GetComponentQuat();
}

/**
 * Calculates world orientation based on velocity.
 * Prioritizes Parent Component velocity, falling back to Actor velocity if
 * needed.
 */
FQuat USCRotationComponent::ComputeVelocityQuat()
{
    FVector CurrentVelocity = FVector::ZeroVector;

    if (USceneComponent* ParentComp = GetAttachParent())
    {
        CurrentVelocity = ParentComp->GetComponentVelocity();
    }

    if (CurrentVelocity.IsNearlyZero())
    {
        if (AActor* Owner = GetOwner())
        {
            CurrentVelocity = Owner->GetVelocity();
        }
    }

    if (CurrentVelocity.Size() < VelocityThreshold)
    {
        return GetComponentQuat();
    }

    return CurrentVelocity.ToOrientationQuat();
}

FQuat USCRotationComponent::ComputeForwardDeltaQuat()
{
    FVector CurrentLoc = GetComponentLocation();
    FVector Delta = CurrentLoc - LastLocation;

    if (Delta.Size() > MinDistanceThreshold)
    {
        LastLocation = CurrentLoc;
        return Delta.ToOrientationQuat();
    }
    return GetComponentQuat();
}

void USCRotationComponent::UpdateTargetSwitching(const FQuat& CurrentQuat, const FQuat& TargetQuat)
{
    if (bIsSwitchingTarget)
    {
        float AngleDiff = FMath::RadiansToDegrees(CurrentQuat.AngularDistance(TargetQuat));
        if (AngleDiff <= SwitchThreshold)
        {
            bIsSwitchingTarget = false;
        }
    }
}

float USCRotationComponent::ProcessAxis(float TargetAngle, const FSCAxisSettings& Settings)
{
    switch (Settings.Mode)
    {
        case ESCAxisMode::Locked:
            return 0.0f;

        case ESCAxisMode::Limited:
            return FMath::Clamp(FMath::UnwindDegrees(TargetAngle), Settings.Min, Settings.Max);

        case ESCAxisMode::Free:
        default:
            return TargetAngle;
    }
}