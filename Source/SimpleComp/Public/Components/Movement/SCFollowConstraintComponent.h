#pragma once

#include "Components/ActorComponent.h"
#include "Core/SCTypes.h"
#include "CoreMinimal.h"
#include "SCFollowConstraintComponent.generated.h"

/**
 * USCFollowConstraintComponent: Constrains the owner actor to stay within a
 * specified distance of a target actor. It also provides smooth rotation toward
 * the direction of movement.
 *
 * Featuring per-axis control for both Location constraint and Rotation
 * behavior.
 */
UCLASS(ClassGroup = (SimpleComp),
    meta = (BlueprintSpawnableComponent, DisplayName = "Simple Follow Constraint Component"))
    class SIMPLECOMP_API USCFollowConstraintComponent : public UActorComponent
    {
        GENERATED_BODY()

    public:
        USCFollowConstraintComponent();

        // --- Core Settings ---

        /** The actor to follow and maintain distance from. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|CORE")
        TObjectPtr<AActor> FollowTarget;

        /** Maximum allowed distance from the FollowTarget before the owner starts
         * following. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|CORE", Interp,
            meta = (ClampMin = "0.0", Units = "cm"))
        float RopeLength = 500.0f;

        /** How smoothly the actor rotates toward its movement direction (0 = instant,
         * higher = faster convergence). */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|CORE", Interp, meta = (ClampMin = "0.0"))
        float RotationSmoothness = 8.0f;

        // --- Axis Control (Location) ---

        /** X world-axis offset from the target: Free follows the rope, Locked preserves owner position, Limited clamps to Min/Max. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Control (Location)")
        FSCAxisSettings XAxisSettings;

        /** Y world-axis offset from the target; Limited clamps to Min/Max in centimeters. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Control (Location)")
        FSCAxisSettings YAxisSettings;

        /** Z world-axis offset from the target. Locked prevents vertical following; Limited clamps to Min/Max. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Control (Location)")
        FSCAxisSettings ZAxisSettings;

        // --- Axis Control (Rotation) ---

        /** Settings for Pitch rotation. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Control (Rotation)")
        FSCAxisSettings PitchSettings;

        /** Settings for Yaw rotation. Usually Free for trailers/carts. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Control (Rotation)")
        FSCAxisSettings YawSettings;

        /** Settings for Roll rotation. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Axis Control (Rotation)")
        FSCAxisSettings RollSettings;

    protected:
        virtual void BeginPlay() override;
        virtual void TickComponent(float DeltaTime, ELevelTick TickType,
            FActorComponentTickFunction* ThisTickFunction) override;

    private:
        /** Helper to process a single axis based on settings. */
        double ProcessAxis(double CurrentVal, double TargetVal, const FSCAxisSettings& Settings);

        /** Tracks the location from the previous frame to calculate movement delta
         * for rotation. */
        FVector LastLocation;
    };
