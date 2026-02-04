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

  UPROPERTY(BlueprintAssignable, Category = "Animation")
  FSCAnimFinishedSignature OnAnimationFinished;

  UPROPERTY(BlueprintAssignable, Category = "Animation")
  FSCAnimNotifySignature OnAnimationNotify;

  UPROPERTY(BlueprintAssignable, Category = "Animation")
  FSCAnimUpdateSignature OnAnimationUpdate;

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void Play();

  UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
  void PlayEx(USCAnimSequence *Sequence, float Duration = -1.0f,
              bool bFromStart = true, bool bReverse = false);

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void PlayFromStart();

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void Stop();

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void Pause();

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void Resume();

  UFUNCTION(BlueprintCallable, Category = "Animation")
  void ReverseFromEnd();

  UFUNCTION(BlueprintCallable, Category = "Animation")
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
  float PlaybackCurrentTime = 0.0f;

  FVector InitialLocation;
  FRotator InitialRotation;
  FVector InitialScale;

  bool bReversePlayback = false;

  TSet<int32> FiredNotifyIndices;
};
