#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SCAnimAsyncAction.generated.h"

class USCCurveAnimComponent;
class USCAnimSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSCAnimProxyOutputPin, FName,
                                               NotifyName, double, CurrentTime,
                                               double, NormalizedTime);

/**
 * Proxy object that handles delegate binding and state for the
 * UK2Node_PlaySCAnimation.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Simple Animation Async Action"))
class SIMPLECOMP_API USCAnimAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()

public:
  virtual void Activate() override;

  /** Internal factory for UK2Node */
  // Use double for Blueprint interface (UE5 "Real" / Green Pins)
  UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
  static USCAnimAsyncAction *CreateProxy(USCCurveAnimComponent *Component,
                                         USCAnimSequence *Sequence,
                                         double Duration, bool bLoop);

  /** Play control functions called by UK2Node */
  UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
  void Play(bool bFromStart);

  UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
  void Stop();

  UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
  void Pause();

  UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
  void Resume();

  UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
  void ReverseFromEnd(bool bFromStart);

  UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
  void ReverseFromCurrent();

  /** Output pins */
  UPROPERTY(BlueprintAssignable)
  FSCAnimProxyOutputPin Update;

  UPROPERTY(BlueprintAssignable)
  FSCAnimProxyOutputPin Finished;

  /** Map of notify name to delegate. UK2Node will bind to these. */
  // Note: Since we can't have dynamic pin counts in a single delegate property
  // easily for a proxy, we use a single 'OnNotify' delegate that UK2Node will
  // use with a dispatcher or separate bindings.
  UPROPERTY(BlueprintAssignable)
  FSCAnimProxyOutputPin OnNotify;

public:
  // Internal usage: Loop state
  bool TargetLoop = false;

private:
  UFUNCTION()
  void HandleUpdate(float CurrentTime, float NormalizedTime);

  UFUNCTION()
  void HandleFinished();

  UFUNCTION()
  void HandleNotify(FName NotifyName);

  void Cleanup();

  UPROPERTY()
  TObjectPtr<USCCurveAnimComponent> TargetComponent;

  UPROPERTY()
  TObjectPtr<USCAnimSequence> TargetSequence;

  float TargetDuration;
};
