#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SCAnimAsyncAction.generated.h"

class USCCurveAnimComponent;
class USCAnimSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSCAnimProxyOutputPin, FName,
                                               NotifyName, float, CurrentTime,
                                               float, NormalizedTime);

/**
 * Proxy object that handles delegate binding and state for the
 * UK2Node_PlaySCAnimation.
 */
UCLASS()
class SIMPLECOMP_API USCAnimAsyncAction : public UBlueprintAsyncActionBase {
  GENERATED_BODY()

public:
  virtual void Activate() override;

  /** Internal factory for UK2Node */
  static USCAnimAsyncAction *CreateProxy(USCCurveAnimComponent *Component,
                                         USCAnimSequence *Sequence,
                                         float Duration);

  /** Play control functions called by UK2Node */
  void Play(bool bFromStart);
  void Stop();
  void Pause();
  void Resume();
  void Reverse(bool bFromStart);
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

private:
  UFUNCTION()
  void HandleUpdate(float CurrentTime, float NormalizedTime);

  UFUNCTION()
  void HandleFinished();

  UFUNCTION()
  void HandleNotify(FName NotifyName);

  void Cleanup();

  UPROPERTY()
  USCCurveAnimComponent *TargetComponent;

  UPROPERTY()
  USCAnimSequence *TargetSequence;

  float TargetDuration;
};
