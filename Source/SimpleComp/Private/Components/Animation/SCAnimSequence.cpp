#include "Components/Animation/SCAnimSequence.h"

TArray<FName> USCAnimSequence::GetNotifyNames() const {
  TSet<FName> UniqueNames;
  for (const FSCAnimNotify &Notify : Notifies) {
    if (!Notify.NotifyName.IsNone()) {
      UniqueNames.Add(Notify.NotifyName);
    }
  }
  return UniqueNames.Array();
}
