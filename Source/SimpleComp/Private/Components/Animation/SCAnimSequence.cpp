#include "Components/Animation/SCAnimSequence.h"

TArray<FName> USCAnimSequence::GetNotifyNames() const
{
    TArray<FName> UniqueNames;
    UniqueNames.Reserve(Notifies.Num());
    for (const FSCAnimNotify& Notify : Notifies)
    {
        if (!Notify.NotifyName.IsNone())
        {
            UniqueNames.AddUnique(Notify.NotifyName);
        }
    }
    return UniqueNames;
}
