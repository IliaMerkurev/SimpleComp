#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SCWheelComponent.generated.h"

/**
 * USCWheelComponent
 * Procedural wheel component that handles rotation and steering based on movement delta.
 */
UCLASS(ClassGroup=(SimpleComp), meta=(BlueprintSpawnableComponent))
class SIMPLECOMP_API USCWheelComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	USCWheelComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Radius of the wheel in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Wheel Settings")
	float WheelRadius = 30.0f;

	/** Enables automatic steering (Yaw) based on the movement direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Wheel Settings")
	bool bEnableSteering = false;

	/** Maximum steering angle in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Wheel Settings", meta = (EditCondition = "bEnableSteering"))
	float MaxSteerAngle = 45.0f;

	/** How fast the wheel interpolates to the target steering angle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Wheel Settings", meta = (EditCondition = "bEnableSteering"))
	float SteerSpeed = 5.0f;

	/** Multiplier for the visual steering effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Wheel Settings", meta = (EditCondition = "bEnableSteering"))
	float SteerMultiplier = 1.0f;

	/** Inverts the roll direction if the mesh is oriented backwards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Wheel Settings")
	bool bInvertRoll = false;

private:
	FVector LastLocation;
	float CurrentRollRotation = 0.0f;
	float CurrentSteerYaw = 0.0f;
};