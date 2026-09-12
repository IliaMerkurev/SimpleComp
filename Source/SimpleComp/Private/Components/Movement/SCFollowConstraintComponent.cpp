#include "Components/Movement/SCFollowConstraintComponent.h"
#include "GameFramework/Actor.h"

USCFollowConstraintComponent::USCFollowConstraintComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

    // Default to XY behavior (Z Locked)
    ZAxisSettings.Mode = ESCAxisMode::Locked;

    // Default to Yaw-only rotation
    PitchSettings.Mode = ESCAxisMode::Locked;
    RollSettings.Mode = ESCAxisMode::Locked;
}

void USCFollowConstraintComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        LastLocation = Owner->GetActorLocation();
    }
}

void USCFollowConstraintComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* Owner = GetOwner();
    if (!IsValid(FollowTarget) || !IsValid(Owner) || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f)
    {
        return;
    }
    const FVector TargetLocation = FollowTarget->GetActorLocation();
    const FVector CurrentLocation = Owner->GetActorLocation();
    FVector Offset = CurrentLocation - TargetLocation;
    if (XAxisSettings.Mode == ESCAxisMode::Locked)
    {
        Offset.X = 0.0;
    }
    if (YAxisSettings.Mode == ESCAxisMode::Locked)
    {
        Offset.Y = 0.0;
    }
    if (ZAxisSettings.Mode == ESCAxisMode::Locked)
    {
        Offset.Z = 0.0;
    }
    const double SafeLength = FMath::IsFinite(RopeLength) ? FMath::Max(0.f, RopeLength) : 0.f;
    Offset = Offset.GetClampedToMaxSize(SafeLength);
    // Location limits constrain world-axis offsets from the target. Explicit axis limits take
    // precedence if the configured interval has no intersection with the rope sphere.
    const FVector NewLocation(
        TargetLocation.X + ProcessAxis(CurrentLocation.X - TargetLocation.X, Offset.X, XAxisSettings),
        TargetLocation.Y + ProcessAxis(CurrentLocation.Y - TargetLocation.Y, Offset.Y, YAxisSettings),
        TargetLocation.Z + ProcessAxis(CurrentLocation.Z - TargetLocation.Z, Offset.Z, ZAxisSettings));
    if (!NewLocation.Equals(CurrentLocation))
    {
        Owner->SetActorLocation(NewLocation);
        if (!IsValid(this) || !IsValid(Owner))
        {
            return;
        }
        const FVector Movement = Owner->GetActorLocation() - LastLocation;
        if (!Movement.IsNearlyZero())
        {
            const FRotator Current = Owner->GetActorRotation();
            const FRotator Target = Movement.Rotation();
            const FRotator Result(ProcessAxis(Current.Pitch, Target.Pitch, PitchSettings),
                                  ProcessAxis(Current.Yaw, Target.Yaw, YawSettings),
                                  ProcessAxis(Current.Roll, Target.Roll, RollSettings));
            const float Alpha = !FMath::IsFinite(RotationSmoothness) || RotationSmoothness <= 0.f
                                    ? 1.f
                                    : FMath::Clamp(DeltaTime * RotationSmoothness, 0.f, 1.f);
            Owner->SetActorRotation(FQuat::Slerp(Current.Quaternion(), Result.Quaternion(), Alpha));
        }
    }
    if (IsValid(Owner))
    {
        LastLocation = Owner->GetActorLocation();
    }
}

double USCFollowConstraintComponent::ProcessAxis(double CurrentVal, double TargetVal, const FSCAxisSettings& Settings)
{
    if (Settings.Mode == ESCAxisMode::Locked)
    {
        return CurrentVal;
    }
    if (Settings.Mode == ESCAxisMode::Limited)
    {
        return FMath::Clamp(TargetVal, static_cast<double>(FMath::Min(Settings.Min, Settings.Max)),
                            static_cast<double>(FMath::Max(Settings.Min, Settings.Max)));
    }
    return TargetVal;
}
