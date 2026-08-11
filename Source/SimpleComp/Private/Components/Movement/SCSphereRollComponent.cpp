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
    ReturnSpeed = Speed;
    bIsRotationActive = bSetRotationActive;
}

void USCSphereRollComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (DeltaTime <= 0.0f)
        return;

    const FVector CurrentLocation = GetComponentLocation();
    const FVector MoveDelta = CurrentLocation - LastLocation;

    if (!MoveDelta.IsNearlyZero(0.01f))
    {
        if (bIsRotationActive)
        {
            bIsReturningToInitialRotation = false;

            const float DistanceMoved = MoveDelta.Size();
            const FVector MoveDir = MoveDelta.GetSafeNormal();

            FVector RotationAxis = FVector::CrossProduct(FVector::UpVector, MoveDir);

            if (!RotationAxis.IsNearlyZero())
            {
                RotationAxis.Normalize();

                float RotationAngle = DistanceMoved / SphereRadius;
                if (bInvertRotation)
                    RotationAngle *= -1.0f;

                FQuat DeltaQuat = FQuat(RotationAxis, RotationAngle);

                // Accumulate rotation (order matters: Delta * Current for world-axis aligned rotation)
                CurrentRotationQuat = DeltaQuat * CurrentRotationQuat;

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