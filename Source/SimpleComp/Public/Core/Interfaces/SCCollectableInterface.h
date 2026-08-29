#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SCCollectableInterface.generated.h"

class USCStackComponent;

UINTERFACE(MinimalAPI, BlueprintType)
class USCCollectableInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * ISCCollectableInterface
 * Implemented by any Actor that can be collected and placed into a USCStackComponent.
 */
class SIMPLECOMP_API ISCCollectableInterface
{
    GENERATED_BODY()

public:
    /**
     * Called by USCCollectorComponent to initiate the resource's flight toward the stack.
     * Implement the flight logic (e.g., via Timeline) inside the resource Actor.
     * When the flight completes, the implementor must call TargetStack->ConfirmArrival(SlotID)
     * and then destroy itself.
     *
     * @param TargetStack  The Stack Component the resource is flying to.
     * @param SlotID       The reserved slot ID that this resource must confirm upon arrival.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SimpleComp|Collection")
    void InitFlight(USCStackComponent* TargetStack, int32 SlotID);
};
