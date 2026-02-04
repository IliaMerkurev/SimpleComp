#pragma once

#include "Components/SceneComponent.h"
#include "Core/SCTypes.h"
#include "SCCurveAnimComponent.generated.h"

class USCAnimSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCAnimFinishedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCAnimNotifySignature, FName,
                                            NotifyName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSCAnimUpdateSignature, float,
                                             CurrentTime, float,
                                             NormalizedTime);

/**
 * A component that drives its parent's or its own transform using Curves
 * defined in a DataAsset. lightweight alternative to Timelines with reusable
 * animation data.
 */
UCLASS(ClassGroup = (SimpleComp), meta = (BlueprintSpawnableComponent))
class SIMPLECOMP_API USCCurveAnimComponent : public USceneComponent {
  GENERATED_BODY()

public:
  USCCurveAnimComponent();

  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  virtual void BeginPlay() override;

  /** The animation sequence to play. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
  USCAnimSequence *AnimSequence;

  /**
   * Total duration of playback in seconds.
   * If 0, uses the DefaultDuration from the AnimSequence.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
  float PlaybackDuration = 0.0f;

  /** Playback speed multiplier. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
  float PlayRate = 1.0f;

  /** Should the animation restart automatically when it ends? */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
  bool bLoop = false;

  /** Should the animation start playing on BeginPlay? */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
  bool bAutoPlay = true;

  /** In which space to apply the transformations. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
  ESCTransformSpace TransformSpace = ESCTransformSpace::Local;

  /** Called when the animation finishes (only if not looping). */
  UPROPERTY(BlueprintAssignable, Category = "Animation")
  FSCAnimFinishedSignature OnAnimationFinished;

  /** Called when a notify is triggered at its specified time. */
  UPROPERTY(BlueprintAssignable, Category = "Animation")
  FSCAnimNotifySignature OnAnimationNotify;

  /** Called every frame while the animation is playing. */
  UPROPERTY(BlueprintAssignable, Category = "Animation")
  FSCAnimUpdateSignature OnAnimationUpdate;

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void Play();

  /**
   * Enhanced play function for use by internal systems and Async Nodes.
   * @param Sequence The animation sequence to play. If null, uses the
   * component's assigned sequence.
   * @param Duration Override for playback duration. If 0, uses asset default.
   * @param bFromStart If true, resets playback to the beginning (or end if
   * reverse).
   * @param bInReverse If true, plays the animation backwards.
   */
  UFUNCTION(BlueprintCallable, Category = "Animation")
  void PlayEx(USCAnimSequence *Sequence = nullptr, float Duration = 0.0f,
              bool bFromStart = true, bool bInReverse = false);

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void PlayFromStart();

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void Stop();

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void Pause();

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void Resume();

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void Reverse();

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void ReverseFromCurrent();

  UFUNCTION(BlueprintCallable, Category = "Animation")
  float GetPlaybackPosition() const { return CurrentTime; }

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void SetPlaybackPosition(float NewTime);

  UFUNCTION(BlueprintCallable, Category = "Animation")
  bool IsPlaying() const { return bIsPlaying; }

private:
  void UpdateAnimation(float DeltaTime);
  void ApplyTransform();
  void ProcessNotifies(float OldTime, float NewTime);
  float GetEffectiveDuration() const;

  bool bIsPlaying = false;
  bool bIsPaused = false;
  float CurrentTime = 0.0f;

  // Captured at start or when Play() is called
  FVector InitialLocation;
  FRotator InitialRotation;
  FVector InitialScale;

  bool bReverse = false;

  // Track which notifies have already fired to ensure they only fire once
  TSet<int32> FiredNotifyIndices;
};
