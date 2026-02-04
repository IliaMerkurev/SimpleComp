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

UCLASS(ClassGroup = (SimpleComp),
       meta = (BlueprintSpawnableComponent,
               DisplayName = "Simple Curve Animation Component"))
class SIMPLECOMP_API USCCurveAnimComponent : public USceneComponent {
  GENERATED_BODY()

public:
  USCCurveAnimComponent();

  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  virtual void BeginPlay() override;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation")
  TObjectPtr<USCAnimSequence> AnimSequence;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation",
            meta = (ClampMin = "0.01"))
  float PlaybackDuration = 1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation")
  float PlayRate = 1.0f;

  UPROPERTY(BlueprintReadOnly, Category = "SimpleComp|Animation")
  float CurrentTime = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation")
  bool bLoop = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation")
  bool bAutoPlay = true;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation")
  ESCTransformSpace TransformSpace = ESCTransformSpace::Local;

  UPROPERTY(BlueprintAssignable, Category = "SimpleComp|Animation")
  FSCAnimFinishedSignature OnAnimationFinished;

  UPROPERTY(BlueprintAssignable, Category = "SimpleComp|Animation")
  FSCAnimNotifySignature OnAnimationNotify;

  UPROPERTY(BlueprintAssignable, Category = "SimpleComp|Animation")
  FSCAnimUpdateSignature OnAnimationUpdate;

  UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
  void Play();

  /**
   * Plays the animation with extended options.
   * @param Sequence Optional sequence to play. If null, plays current.
   * @param Duration Duration to play for. -1 uses sequence duration.
   * @param bFromStart Whether to restart from beginning.
   * @param bReverse Whether to play in reverse.
   * @param bInLoop Whether to loop the animation.
   */
  UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
  void PlayEx(USCAnimSequence *Sequence, float Duration = -1.0f,
              bool bFromStart = true, bool bReverse = false,
              bool bInLoop = false);

  UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
  void PlayFromStart();

  UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
  void Stop();

  UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
  void Pause();

  UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
  void Resume();

  UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
  void ReverseFromEnd();

  UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
  void ReverseFromCurrent();

  UFUNCTION(BlueprintPure, Category = "SimpleComp|Animation")
  float GetPlaybackPosition() const { return CurrentTime; }

  void SetPlaybackPosition(float NewTime);

  UFUNCTION(BlueprintPure, Category = "SimpleComp|Animation")
  bool IsPlaying() const { return bIsPlaying; }

private:
  void UpdateAnimation(float DeltaTime);
  void ApplyTransform();
  void ApplyTransform(float SampleTime);
  void ProcessNotifies(float OldTime, float NewTime);
  float GetEffectiveDuration() const;

  bool bIsPlaying = false;
  bool bIsPaused = false;
  bool bFinished = false; // Added missing state variable
  float PlaybackCurrentTime = 0.0f;

  FVector InitialLocation;
  FRotator InitialRotation;
  FVector InitialScale;

  bool bReversePlayback = false;

  TSet<int32> FiredNotifyIndices;
};
