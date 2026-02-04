#pragma once

#include "Core/SCTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SCAnimSequence.generated.h"

class UCurveBase;
class UCurveFloat;

/**
 * Defines a single curve track within an animation sequence.
 */
USTRUCT(BlueprintType)
struct FSCCurveTrack {
  GENERATED_BODY()

  /** The curve asset to sample from. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
  TObjectPtr<UCurveBase> CurveAsset = nullptr;

  /** What transform property this curve should drive. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
  ESCCurveTrackType TrackType = ESCCurveTrackType::VectorLocation;

  /** If true, the sampled value will be added to the initial transform instead
   * of overriding it. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
  bool bAddBaseValue = true;

  /** Scale factor for float curves. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
  float ScaleFloat = 1.0f;

  /** Scale factor for vector curves. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
  FVector ScaleVector = FVector::OneVector;
};

/**
 * Defines a single notify marker at a specific time.
 */
USTRUCT(BlueprintType)
struct FSCAnimNotify {
  GENERATED_BODY()

  /** Name of the notify event. Each unique name will become a separate output
   * pin in the UK2Node. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
  FName NotifyName;

  /** Time (in seconds, within the animation's duration) when this notify should
   * trigger. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
  float Time = 0.0f;
};

/**
 * DataAsset containing reusable animation data for the SC Curve Animation
 * Component.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Simple Animation Sequence"))
class SIMPLECOMP_API USCAnimSequence : public UDataAsset {
  GENERATED_BODY()

public:
  /** List of tracks driving transformations. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation")
  TArray<FSCCurveTrack> CurveTracks;

  /** List of notify markers that trigger at specific times. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation")
  TArray<FSCAnimNotify> Notifies;

  /** Default duration for this animation. If > 0, the curve playback will be
   * scaled to fit this time. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation",
            meta = (ClampMin = "0.01"))
  float DefaultDuration = 1.0f;

  /** Returns all unique notify names defined in this sequence. */
  UFUNCTION(BlueprintPure, Category = "SimpleComp|Animation")
  TArray<FName> GetNotifyNames() const;
};
