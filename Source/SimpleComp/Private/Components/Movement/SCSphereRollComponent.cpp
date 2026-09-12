#include "Components/Movement/SCSphereRollComponent.h"
#include "GameFramework/Actor.h"

USCSphereRollComponent::USCSphereRollComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    bTickInEditor = false;

    CurrentRotationQuat = FQuat::Identity;
}

void USCSphereRollComponent::BeginPlay()
{
    Super::BeginPlay();
    ensure(GetOwner() != nullptr);
    LastLocation = GetComponentLocation();
    CurrentRotationQuat = GetRelativeRotation().Quaternion();
    InitialRotationQuat = CurrentRotationQuat;
}

void USCSphereRollComponent::ReturnToInitialRotation(const float Speed, const bool bSetRotationActive)
{
    bIsReturningToInitialRotation = true;
    ReturnSpeed = FMath::IsFinite(Speed) ? FMath::Max(0.f, Speed) : 0.f;
    bIsRotationActive = bSetRotationActive;
}

void USCSphereRollComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f)
    {
        return;
    }

    CurrentRotationQuat = GetRelativeRotation().Quaternion();
    const FVector CurrentLocation = GetComponentLocation();
    const FVector MoveDelta = CurrentLocation - LastLocation;

    if (!MoveDelta.IsNearlyZero(0.01f))
    {
        if (bIsRotationActive && FMath::IsFinite(SphereRadius) && SphereRadius > UE_SMALL_NUMBER)
        {
            bIsReturningToInitialRotation = false;

            const float DistanceMoved = MoveDelta.Size();
            const FVector MoveDir = MoveDelta.GetSafeNormal();

            FVector RotationAxis = FVector::CrossProduct(FVector::UpVector, MoveDir);

            if (!RotationAxis.IsNearlyZero())
            {
                RotationAxis.Normalize();

                // The rolling axis is computed in world space; the accumulated quaternion is relative.
                if (GetAttachParent() && !IsUsingAbsoluteRotation())
                {
                    RotationAxis = GetAttachParent()->GetComponentQuat().UnrotateVector(RotationAxis);
                }
                float RotationAngle = FMath::Fmod(static_cast<double>(DistanceMoved) / SphereRadius, 2.0 * PI);
                if (bInvertRotation)
                    RotationAngle *= -1.0f;

                FQuat DeltaQuat = FQuat(RotationAxis, RotationAngle);

                // Accumulate rotation (order matters: Delta * Current for world-axis aligned rotation)
                CurrentRotationQuat = (DeltaQuat * CurrentRotationQuat).GetNormalized();

                SetRelativeRotation(CurrentRotationQuat);
            }
        }
    }
    
    if (bIsReturningToInitialRotation)
    {
        CurrentRotationQuat = FMath::QInterpTo(CurrentRotationQuat, InitialRotationQuat, DeltaTime, ReturnSpeed);
        SetRelativeRotation(CurrentRotationQuat);

        if (CurrentRotationQuat.Equals(InitialRotationQuat, 0.001f))
        {
            bIsReturningToInitialRotation = false;
        }
    }

    LastLocation = CurrentLocation;
}