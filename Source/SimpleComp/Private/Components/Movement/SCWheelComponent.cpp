#include "Components/Movement/SCWheelComponent.h"
#include "GameFramework/Actor.h"

USCWheelComponent::USCWheelComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    bTickInEditor = false;
}

void USCWheelComponent::BeginPlay()
{
    Super::BeginPlay();
    LastLocation = GetComponentLocation();
    InitialRotation = GetRelativeRotation().Quaternion();
}

void USCWheelComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* Owner = GetOwner();
    if (!IsValid(Owner) || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f)
    {
        return;
    }

    const FVector CurrentLocation = GetComponentLocation();
    const FVector MoveDelta = CurrentLocation - LastLocation;

    if (!MoveDelta.IsNearlyZero(0.01f))
    {
        const float DistanceMoved = MoveDelta.Size();
        const FVector MoveDir = MoveDelta.GetSafeNormal();
        const FVector VehicleForward = Owner->GetActorForwardVector();
        const float DirectionValue = FVector::DotProduct(MoveDir, VehicleForward);
        const bool bIsReversing = (DirectionValue < 0.0f);

        float RotationDirection = bIsReversing ? 1.0f : -1.0f;
        if (bInvertRoll)
            RotationDirection *= -1.0f;

        if (FMath::IsFinite(WheelRadius) && WheelRadius > UE_SMALL_NUMBER)
        {
            const double RotationAngle =
                (static_cast<double>(DistanceMoved) / WheelRadius) * (180.0 / PI) * RotationDirection;
            CurrentRollRotation = FMath::Fmod(CurrentRollRotation + RotationAngle, 360.0);
        }

        if (bEnableSteering)
        {
            const FVector LocalMoveDir = Owner->GetTransform().InverseTransformVectorNoScale(MoveDir);
            float TargetAngleDeg = FMath::Atan2(LocalMoveDir.Y, FMath::Abs(LocalMoveDir.X)) * (180.0f / PI);
            if (bIsReversing)
                TargetAngleDeg *= -1.0f;

            TargetSteerYaw =
                FMath::Clamp(TargetAngleDeg * SteerMultiplier, -FMath::Abs(MaxSteerAngle), FMath::Abs(MaxSteerAngle));
        }
    }
    if (!bEnableSteering)
    {
        TargetSteerYaw = 0.f;
    }
    CurrentSteerYaw = FMath::FInterpTo(CurrentSteerYaw, TargetSteerYaw, DeltaTime, SteerSpeed);
    const FQuat SteerQuat(FVector::UpVector, FMath::DegreesToRadians(CurrentSteerYaw));
    const FQuat RollQuat(FVector::RightVector, FMath::DegreesToRadians(-CurrentRollRotation));
    SetRelativeRotation(SteerQuat * InitialRotation * RollQuat);
    LastLocation = CurrentLocation;
}
