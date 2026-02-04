#include "UK2Node_PlaySCAnimation.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Components/Animation/SCAnimAsyncAction.h"
#include "Components/Animation/SCAnimSequence.h"
#include "Components/Animation/SCCurveAnimComponent.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Self.h"
#include "K2Node_SwitchName.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KismetCompiler.h"

// Pin name constants
const FName UK2Node_PlaySCAnimation::PN_Play(TEXT("Play"));
const FName UK2Node_PlaySCAnimation::PN_PlayFromStart(TEXT("PlayFromStart"));
const FName UK2Node_PlaySCAnimation::PN_Stop(TEXT("Stop"));
const FName UK2Node_PlaySCAnimation::PN_Pause(TEXT("Pause"));
const FName UK2Node_PlaySCAnimation::PN_Resume(TEXT("Resume"));
const FName UK2Node_PlaySCAnimation::PN_Reverse(TEXT("Reverse"));
const FName
    UK2Node_PlaySCAnimation::PN_ReverseFromCurrent(TEXT("ReverseFromCurrent"));

const FName UK2Node_PlaySCAnimation::PN_Then(TEXT("Then"));
const FName UK2Node_PlaySCAnimation::PN_Update(TEXT("Update"));
const FName UK2Node_PlaySCAnimation::PN_Finished(TEXT("Finished"));
const FName UK2Node_PlaySCAnimation::PN_Component(TEXT("Component"));
const FName UK2Node_PlaySCAnimation::PN_Sequence(TEXT("Sequence"));
const FName UK2Node_PlaySCAnimation::PN_Duration(TEXT("Duration"));
const FName UK2Node_PlaySCAnimation::PN_CurrentTime(TEXT("CurrentTime"));
const FName UK2Node_PlaySCAnimation::PN_NormalizedTime(TEXT("NormalizedTime"));

UK2Node_PlaySCAnimation::UK2Node_PlaySCAnimation() { AnimSequence = nullptr; }

void UK2Node_PlaySCAnimation::AllocateDefaultPins() {
  Super::AllocateDefaultPins();
  const UEdGraphSchema_K2 *K2Schema = GetDefault<UEdGraphSchema_K2>();

  // Input execution pins
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_Play);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_PlayFromStart);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_Stop);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_Pause);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_Resume);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_Reverse);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_ReverseFromCurrent);

  // Component input
  UEdGraphPin *ComponentPin =
      CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object,
                USCCurveAnimComponent::StaticClass(), PN_Component);
  K2Schema->ConstructBasicPinTooltip(
      *ComponentPin,
      NSLOCTEXT("K2Node", "ComponentTooltip",
                "The animation component to play on"),
      ComponentPin->PinToolTip);

  // Sequence override input
  UEdGraphPin *SequencePin =
      CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object,
                USCAnimSequence::StaticClass(), PN_Sequence);
  SequencePin->DefaultObject = AnimSequence;
  K2Schema->ConstructBasicPinTooltip(
      *SequencePin,
      NSLOCTEXT("K2Node", "SequenceTooltip", "Optional sequence override"),
      SequencePin->PinToolTip);

  // Duration override input
  UEdGraphPin *DurationPin =
      CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, PN_Duration);
  DurationPin->PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
  K2Schema->ConstructBasicPinTooltip(
      *DurationPin,
      NSLOCTEXT("K2Node", "DurationTooltip",
                "Optional duration override (0 = use default)"),
      DurationPin->PinToolTip);
  DurationPin->DefaultValue = TEXT("0.0");

  // Output execution pins
  CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_Then);
  CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_Update);
  CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_Finished);

  // Output data pins
  UEdGraphPin *CurrentTimePin =
      CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Real, PN_CurrentTime);
  CurrentTimePin->PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;

  UEdGraphPin *NormalizedTimePin =
      CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Real, PN_NormalizedTime);
  NormalizedTimePin->PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;

  // Create notify pins based on selected sequence
  CreateNotifyPins();
}

void UK2Node_PlaySCAnimation::CreateNotifyPins() {
  if (!AnimSequence)
    return;

  TArray<FName> NotifyNames = AnimSequence->GetNotifyNames();
  for (const FName &NotifyName : NotifyNames) {
    if (!NotifyName.IsNone()) {
      // Create output execution pin for this notify
      UEdGraphPin *NotifyPin =
          CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, NotifyName);
      NotifyPin->PinFriendlyName = FText::FromName(NotifyName);
    }
  }
}

void UK2Node_PlaySCAnimation::RemoveNotifyPins() {
  TArray<UEdGraphPin *> PinsToRemove;

  for (UEdGraphPin *Pin : Pins) {
    // Remove pins that are not standard pins
    if (Pin->Direction == EGPD_Output &&
        Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec &&
        Pin->PinName != PN_Then && Pin->PinName != PN_Update &&
        Pin->PinName != PN_Finished) {
      PinsToRemove.Add(Pin);
    }
  }

  for (UEdGraphPin *Pin : PinsToRemove) {
    Pin->BreakAllPinLinks();
    Pins.Remove(Pin);
  }
}

void UK2Node_PlaySCAnimation::ReconstructNode() {
  // Sync property with pin default object before reconstruction
  if (UEdGraphPin *SequencePin = FindPin(PN_Sequence)) {
    if (SequencePin->DefaultObject) {
      AnimSequence = Cast<USCAnimSequence>(SequencePin->DefaultObject);
    } else if (SequencePin->LinkedTo.Num() == 0) {
      AnimSequence = nullptr;
    }
  }

  Super::ReconstructNode();

  if (UEdGraph *Graph = GetGraph()) {
    Graph->NotifyGraphChanged();
  }
}

void UK2Node_PlaySCAnimation::PinDefaultValueChanged(UEdGraphPin *Pin) {
  Super::PinDefaultValueChanged(Pin);

  if (Pin && Pin->PinName == PN_Sequence) {
    ReconstructNode();
  }
}

void UK2Node_PlaySCAnimation::PinConnectionListChanged(UEdGraphPin *Pin) {
  Super::PinConnectionListChanged(Pin);

  // If the Sequence pin connection changed, update notify pins
  if (Pin && Pin->PinName == PN_Sequence) {
    ReconstructNode();
  }
}

#if WITH_EDITOR
void UK2Node_PlaySCAnimation::PostEditChangeProperty(
    FPropertyChangedEvent &PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
                                 ? PropertyChangedEvent.Property->GetFName()
                                 : NAME_None;

  if (PropertyName ==
      GET_MEMBER_NAME_CHECKED(UK2Node_PlaySCAnimation, AnimSequence)) {
    ReconstructNode();
  }
}
#endif

FText UK2Node_PlaySCAnimation::GetNodeTitle(
    ENodeTitleType::Type TitleType) const {
  return NSLOCTEXT("K2Node", "PlaySCAnimation_Title", "Play SC Animation");
}

FText UK2Node_PlaySCAnimation::GetTooltipText() const {
  return NSLOCTEXT("K2Node", "PlaySCAnimation_Tooltip",
                   "Plays an animation on an SC Curve Animation Component with "
                   "dynamic notify outputs");
}

FLinearColor UK2Node_PlaySCAnimation::GetNodeTitleColor() const {
  return FLinearColor(0.8f, 0.4f, 0.0f);
}

FSlateIcon
UK2Node_PlaySCAnimation::GetIconAndTint(FLinearColor &OutColor) const {
  OutColor = GetNodeTitleColor();
  static FSlateIcon Icon("EditorStyle", "GraphEditor.Timeline_16x");
  return Icon;
}

void UK2Node_PlaySCAnimation::GetMenuActions(
    FBlueprintActionDatabaseRegistrar &ActionRegistrar) const {
  UClass *ActionKey = GetClass();
  if (ActionRegistrar.IsOpenForRegistration(ActionKey)) {
    UBlueprintNodeSpawner *NodeSpawner =
        UBlueprintNodeSpawner::Create(GetClass());
    check(NodeSpawner != nullptr);

    ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
  }
}

FText UK2Node_PlaySCAnimation::GetMenuCategory() const {
  return NSLOCTEXT("K2Node", "PlaySCAnimation_Category",
                   "Simple Comp|Animation");
}

void UK2Node_PlaySCAnimation::ExpandNode(
    class FKismetCompilerContext &CompilerContext, UEdGraph *SourceGraph) {
  Super::ExpandNode(CompilerContext, SourceGraph);

  const UEdGraphSchema_K2 *K2Schema = GetDefault<UEdGraphSchema_K2>();

  // 1. Spawn Proxy Creation Node
  UK2Node_CallFunction *CreateProxyNode =
      CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this,
                                                                  SourceGraph);
  CreateProxyNode->FunctionReference.SetExternalMember(
      GET_FUNCTION_NAME_CHECKED(USCAnimAsyncAction, CreateProxy),
      USCAnimAsyncAction::StaticClass());
  CreateProxyNode->AllocateDefaultPins();

  // Wire Proxy Inputs
  CompilerContext.CopyPinLinksToIntermediate(
      *FindPinChecked(PN_Component),
      *CreateProxyNode->FindPinChecked(TEXT("Component")));
  CompilerContext.CopyPinLinksToIntermediate(
      *FindPinChecked(PN_Sequence),
      *CreateProxyNode->FindPinChecked(TEXT("Sequence")));
  CompilerContext.CopyPinLinksToIntermediate(
      *FindPinChecked(PN_Duration),
      *CreateProxyNode->FindPinChecked(TEXT("Duration")));

  UEdGraphPin *ProxyResultPin = CreateProxyNode->GetReturnValuePin();

  // 2. Wrap Play Controls via Proxy
  auto CreateProxyCall = [&](const FName &PinName, const FName &ProxyFuncName,
                             bool bIsPlayFunc = false, bool bArg1 = false) {
    UEdGraphPin *ExecIn = FindPinChecked(PinName);
    if (ExecIn->LinkedTo.Num() == 0)
      return (UK2Node_CallFunction *)nullptr;

    UK2Node_CallFunction *CallNode =
        CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(
            this, SourceGraph);
    CallNode->FunctionReference.SetExternalMember(
        ProxyFuncName, USCAnimAsyncAction::StaticClass());
    CallNode->AllocateDefaultPins();

    CallNode->FindPinChecked(UEdGraphSchema_K2::PN_Self)
        ->MakeLinkTo(ProxyResultPin);
    CompilerContext.MovePinLinksToIntermediate(*ExecIn,
                                               *CallNode->GetExecPin());

    if (bIsPlayFunc) {
      CallNode->FindPinChecked(TEXT("bFromStart"))->DefaultValue =
          bArg1 ? TEXT("true") : TEXT("false");
    }

    return CallNode;
  };

  // Wire Entry Pins
  CreateProxyCall(PN_Play, GET_FUNCTION_NAME_CHECKED(USCAnimAsyncAction, Play),
                  true, false);
  CreateProxyCall(PN_PlayFromStart,
                  GET_FUNCTION_NAME_CHECKED(USCAnimAsyncAction, Play), true,
                  true);
  CreateProxyCall(PN_Stop, GET_FUNCTION_NAME_CHECKED(USCAnimAsyncAction, Stop));
  CreateProxyCall(PN_Pause,
                  GET_FUNCTION_NAME_CHECKED(USCAnimAsyncAction, Pause));
  CreateProxyCall(PN_Resume,
                  GET_FUNCTION_NAME_CHECKED(USCAnimAsyncAction, Resume));
  CreateProxyCall(PN_Reverse,
                  GET_FUNCTION_NAME_CHECKED(USCAnimAsyncAction, Reverse), true,
                  false);
  CreateProxyCall(
      PN_ReverseFromCurrent,
      GET_FUNCTION_NAME_CHECKED(USCAnimAsyncAction, ReverseFromCurrent));

  // 3. Activate Proxy
  UK2Node_CallFunction *ActivateNode =
      CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this,
                                                                  SourceGraph);
  ActivateNode->FunctionReference.SetExternalMember(
      GET_FUNCTION_NAME_CHECKED(USCAnimAsyncAction, Activate),
      USCAnimAsyncAction::StaticClass());
  ActivateNode->AllocateDefaultPins();
  ActivateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self)
      ->MakeLinkTo(ProxyResultPin);

  // Trigger Activate immediately after Creation
  CreateProxyNode->GetThenPin()->MakeLinkTo(ActivateNode->GetExecPin());

  // 4. Handle Output Delegates (Update, Finished, Notifies)
  auto BindOutput = [&](const FName &DelegateName) {
    // Spawn Custom Event
    UK2Node_CustomEvent *EventNode =
        CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this,
                                                                   SourceGraph);
    EventNode->CustomFunctionName =
        *FString::Printf(TEXT("On_%s_%s"), *DelegateName.ToString(),
                         *FGuid::NewGuid().ToString());
    EventNode->AllocateDefaultPins();

    // Add parameters to the Custom Event to match FSCAnimProxyOutputPin
    EventNode->CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Name,
                         TEXT("NotifyName"));
    UEdGraphPin *CTPin = EventNode->CreatePin(
        EGPD_Output, UEdGraphSchema_K2::PC_Real, TEXT("CurrentTime"));
    CTPin->PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;

    UEdGraphPin *NTPin = EventNode->CreatePin(
        EGPD_Output, UEdGraphSchema_K2::PC_Real, TEXT("NormalizedTime"));
    NTPin->PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;

    // Spawn AddDelegate
    UK2Node_AddDelegate *AddDelegateNode =
        CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(this,
                                                                   SourceGraph);
    AddDelegateNode->DelegateReference.SetExternalMember(
        DelegateName, USCAnimAsyncAction::StaticClass());
    AddDelegateNode->AllocateDefaultPins();

    // Wire AddDelegate
    AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self)
        ->MakeLinkTo(ProxyResultPin);
    AddDelegateNode->GetDelegatePin()->MakeLinkTo(
        EventNode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName));

    return TPair<UK2Node_CustomEvent *, UK2Node_AddDelegate *>(EventNode,
                                                               AddDelegateNode);
  };

  auto UpdateBinding = BindOutput(TEXT("Update"));
  auto FinishedBinding = BindOutput(TEXT("Finished"));
  auto NotifyBinding = BindOutput(TEXT("OnNotify"));

  // Chain bindings to Activate node
  ActivateNode->GetThenPin()->MakeLinkTo(UpdateBinding.Value->GetExecPin());
  UpdateBinding.Value->GetThenPin()->MakeLinkTo(
      FinishedBinding.Value->GetExecPin());
  FinishedBinding.Value->GetThenPin()->MakeLinkTo(
      NotifyBinding.Value->GetExecPin());

  // Chain Then output from the end of binding chain
  CompilerContext.MovePinLinksToIntermediate(
      *FindPinChecked(PN_Then), *NotifyBinding.Value->GetThenPin());

  // Wire Output Data
  CompilerContext.MovePinLinksToIntermediate(
      *FindPinChecked(PN_CurrentTime),
      *UpdateBinding.Key->FindPinChecked(TEXT("CurrentTime")));
  CompilerContext.MovePinLinksToIntermediate(
      *FindPinChecked(PN_NormalizedTime),
      *UpdateBinding.Key->FindPinChecked(TEXT("NormalizedTime")));

  // Wire Output Exec
  CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(PN_Update),
                                             *UpdateBinding.Key->GetThenPin());
  CompilerContext.MovePinLinksToIntermediate(
      *FindPinChecked(PN_Finished), *FinishedBinding.Key->GetThenPin());

  // 5. Dynamic Notify Dispatcher
  if (AnimSequence) {
    UK2Node_SwitchName *SwitchNode =
        CompilerContext.SpawnIntermediateNode<UK2Node_SwitchName>(this,
                                                                  SourceGraph);
    SwitchNode->AllocateDefaultPins();

    NotifyBinding.Key->GetThenPin()->MakeLinkTo(SwitchNode->GetExecPin());
    SwitchNode->FindPinChecked(TEXT("Selection"))
        ->MakeLinkTo(NotifyBinding.Key->FindPinChecked(TEXT("NotifyName")));

    TArray<FName> NotifyNames = AnimSequence->GetNotifyNames();
    for (const FName &Name : NotifyNames) {
      if (Name.IsNone())
        continue;
      SwitchNode->AddPinToSwitchNode();
      UEdGraphPin *CasePin = SwitchNode->Pins.Last();
      CasePin->PinName = Name;
      CasePin->PinFriendlyName = FText::FromName(Name);

      if (UEdGraphPin *NodeNotifyPin = FindPin(Name)) {
        CompilerContext.MovePinLinksToIntermediate(*NodeNotifyPin, *CasePin);
      }
    }
  }

  // Break all our pins
  BreakAllNodeLinks();
}
