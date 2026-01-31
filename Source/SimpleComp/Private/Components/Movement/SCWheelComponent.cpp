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
}

void USCWheelComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner || DeltaTime <= 0.0f) return;

	const FVector CurrentLocation = GetComponentLocation();
	const FVector MoveDelta = CurrentLocation - LastLocation;

	if (!MoveDelta.IsNearlyZero(0.01f))
	{
		const float DistanceMoved = MoveDelta.Size();
		const FVector MoveDir = MoveDelta.GetSafeNormal();
		const FVector VehicleForward = Owner->GetActorForwardVector();
		const float DirectionValue = FVector::DotProduct(MoveDir, VehicleForward);
		const bool bIsReversing = (DirectionValue < 0.0f);

		// Roll Logic
		float RotationDirection = bIsReversing ? 1.0f : -1.0f;
		if (bInvertRoll) RotationDirection *= -1.0f;

		const float RotationAngle = (DistanceMoved / WheelRadius) * (180.0f / PI) * RotationDirection;
		CurrentRollRotation = FMath::Fmod(CurrentRollRotation + RotationAngle, 360.0f);

		// Steering Logic
		if (bEnableSteering)
		{
			const FVector LocalMoveDir = Owner->GetTransform().InverseTransformVectorNoScale(MoveDir);
			float TargetAngleDeg = FMath::Atan2(LocalMoveDir.Y, FMath::Abs(LocalMoveDir.X)) * (180.0f / PI);
			if (bIsReversing) TargetAngleDeg *= -1.0f;

			const float ClampedTargetYaw = FMath::Clamp(TargetAngleDeg * SteerMultiplier, -MaxSteerAngle, MaxSteerAngle);
			CurrentSteerYaw = FMath::FInterpTo(CurrentSteerYaw, ClampedTargetYaw, DeltaTime, SteerSpeed);
		}
		else
		{
			CurrentSteerYaw = FMath::FInterpTo(CurrentSteerYaw, 0.0f, DeltaTime, SteerSpeed);
		}

		// Apply Final Rotation
		FQuat SteerQuat = FQuat(FVector::UpVector, FMath::DegreesToRadians(CurrentSteerYaw));
		FQuat RollQuat = FQuat(FVector::RightVector, FMath::DegreesToRadians(-CurrentRollRotation));
		SetRelativeRotation(SteerQuat * RollQuat);
	}
	LastLocation = CurrentLocation;
}