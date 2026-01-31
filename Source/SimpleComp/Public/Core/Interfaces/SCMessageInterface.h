#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SCMessageInterface.generated.h"

/**
 * Universal data container for the SimpleComp messaging system.
 */
USTRUCT(BlueprintType)
struct FSCMessagePayload
{
    GENERATED_BODY()

    /** Primary numerical value (e.g., speed, damage, or an ID). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SC Message")
    float Value = 0.0f;

    /** Primary string data (e.g., state name or a specific command). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SC Message")
    FString StringMessage = TEXT("");

    /** Optional reference to the Actor who sent the message (usually the Spawner or its Owner). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SC Message")
    AActor* Sender = nullptr;

    /** Optional reference to a target Actor on the scene. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SC Message")
    AActor* TargetActor = nullptr;
};

UINTERFACE(MinimalAPI, BlueprintType)
class USCMessageInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * ISCMessageInterface: A stable and extensible way for components to talk to each other.
 */
class SIMPLECOMP_API ISCMessageInterface
{
    GENERATED_BODY()

public:
    /**
     * Main event for receiving a universal message.
     * Call via ISCMessageInterface::Execute_OnReceiveSCMessage(Target, Payload) in C++.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SC Message")
    void OnReceiveSCMessage(const FSCMessagePayload& Payload);
};