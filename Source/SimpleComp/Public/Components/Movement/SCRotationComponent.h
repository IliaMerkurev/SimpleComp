#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SCRotationComponent.generated.h"

/**
 * Defines the logic used to calculate the target rotation vector.
 */
UENUM(BlueprintType)
enum class ESCRotationMode : uint8
{
    /** Traditional look-at logic targeting an Actor or Location. */
    ToTarget        UMETA(DisplayName = "Rotate to Target"),
    /** Orients the component toward the Owner's current Velocity vector. */
    ToVelocity      UMETA(DisplayName = "Rotate to Velocity"),
    /** Orients toward the movement direction (CurrentPos - LastPos). Ideal for Lerp/Spline movement. */
    ToForwardDelta  UMETA(DisplayName = "Rotate to Forward Delta"),
    /** Continuous local rotation at a fixed rate (e.g., for propellers or idle spin). */
    Constant        UMETA(DisplayName = "Constant Rotation")
};

/**
 * Defines constraint behavior for a specific rotation axis.
 */
UENUM(BlueprintType)
enum class ESCAxisMode : uint8
{
    /** Component logic fully controls this axis. */
    Free            UMETA(DisplayName = "Free"),
    /** Axis is clamped between Min and Max angles. */
    Limited         UMETA(DisplayName = "Limited"),
    /** Axis is locked to 0 in local space, effectively aligning it with the parent. */
    Locked          UMETA(DisplayName = "Locked")
};

/**
 * Settings for individual axis constraints and limits.
 */
USTRUCT(BlueprintType)
struct FSCRotationAxisSettings
{
    GENERATED_BODY()

    /** How this axis should behave. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    ESCAxisMode Mode = ESCAxisMode::Free;

    /** Minimum allowed angle (used only in Limited mode). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "Mode == ESCAxisMode::Limited"))
    float MinAngle = -90.0f;

    /** Maximum allowed angle (used only in Limited mode). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "Mode == ESCAxisMode::Limited"))
    float MaxAngle = 90.0f;
};

/**
 * SCRotationComponent: A professional-grade SceneComponent for handling complex rotations.
 * Features: Target tracking, Velocity alignment, Forward Delta tracking, and Axis Constraints.
 * Optimized for VR (Quest 3) using Quaternion math to prevent Gimbal Lock.
 */
UCLASS(ClassGroup=(SimpleComp), meta=(BlueprintSpawnableComponent))
class SIMPLECOMP_API USCRotationComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    USCRotationComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // --- General Settings ---

    /** Master toggle. When false, the component smoothly returns to local zero rotation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|General", Interp)
    bool bLookAtTarget = true;

    /** Selection of the primary rotation calculation mode. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|General", Interp)
    ESCRotationMode RotationMode = ESCRotationMode::ToTarget;

    /** Standard interpolation speed. Used for steady tracking. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|General", Interp, meta = (ClampMin = "0.0"))
    float DefaultInterpSpeed = 15.0f;

    /** Faster interpolation speed used when switching targets or states. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|General", Interp, meta = (ClampMin = "0.0"))
    float SwitchInterpSpeed = 5.0f;

    /** Precision threshold (degrees) to stop using SwitchInterpSpeed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|General", meta = (ClampMin = "0.0"))
    float SwitchThreshold = 1.0f;

    // --- Axis Control ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Control")
    FSCRotationAxisSettings PitchSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Control")
    FSCRotationAxisSettings YawSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Control")
    FSCRotationAxisSettings RollSettings;

    // --- Mode Specific Settings ---

    /** The target actor to look at (ToTarget mode only). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Target", meta = (EditCondition = "RotationMode == ESCRotationMode::ToTarget", EditConditionHides), Interp)
    TWeakObjectPtr<AActor> TargetActor;

    /** Offset added to TargetActor position or used as static world location if actor is null. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Target", meta = (EditCondition = "RotationMode == ESCRotationMode::ToTarget", EditConditionHides), Interp)
    FVector TargetLocationOffset;

    /** Degrees per second to rotate in Constant mode (Applied in Local Space). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Constant", meta = (EditCondition = "RotationMode == ESCRotationMode::Constant", EditConditionHides, ForceUnits = "deg/s"), Interp)
    FRotator RotationRate = FRotator(0.f, 90.f, 0.f);

    /** Minimum movement distance (cm) to trigger rotation update in Forward Delta mode. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Forward Delta", meta = (EditCondition = "RotationMode == ESCRotationMode::ToForwardDelta", EditConditionHides, ForceUnits = "cm"))
    float MinDistanceThreshold = 0.1f;

    /** Minimum velocity magnitude required to update rotation. Prevents jittering when nearly stationary. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Velocity", meta = (EditCondition = "RotationMode == ESCRotationMode::ToVelocity", EditConditionHides), Interp)
    float VelocityThreshold = 10.0f;

private:
    /** Clamps or locks an axis based on user settings. */
    float ProcessAxis(float TargetAngle, const FSCRotationAxisSettings& Settings);
    
    /** Returns world orientation toward a target or offset. */
    FQuat ComputeTargetQuat();
    
    /** Returns world orientation toward the owner's velocity. */
    FQuat ComputeVelocityQuat();
    
    /** Returns world orientation toward current movement direction. */
    FQuat ComputeForwardDeltaQuat();

    /** Manages the bIsSwitchingTarget state based on angular distance. */
    void UpdateTargetSwitching(const FQuat& CurrentQuat, const FQuat& TargetQuat);

    /** Location stored from previous frame for delta calculation. */
    FVector LastLocation;
    
    /** Tracking variable to detect when the target actor changes. */
    TWeakObjectPtr<AActor> LastTargetActor;
    
    /** True if we are in the process of rapid rotation to a new target. */
    bool bIsSwitchingTarget = false;
    
    /** Tracking variable for the bLookAtTarget toggle. */
    bool bLastLookAtTarget = true;
};