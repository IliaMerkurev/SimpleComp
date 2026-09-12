#pragma once

#include "Core/SCTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SCAnimSequence.generated.h"

class UCurveBase;
class UCurveFloat;
class UCurveTable;

/**
 * Defines a single curve track within an animation sequence.
 */
USTRUCT(BlueprintType)
    struct FSCCurveTrack
    {
        GENERATED_BODY()

        /** Float curve for a single-axis track. Vector curve assets are not evaluated. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Track")
        TObjectPtr<UCurveBase> CurveAsset = nullptr;

        /** CurveTable asset. Used when TrackType is TableLocation, TableRotation, or TableScale.
         * The table must contain rows named X, Y, and Z (each a float curve). */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Track")
        TObjectPtr<UCurveTable> CurveTableAsset = nullptr;

        /** What transform property this curve should drive. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Track")
        ESCCurveTrackType TrackType = ESCCurveTrackType::TableLocation;

        /** If true, the sampled value will be added to the initial transform instead
         * of overriding it. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Track")
        bool bAddBaseValue = true;

        /** Uniform scale factor applied to the sampled value. Works for both single Curve and CurveTable tracks. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Track")
        float ScaleCurve = 1.0f;

        /** Per-axis scale factor for CurveTable tracks. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Track")
        FVector ScaleVector = FVector::OneVector;
    };

    /**
     * Defines a single notify marker at a specific time.
     */
    USTRUCT(BlueprintType)
        struct FSCAnimNotify
        {
            GENERATED_BODY()

            /** Name of the notify event. Each unique name will become a separate output
             * pin in the UK2Node. */
            UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Notify")
            FName NotifyName;

            /** Time (in seconds, within the animation's duration) when this notify should
             * trigger. */
            UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Notify")
            float Time = 0.0f;
        };

        /**
         * DataAsset containing reusable animation data for the SC Curve Animation
         * Component.
         */
        UCLASS(BlueprintType, meta = (DisplayName = "Simple Animation Sequence"))
            class SIMPLECOMP_API USCAnimSequence : public UDataAsset
            {
                GENERATED_BODY()

            public:
                /** List of tracks driving transformations. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation")
                TArray<FSCCurveTrack> CurveTracks;

                /** List of notify markers that trigger at specific times. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation")
                TArray<FSCAnimNotify> Notifies;

                /** Sequence sample-time extent in seconds. PlaybackDuration retimes this extent; non-positive values infer it from curve ranges. */
                UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation",
                    meta = (ClampMin = "0.01"))
                float DefaultDuration = 1.0f;

                /** Returns all unique notify names defined in this sequence. */
                UFUNCTION(BlueprintPure, Category = "SimpleComp|Animation")
                TArray<FName> GetNotifyNames() const;
            };
