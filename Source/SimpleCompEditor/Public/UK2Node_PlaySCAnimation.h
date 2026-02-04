#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "UK2Node_PlaySCAnimation.generated.h"

class USCAnimSequence;
class USCCurveAnimComponent;

/**
 * Custom Blueprint node that plays an SC Animation with dynamic output pins
 * for each notify in the selected animation sequence.
 */
UCLASS(meta = (Keywords = "Play SC Animation Simple Animation",
               DisplayName = "Simple Play Animation"))
class SIMPLECOMPEDITOR_API UK2Node_PlaySCAnimation : public UK2Node {
  GENERATED_BODY()

public:
  UK2Node_PlaySCAnimation();

  // UEdGraphNode interface
  virtual void AllocateDefaultPins() override;
  virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
  virtual FText GetTooltipText() const override;
  virtual FLinearColor GetNodeTitleColor() const override;
  virtual FSlateIcon GetIconAndTint(FLinearColor &OutColor) const override;
  // End of UEdGraphNode interface

  // UK2Node interface
  virtual void GetMenuActions(
      FBlueprintActionDatabaseRegistrar &ActionRegistrar) const override;
  virtual FText GetMenuCategory() const override;
  virtual void ExpandNode(class FKismetCompilerContext &CompilerContext,
                          UEdGraph *SourceGraph) override;
  virtual void ReconstructNode() override;
  virtual void PinConnectionListChanged(UEdGraphPin *Pin) override;
  virtual void PinDefaultValueChanged(UEdGraphPin *Pin) override;
#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif
  // End of UK2Node interface

  /** The animation sequence asset to use for generating dynamic notification
   * pins. */
  UPROPERTY(EditAnywhere, Category = "SimpleComp|Animation")
  TObjectPtr<USCAnimSequence> AnimSequence;

private:
  static const FName PN_Play;
  static const FName PN_PlayFromStart;
  static const FName PN_Stop;
  static const FName PN_Pause;
  static const FName PN_Resume;
  static const FName PN_ReverseFromEnd;
  static const FName PN_ReverseFromCurrent;

  // Input Data Pins
  static const FName PN_Component;
  static const FName PN_Sequence;
  static const FName PN_Duration;

  // Output Pins
  static const FName PN_Then;
  static const FName PN_Update;
  static const FName PN_Finished;
  static const FName PN_CurrentTime;
  static const FName PN_NormalizedTime;

  void CreateNotifyPins();
  void RemoveNotifyPins();
};
