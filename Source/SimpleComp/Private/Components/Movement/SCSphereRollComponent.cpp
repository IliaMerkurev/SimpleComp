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
	LastLocation = GetComponentLocation();
	CurrentRotationQuat = GetRelativeRotation().Quaternion();
}

void USCSphereRollComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime <= 0.0f) return;

	const FVector CurrentLocation = GetComponentLocation();
	const FVector MoveDelta = CurrentLocation - LastLocation;

	// Only process if movement is significant to avoid jitter
	if (!MoveDelta.IsNearlyZero(0.01f))
	{
		const float DistanceMoved = MoveDelta.Size();
		const FVector MoveDir = MoveDelta.GetSafeNormal();

		// Calculate rotation axis: perpendicular to move direction and up vector
		FVector RotationAxis = FVector::CrossProduct(FVector::UpVector, MoveDir);

		if (!RotationAxis.IsNearlyZero())
		{
			RotationAxis.Normalize();

			// Calculate rotation angle in radians: theta = distance / radius
			float RotationAngle = DistanceMoved / SphereRadius;
			if (bInvertRotation) RotationAngle *= -1.0f;

			// Create delta rotation quaternion
			FQuat DeltaQuat = FQuat(RotationAxis, RotationAngle);

			// Accumulate rotation (order matters: Delta * Current for world-axis aligned rotation)
			CurrentRotationQuat = DeltaQuat * CurrentRotationQuat;

			SetRelativeRotation(CurrentRotationQuat);
		}
	}

	LastLocation = CurrentLocation;
}