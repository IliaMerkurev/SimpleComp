#include "Components/Movement/SCRotationComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Actor.h" 

USCRotationComponent::USCRotationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USCRotationComponent::BeginPlay()
{
    Super::BeginPlay();
    LastLocation = GetComponentLocation();
    bLastLookAtTarget = bLookAtTarget;
}

void USCRotationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Constant rotation: Standard local additive rotation
    if (RotationMode == ESCRotationMode::Constant && bLookAtTarget)
    {
        AddLocalRotation(FQuat(RotationRate * DeltaTime));
        return;
    }

    FQuat TargetWorldQuat = GetComponentQuat();

    // Toggle detection for bLookAtTarget to trigger Switch speed
    if (bLookAtTarget != bLastLookAtTarget)
    {
        bIsSwitchingTarget = true;
        bLastLookAtTarget = bLookAtTarget;
    }

    // Step 1: Calculate Target World Orientation
    if (bLookAtTarget)
    {
        switch (RotationMode)
        {
        case ESCRotationMode::ToTarget:       TargetWorldQuat = ComputeTargetQuat(); break;
        case ESCRotationMode::ToVelocity:     TargetWorldQuat = ComputeVelocityQuat(); break;
        case ESCRotationMode::ToForwardDelta: TargetWorldQuat = ComputeForwardDeltaQuat(); break;
        default: break;
        }
    }
    else
    {
        // Smooth return to zero: Target becomes parent orientation (Identity in local space)
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

    // Step 2: Convert World Target to Local Space for constraints
    FRotator LocalTargetRot = FRotator::ZeroRotator;
    if (GetAttachParent())
    {
        // Equivalent to NormalizedDeltaRotator but using Quat math for stability
        FQuat LocalQuat = GetAttachParent()->GetComponentQuat().Inverse() * TargetWorldQuat;
        LocalTargetRot = LocalQuat.Rotator();
    }
    else
    {
        LocalTargetRot = TargetWorldQuat.Rotator();
    }

    // Step 3: Apply individual axis logic (Locked/Limited/Free)
    FRotator FinalLocalTargetRot;
    FinalLocalTargetRot.Pitch = ProcessAxis(LocalTargetRot.Pitch, PitchSettings);
    FinalLocalTargetRot.Yaw = ProcessAxis(LocalTargetRot.Yaw, YawSettings);
    FinalLocalTargetRot.Roll = ProcessAxis(LocalTargetRot.Roll, RollSettings);

    const FQuat CurrentQuat = GetRelativeRotation().Quaternion();
    const FQuat TargetQuat = FQuat(FinalLocalTargetRot);

    // Step 4: Update Switching State
    UpdateTargetSwitching(CurrentQuat, TargetQuat);

    // Step 5: Smooth Interpolation via QInterpTo (Slerp)
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
 * Prioritizes Parent Component velocity, falling back to Actor velocity if needed.
 */
FQuat USCRotationComponent::ComputeVelocityQuat()
{
    FVector CurrentVelocity = FVector::ZeroVector;

    // 1. Try to get velocity from the parent component first (for nested motion)
    if (USceneComponent* ParentComp = GetAttachParent())
    {
        CurrentVelocity = ParentComp->GetComponentVelocity();
    }

    // 2. Fallback to Owner Actor velocity if parent is static or missing
    if (CurrentVelocity.IsNearlyZero())
    {
        if (AActor* Owner = GetOwner())
        {
            CurrentVelocity = Owner->GetVelocity();
        }
    }

    // 3. Apply Velocity Threshold check to prevent jittering at low speeds
    if (CurrentVelocity.Size() < VelocityThreshold)
    {
        // Return current rotation to stop updating when moving too slowly
        return GetComponentQuat();
    }

    // 4. Return target orientation quat
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

float USCRotationComponent::ProcessAxis(float TargetAngle, const FSCRotationAxisSettings& Settings)
{
    switch (Settings.Mode)
    {
    case ESCAxisMode::Locked:
        return 0.0f;

    case ESCAxisMode::Limited:
        return FMath::Clamp(FMath::UnwindDegrees(TargetAngle), Settings.MinAngle, Settings.MaxAngle);

    case ESCAxisMode::Free:
    default:
        return TargetAngle;
    }
}