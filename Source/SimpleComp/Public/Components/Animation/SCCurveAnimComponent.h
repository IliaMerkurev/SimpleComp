#pragma once

#include "Components/SceneComponent.h"
#include "Core/SCTypes.h"
#include "SCCurveAnimComponent.generated.h"

class USCAnimSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCAnimFinishedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCAnimNotifySignature, FName, NotifyName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSCAnimUpdateSignature, float, CurrentTime, float, NormalizedTime);

UCLASS(ClassGroup = (SimpleComp),
    meta = (BlueprintSpawnableComponent, DisplayName = "Simple Curve Animation Component"))
    class SIMPLECOMP_API USCCurveAnimComponent : public USceneComponent
    {
        GENERATED_BODY()

    public:
        USCCurveAnimComponent();

        virtual void TickComponent(float DeltaTime, ELevelTick TickType,
            FActorComponentTickFunction* ThisTickFunction) override;

        virtual void BeginPlay() override;

        /** The animation sequence to play. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleComp|Animation")
        TObjectPtr<USCAnimSequence> AnimSequence;

        /** Total duration of the playback in seconds. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Animation",
            meta = (ClampMin = "0.01"))
        float PlaybackDuration = 1.0f;

        /** Multiplier for playback speed. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Animation")
        float PlayRate = 1.0f;

        /** Current playback position in seconds. */
        UPROPERTY(BlueprintReadOnly, Category = "SimpleComp|Animation")
        float CurrentTime = 0.0f;

        /** Whether the animation should loop when it reaches the end. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Animation")
        bool bLoop = false;

        /** Whether to start playing automatically on BeginPlay. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Animation")
        bool bAutoPlay = true;

        /** Whether to apply transforms in Local or World space. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "SimpleComp|Animation")
        ESCTransformSpace TransformSpace = ESCTransformSpace::Local;

        /** Called when playback finishes (not called when looping). */
        UPROPERTY(BlueprintAssignable, Category = "SimpleComp|Animation")
        FSCAnimFinishedSignature OnAnimationFinished;

        /** Called when a notify marker is triggered. */
        UPROPERTY(BlueprintAssignable, Category = "SimpleComp|Animation")
        FSCAnimNotifySignature OnAnimationNotify;

        /** Called every frame during playback with current time and normalized time.
         */
        UPROPERTY(BlueprintAssignable, Category = "SimpleComp|Animation")
        FSCAnimUpdateSignature OnAnimationUpdate;

        /** Starts playback with current settings. */
        UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
        void Play();

        /**
         * Plays the animation with extended options.
         * @param Sequence Optional sequence to play. If null, plays current.
         * @param Duration Duration to play for. -1 uses sequence duration.
         * @param bFromStart Whether to restart from beginning.
         * @param bReverse Whether to play in reverse.
         * @param bInLoop Whether to loop the animation.
         */
        UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
        void PlayEx(USCAnimSequence* Sequence, float Duration = -1.0f, bool bFromStart = true, bool bReverse = false,
            bool bInLoop = false);

        /** Restarts playback from the beginning. */
        UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
        void PlayFromStart();

        /** Stops playback / resets state. */
        UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
        void Stop();

        /** Pauses playback at current time. */
        UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
        void Pause();

        /** Resumes playback from paused state. */
        UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
        void Resume();

        /** Plays in reverse from the end. */
        UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
        void ReverseFromEnd();

        /** Toggles reverse playback from current position. */
        UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
        void ReverseFromCurrent();

        /** Returns current playback position. */
        UFUNCTION(BlueprintPure, Category = "SimpleComp|Animation")
        float GetPlaybackPosition() const
        {
            return CurrentTime;
        }

        /** Jumps to specific playback position. */
        UFUNCTION(BlueprintCallable, Category = "SimpleComp|Animation")
        void SetPlaybackPosition(float NewTime);

        /** Returns true if currently playing. */
        UFUNCTION(BlueprintPure, Category = "SimpleComp|Animation")
        bool IsPlaying() const
        {
            return bIsPlaying;
        }

    private:
        void UpdateAnimation(float DeltaTime);
        void ApplyTransform();
        void ApplyTransform(float SampleTime);
        void ProcessNotifies(float OldTime, float NewTime);
        float GetEffectiveDuration() const;

        bool bIsPlaying = false;
        bool bIsPaused = false;
        bool bFinished = false;
        float PlaybackCurrentTime = 0.0f;

        FVector InitialLocation;
        FRotator InitialRotation;
        FVector InitialScale;

        bool bReversePlayback = false;

        TSet<int32> FiredNotifyIndices;
    };
