#include "UK2Node_PlaySCAnimation.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Components/Animation/SCAnimAsyncAction.h"
#include "Components/Animation/SCAnimSequence.h"
#include "Components/Animation/SCCurveAnimComponent.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_Self.h"
#include "K2Node_SwitchName.h"
#include "K2Node_TemporaryVariable.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KismetCompiler.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "UK2Node_PlaySCAnimation"

// Pin name constants
const FName UK2Node_PlaySCAnimation::PN_Play(TEXT("Play"));
const FName UK2Node_PlaySCAnimation::PN_PlayFromStart(TEXT("PlayFromStart"));
const FName UK2Node_PlaySCAnimation::PN_Stop(TEXT("Stop"));
const FName UK2Node_PlaySCAnimation::PN_Pause(TEXT("Pause"));
const FName UK2Node_PlaySCAnimation::PN_Resume(TEXT("Resume"));
const FName UK2Node_PlaySCAnimation::PN_ReverseFromEnd(TEXT("ReverseFromEnd"));
const FName UK2Node_PlaySCAnimation::PN_Loop(TEXT("Loop"));
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

  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_Play);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_PlayFromStart);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_Stop);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_Pause);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_Resume);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_ReverseFromEnd);
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_ReverseFromCurrent);

  UEdGraphPin *ComponentPin =
      CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object,
                USCCurveAnimComponent::StaticClass(), PN_Component);
  K2Schema->ConstructBasicPinTooltip(
      *ComponentPin,
      NSLOCTEXT("K2Node", "ComponentTooltip",
                "The animation component to play on"),
      ComponentPin->PinToolTip);

  UEdGraphPin *SequencePin =
      CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object,
                USCAnimSequence::StaticClass(), PN_Sequence);
  SequencePin->DefaultObject = AnimSequence;
  K2Schema->ConstructBasicPinTooltip(
      *SequencePin,
      NSLOCTEXT("K2Node", "SequenceTooltip", "Optional sequence override"),
      SequencePin->PinToolTip);

  UEdGraphPin *DurationPin =
      CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, PN_Duration);
  K2Schema->ConstructBasicPinTooltip(
      *DurationPin,
      NSLOCTEXT("K2Node", "DurationTooltip",
                "Optional duration override (0 = use default)"),
      DurationPin->PinToolTip);
  DurationPin->DefaultValue = TEXT("1.0");

  UEdGraphPin *LoopPin =
      CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, PN_Loop);
  LoopPin->DefaultValue = TEXT("false");
  K2Schema->ConstructBasicPinTooltip(
      *LoopPin,
      NSLOCTEXT("K2Node", "LoopTooltip", "Whether to loop the animation"),
      LoopPin->PinToolTip);

  CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_Then);
  CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_Update);
  CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_Finished);

  UEdGraphPin *CurrentTimePin =
      CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Real, PN_CurrentTime);
  UEdGraphPin *NormalizedTimePin =
      CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Real, PN_NormalizedTime);

  CreateNotifyPins();
}

void UK2Node_PlaySCAnimation::CreateNotifyPins() {
  if (!AnimSequence)
    return;

  TArray<FName> NotifyNames = AnimSequence->GetNotifyNames();
  for (const FName &NotifyName : NotifyNames) {
    if (!NotifyName.IsNone()) {
      UEdGraphPin *NotifyPin =
          CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, NotifyName);
      NotifyPin->PinFriendlyName = FText::FromName(NotifyName);
    }
  }
}

void UK2Node_PlaySCAnimation::RemoveNotifyPins() {
  TArray<UEdGraphPin *> PinsToRemove;

  for (UEdGraphPin *Pin : Pins) {
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
  if (UEdGraphPin *SequencePin = FindPin(PN_Sequence)) {
    if (SequencePin->DefaultObject) {
      AnimSequence = Cast<USCAnimSequence>(SequencePin->DefaultObject);
    } else if (SequencePin->LinkedTo.Num() == 0) {
      AnimSequence = nullptr;
    }
  }

  Super::ReconstructNode();
}

void UK2Node_PlaySCAnimation::PreloadRequiredAssets() {
  if (AnimSequence) {
    PreloadObject(AnimSequence);
    // Ensure pins are reconstructed after force-loading the asset
    ReconstructNode();
  }
  Super::PreloadRequiredAssets();
}

void UK2Node_PlaySCAnimation::PinDefaultValueChanged(UEdGraphPin *Pin) {
  Super::PinDefaultValueChanged(Pin);

  if (Pin && Pin->PinName == PN_Sequence) {
    ReconstructNode();
  }
}

void UK2Node_PlaySCAnimation::PinConnectionListChanged(UEdGraphPin *Pin) {
  Super::PinConnectionListChanged(Pin);

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
  return NSLOCTEXT("K2Node", "PlaySCAnimation_Title", "Simple Play Animation");
}

FText UK2Node_PlaySCAnimation::GetTooltipText() const {
  return NSLOCTEXT("K2Node", "PlaySCAnimation_Tooltip",
                   "Plays an animation on an SC Curve Animation Component with "
                   "dynamic notify outputs");
}

void UK2Node_PlaySCAnimation::ExpandNode(
    class FKismetCompilerContext &CompilerContext, UEdGraph *SourceGraph) {
  Super::ExpandNode(CompilerContext, SourceGraph);

  const UEdGraphSchema_K2 *K2Schema = GetDefault<UEdGraphSchema_K2>();

  // Helper to copy links from the node's input pin to an internal node's pin
  auto CopyInput = [&](const FName &NodePinName, UEdGraphNode *DestNode,
                       const FName &DestPinName) {
    UEdGraphPin *NodePin = FindPin(NodePinName);
    UEdGraphPin *DestPin = DestNode ? DestNode->FindPin(DestPinName) : nullptr;
    if (NodePin && DestPin) {
      CompilerContext.CopyPinLinksToIntermediate(*NodePin, *DestPin);
      DestPin->DefaultObject = NodePin->DefaultObject;
      DestPin->DefaultTextValue = NodePin->DefaultTextValue;

      // Removed: Explicitly setting DefaultValue from path causes validation
      // errors on Object pins. K2Node_CallFunction will use DefaultObject
      // correctly.
      if (!NodePin->DefaultObject) {
        DestPin->DefaultValue = NodePin->DefaultValue;
      }
    }
  };

  // Helper to move links from the node's output pin to an internal node's pin
  auto MoveOutput = [&](const FName &NodePinName, UEdGraphNode *SourceNode,
                        const FName &SourcePinName) {
    UEdGraphPin *NodePin = FindPin(NodePinName);
    UEdGraphPin *SourcePin =
        SourceNode ? SourceNode->FindPin(SourcePinName) : nullptr;
    if (NodePin && SourcePin) {
      CompilerContext.MovePinLinksToIntermediate(*NodePin, *SourcePin);
    }
  };

  struct FActionPath {
    FName ExecPinName;
    FName FuncName;
    bool bIsControl;
    bool bSetFromStart;
    bool bValueFromStart;
  };

  const TArray<FActionPath> Paths = {
      {PN_Play, TEXT("Play"), false, true, false},
      {PN_PlayFromStart, TEXT("Play"), false, true, true},
      {PN_Stop, TEXT("Stop"), true, false, false},
      {PN_Pause, TEXT("Pause"), true, false, false},
      {PN_Resume, TEXT("Resume"), true, false, false},
      {PN_ReverseFromEnd, TEXT("ReverseFromEnd"), false, true, true},
      {PN_ReverseFromCurrent, TEXT("ReverseFromCurrent"), false, false, false}};

  auto ConnectInternalOutputToUserOutput = [&](UEdGraphPin *InternalOut,
                                               const FName &NodeOutputName) {
    if (!InternalOut)
      return;
    UEdGraphPin *NodeOut = FindPin(NodeOutputName);
    if (NodeOut) {
      TArray<UEdGraphPin *> Linked = NodeOut->LinkedTo;
      for (UEdGraphPin *Remote : Linked) {
        InternalOut->MakeLinkTo(Remote);
      }
    }
  };

  for (const FActionPath &Path : Paths) {
    UEdGraphPin *ExecInput = FindPin(Path.ExecPinName);
    if (!ExecInput || ExecInput->LinkedTo.Num() == 0) {
      continue;
    }

    UK2Node_CallFunction *CreateProxyNode =
        CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(
            this, SourceGraph);
    CreateProxyNode->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(USCAnimAsyncAction, CreateProxy),
        USCAnimAsyncAction::StaticClass());
    CreateProxyNode->AllocateDefaultPins();

    CopyInput(PN_Component, CreateProxyNode, TEXT("Component"));
    CopyInput(PN_Sequence, CreateProxyNode, TEXT("Sequence"));
    CopyInput(PN_Duration, CreateProxyNode, TEXT("Duration"));
    CopyInput(PN_Loop, CreateProxyNode, TEXT("bLoop"));

    CompilerContext.MovePinLinksToIntermediate(*ExecInput,
                                               *CreateProxyNode->GetExecPin());

    UEdGraphPin *ProxyPin = CreateProxyNode->GetReturnValuePin();

    UEdGraphNode *LastNode = CreateProxyNode;

    if (!Path.bIsControl) {
      auto SetupDelegate = [&](const FName &DelegateName,
                               const FName &NodeOutName,
                               const FName &ParamName = NAME_None,
                               const FName &NodeParamName = NAME_None,
                               const FName &ParamName2 = NAME_None,
                               const FName &NodeParamName2 = NAME_None) {
        UK2Node_AddDelegate *AddDel =
            CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(
                this, SourceGraph);
        AddDel->SetFromProperty(
            USCAnimAsyncAction::StaticClass()->FindPropertyByName(DelegateName),
            false, USCAnimAsyncAction::StaticClass());
        AddDel->AllocateDefaultPins();

        if (UEdGraphPin *Self = AddDel->FindPin(UEdGraphSchema_K2::PN_Self)) {
          Self->MakeLinkTo(ProxyPin);
        }

        UK2Node_CustomEvent *Evt =
            CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(
                this, SourceGraph);
        Evt->CustomFunctionName = *FString::Printf(
            TEXT("%s_%s_%s"), *GetName(), *DelegateName.ToString(),
            *FGuid::NewGuid().ToString());

        if (FMulticastDelegateProperty *Prop =
                CastField<FMulticastDelegateProperty>(
                    USCAnimAsyncAction::StaticClass()->FindPropertyByName(
                        DelegateName))) {
          Evt->SetDelegateSignature(Prop->SignatureFunction);
        }
        Evt->AllocateDefaultPins();

        if (UEdGraphPin *DelPin = AddDel->GetDelegatePin()) {
          DelPin->MakeLinkTo(
              Evt->FindPin(UK2Node_CustomEvent::DelegateOutputName));
        }

        LastNode->FindPin(UEdGraphSchema_K2::PN_Then)
            ->MakeLinkTo(AddDel->GetExecPin());
        LastNode = AddDel;

        ConnectInternalOutputToUserOutput(
            Evt->FindPin(UEdGraphSchema_K2::PN_Then), NodeOutName);

        if (!ParamName.IsNone() && !NodeParamName.IsNone()) {
          ConnectInternalOutputToUserOutput(Evt->FindPin(ParamName),
                                            NodeParamName);
        }
        if (!ParamName2.IsNone() && !NodeParamName2.IsNone()) {
          ConnectInternalOutputToUserOutput(Evt->FindPin(ParamName2),
                                            NodeParamName2);
        }

        return Evt;
      };

      SetupDelegate(TEXT("Update"), PN_Update, TEXT("CurrentTime"),
                    PN_CurrentTime, TEXT("NormalizedTime"), PN_NormalizedTime);
      SetupDelegate(TEXT("Finished"), PN_Finished);

      UK2Node_AddDelegate *AddDelNotify =
          CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(
              this, SourceGraph);
      AddDelNotify->SetFromProperty(
          USCAnimAsyncAction::StaticClass()->FindPropertyByName(
              TEXT("OnNotify")),
          false, USCAnimAsyncAction::StaticClass());
      AddDelNotify->AllocateDefaultPins();
      if (UEdGraphPin *Self =
              AddDelNotify->FindPin(UEdGraphSchema_K2::PN_Self)) {
        Self->MakeLinkTo(ProxyPin);
      }

      UK2Node_CustomEvent *EvtNotify =
          CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(
              this, SourceGraph);
      EvtNotify->CustomFunctionName = *FString::Printf(
          TEXT("%s_OnNotify_%s"), *GetName(), *FGuid::NewGuid().ToString());
      if (FMulticastDelegateProperty *Prop =
              CastField<FMulticastDelegateProperty>(
                  USCAnimAsyncAction::StaticClass()->FindPropertyByName(
                      TEXT("OnNotify")))) {
        EvtNotify->SetDelegateSignature(Prop->SignatureFunction);
      }
      EvtNotify->AllocateDefaultPins();
      if (UEdGraphPin *DelPin = AddDelNotify->GetDelegatePin()) {
        DelPin->MakeLinkTo(
            EvtNotify->FindPin(UK2Node_CustomEvent::DelegateOutputName));
      }

      LastNode->FindPin(UEdGraphSchema_K2::PN_Then)
          ->MakeLinkTo(AddDelNotify->GetExecPin());
      LastNode = AddDelNotify;

      if (AnimSequence) {
        UK2Node_SwitchName *Switch =
            CompilerContext.SpawnIntermediateNode<UK2Node_SwitchName>(
                this, SourceGraph);
        Switch->AllocateDefaultPins();
        EvtNotify->FindPin(UEdGraphSchema_K2::PN_Then)
            ->MakeLinkTo(Switch->GetExecPin());
        if (UEdGraphPin *NamePin = EvtNotify->FindPin(TEXT("NotifyName"))) {
          NamePin->MakeLinkTo(Switch->GetSelectionPin());
        }

        for (const FSCAnimNotify &Notify : AnimSequence->Notifies) {
          Switch->AddPinToSwitchNode();
          UEdGraphPin *SwPin = Switch->Pins.Last();
          // Ensure PinName matches the NotifyName for the Switch to route
          // correctly
          SwPin->PinName = Notify.NotifyName;
          SwPin->PinFriendlyName = FText::FromName(Notify.NotifyName);

          ConnectInternalOutputToUserOutput(SwPin, Notify.NotifyName);
        }
      }

      UK2Node_CallFunction *ActivateNode =
          CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(
              this, SourceGraph);
      ActivateNode->FunctionReference.SetExternalMember(
          GET_FUNCTION_NAME_CHECKED(UBlueprintAsyncActionBase, Activate),
          UBlueprintAsyncActionBase::StaticClass());
      ActivateNode->AllocateDefaultPins();
      if (UEdGraphPin *Self =
              ActivateNode->FindPin(UEdGraphSchema_K2::PN_Self)) {
        Self->MakeLinkTo(ProxyPin);
      }
      LastNode->FindPin(UEdGraphSchema_K2::PN_Then)
          ->MakeLinkTo(ActivateNode->GetExecPin());
      LastNode = ActivateNode;
    }

    UK2Node_CallFunction *CallFuncNode =
        CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(
            this, SourceGraph);
    CallFuncNode->FunctionReference.SetExternalMember(
        Path.FuncName, USCAnimAsyncAction::StaticClass());
    CallFuncNode->AllocateDefaultPins();
    if (UEdGraphPin *Self = CallFuncNode->FindPin(UEdGraphSchema_K2::PN_Self)) {
      Self->MakeLinkTo(ProxyPin);
    }

    if (Path.bSetFromStart) {
      if (UEdGraphPin *Pin = CallFuncNode->FindPin(TEXT("bFromStart"))) {
        Pin->DefaultValue = Path.bValueFromStart ? TEXT("true") : TEXT("false");
      }
    }

    LastNode->FindPin(UEdGraphSchema_K2::PN_Then)
        ->MakeLinkTo(CallFuncNode->GetExecPin());

    ConnectInternalOutputToUserOutput(CallFuncNode->GetThenPin(), PN_Then);
  }

  BreakAllNodeLinks();
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

#undef LOCTEXT_NAMESPACE
