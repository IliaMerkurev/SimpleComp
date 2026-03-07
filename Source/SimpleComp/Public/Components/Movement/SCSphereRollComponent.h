#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "SCSphereRollComponent.generated.h"

/**
 * USCSphereRollComponent
 * Procedural rolling component for spherical objects (rocks, balls, etc.).
 * Calculates rotation based on movement delta and surface contact.
 */
UCLASS(ClassGroup = (SimpleComp), meta = (BlueprintSpawnableComponent, DisplayName = "Simple Sphere Roll Component"))
    class SIMPLECOMP_API USCSphereRollComponent : public USceneComponent
    {
        GENERATED_BODY()

    public:
        USCSphereRollComponent();

    protected:
        virtual void BeginPlay() override;

    public:
        virtual void TickComponent(float DeltaTime, ELevelTick TickType,
            FActorComponentTickFunction* ThisTickFunction) override;

        /** Radius of the sphere in centimeters. Used to calculate rotation angle. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Sphere Settings", Interp)
        float SphereRadius = 50.0f;

        /** Inverts the rotation direction. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Sphere Settings", Interp)
        bool bInvertRotation = false;

    private:
        FVector LastLocation;
        FQuat CurrentRotationQuat;
    };