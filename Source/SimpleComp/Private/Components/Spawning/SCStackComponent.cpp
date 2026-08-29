#include "Components/Spawning/SCStackComponent.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSCStack, Log, All);

USCStackComponent::USCStackComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    bTickInEditor = true;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void USCStackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsValid(StackHISM) || SlotStatuses.IsEmpty())
    {
        return;
    }

    if (CurveMode == ESCStackCurveMode::Inertia)
    {
        UpdateInertiaSimulation(DeltaTime);
    }
    else if (CurveMode == ESCStackCurveMode::ManualCurve)
    {
        BuildCachedCurve();
        bNeedsTransformUpdate = true;
    }

    if (bNeedsTransformUpdate)
    {
        RefreshStackTransforms();
        bNeedsTransformUpdate = false;
    }

    if (GetTotalCapacity() != SlotStatuses.Num())
    {
        InitializeRuntimeState();
    }

    if (!bEnableFillAnimation)
    {
        return;
    }

    const float ClampedLevel = FMath::Clamp(FillLevel, 0.f, 1.f);

    if (FMath::IsNearlyEqual(ClampedLevel, LastAppliedFillLevel, KINDA_SMALL_NUMBER))
    {
        return;
    }

    const int32 TotalCapacity = GetTotalCapacity();
    const int32 NewTarget     = FMath::RoundToInt(ClampedLevel * static_cast<float>(TotalCapacity));
    const int32 OldTarget     = FMath::RoundToInt(LastAppliedFillLevel * static_cast<float>(TotalCapacity));

    LastAppliedFillLevel = ClampedLevel;

    if (NewTarget > OldTarget)
    {
        int32 SlotsNeeded = NewTarget - OldTarget;
        for (int32 i = 0; i < SlotStatuses.Num() && SlotsNeeded > 0; ++i)
        {
            if (SlotStatuses[i] == ESCSlotStatus::Free)
            {
                SlotStatuses[i] = ESCSlotStatus::Reserved;
                ConfirmArrival(i);
                --SlotsNeeded;
            }
        }
    }
    else
    {
        int32 SlotsToRelease = OldTarget - NewTarget;
        for (int32 i = SlotStatuses.Num() - 1; i >= 0 && SlotsToRelease > 0; --i)
        {
            if (SlotStatuses[i] == ESCSlotStatus::Filled)
            {
                SlotStatuses[i] = ESCSlotStatus::Free;
                FTransform HiddenTransform = CalculateDeformedTransform(CalculateSlotGridTransform(i));
                HiddenTransform.SetScale3D(FVector::ZeroVector);
                StackHISM->UpdateInstanceTransform(i, HiddenTransform, false, false);
                --SlotsToRelease;
            }
        }
        StackHISM->MarkRenderStateDirty();
    }
}

void USCStackComponent::OnRegister()
{
    PrimaryComponentTick.bCanEverTick = true;
    Super::OnRegister();
}

void USCStackComponent::BeginPlay()
{
    Super::BeginPlay();

    ensure(GetOwner() != nullptr);

    CreateAndAttachHISM();
    InitializeRuntimeState();

    if (bEnableFillAnimation || CurveMode == ESCStackCurveMode::Inertia)
    {
        PrimaryComponentTick.bCanEverTick = true;
        SetComponentTickEnabled(true);
    }
    
    if (AActor* Owner = GetOwner())
    {
        PreviousOwnerLocation = Owner->GetActorLocation();
    }

    if (!bEnableFillAnimation && InitialFillLevel > 0.f)
    {
        SetFillLevel(InitialFillLevel);
    }
}

void USCStackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelPendingFill();
    ClearAllAnimations();
    Super::EndPlay(EndPlayReason);
}

void USCStackComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    if (IsValid(StackHISM))
    {
        StackHISM->DestroyComponent();
        StackHISM = nullptr;
    }
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

#if WITH_EDITOR
void USCStackComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
    {
        return;
    }

    CreateAndAttachHISM();
    UpdateEditorPreview();
}

void USCStackComponent::OnComponentCreated()
{
    Super::OnComponentCreated();

#if WITH_EDITOR
    CreateAndAttachHISM();
    UpdateEditorPreview();
#endif
}
#endif

// ---------------------------------------------------------------------------
// Initialization & Preview
// ---------------------------------------------------------------------------

void USCStackComponent::CreateAndAttachHISM()
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }

    if (IsValid(StackHISM) && StackHISM->GetOwner() == Owner)
    {
        if (!StackHISM->IsRegistered())
        {
            StackHISM->RegisterComponent();
        }
        if (StackHISM->GetAttachParent() != this)
        {
            StackHISM->SetupAttachment(this);
        }
        return;
    }

    StackHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(
        Owner,
        UHierarchicalInstancedStaticMeshComponent::StaticClass(),
        MakeUniqueObjectName(Owner, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("SCStackHISM")),
        RF_Transient);

    if (!IsValid(StackHISM))
    {
        return;
    }

    StackHISM->SetVisibility(true);
    StackHISM->SetHiddenInGame(false);
    StackHISM->BoundsScale = 10000.0f; // Force bounds to remain huge even if instances shrink
    StackHISM->SetupAttachment(this);
    StackHISM->RegisterComponent();
}

void USCStackComponent::InitializeRuntimeState()
{
    if (!IsValid(StackHISM))
    {
        return;
    }

    if (IsValid(ElementMesh))
    {
        StackHISM->SetStaticMesh(ElementMesh);
    }
    else
    {
        UE_LOG(LogSCStack, Warning, TEXT("USCStackComponent on '%s': ElementMesh is not set. "
            "HISM will be empty until a mesh is assigned."), *GetOwner()->GetName());
    }

    StackHISM->ClearInstances();

    const int32 SafeRows    = FMath::Max(1, Rows);
    const int32 SafeColumns = FMath::Max(1, Columns);
    const int32 SafeLayers  = FMath::Max(1, Layers);
    const int32 TotalSlots  = SafeRows * SafeColumns * SafeLayers;

    TArray<FTransform> InitialTransforms;
    InitialTransforms.Reserve(TotalSlots);

    for (int32 i = 0; i < TotalSlots; ++i)
    {
        // Add instances at full scale so HISM computes the correct maximum bounding box
        FTransform FullTransform = CalculateDeformedTransform(CalculateSlotGridTransform(i));
        InitialTransforms.Add(FullTransform);
    }

    StackHISM->AddInstances(InitialTransforms, false);

    // Now hide them by setting scale to a near-zero value.
    // We cannot use exactly FVector::ZeroVector, because if a HISM tree rebuilds while all instances
    // are exactly 0 scale, its bounds collapse to 0, causing it to be permanently frustum culled.
    for (int32 i = 0; i < TotalSlots; ++i)
    {
        FTransform HiddenTransform = InitialTransforms[i];
        HiddenTransform.SetScale3D(FVector(0.0001f));
        StackHISM->UpdateInstanceTransform(i, HiddenTransform, false, false, false);
    }
    
    StackHISM->MarkRenderStateDirty();

    SlotStatuses.Reset(TotalSlots);
    SlotStatuses.Init(ESCSlotStatus::Free, TotalSlots);

    LastAppliedFillLevel = 0.f;
}

void USCStackComponent::UpdateEditorPreview()
{
    if (!IsValid(StackHISM))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World && World->IsGameWorld())
    {
        return;
    }

    StackHISM->ClearInstances();

    if (!bShowPreview || !IsValid(ElementMesh))
    {
        return;
    }

    StackHISM->SetStaticMesh(ElementMesh);

    const int32 SafeRows    = FMath::Max(1, Rows);
    const int32 SafeColumns = FMath::Max(1, Columns);
    const int32 SafeLayers  = FMath::Max(1, Layers);
    const int32 TotalSlots  = SafeRows * SafeColumns * SafeLayers;

    TArray<FTransform> PreviewTransforms;
    PreviewTransforms.Reserve(TotalSlots);

    const int32 TargetCount = FMath::RoundToInt(FMath::Clamp(FillLevel, 0.0f, 1.0f) * static_cast<float>(TotalSlots));

    for (int32 i = 0; i < TotalSlots; ++i)
    {
        FTransform SlotTransform = CalculateDeformedTransform(CalculateSlotGridTransform(i));
        
        if (i >= TargetCount)
        {
            SlotTransform.SetScale3D(FVector(0.0001f));
        }
        
        PreviewTransforms.Add(SlotTransform);
    }

    StackHISM->AddInstances(PreviewTransforms, false);
}

// ---------------------------------------------------------------------------
// Slot geometry
// ---------------------------------------------------------------------------

FTransform USCStackComponent::CalculateSlotGridTransform(int32 SlotID) const
{
    const int32 SafeRows    = FMath::Max(1, Rows);
    const int32 SafeColumns = FMath::Max(1, Columns);
    const int32 SafeLayers  = FMath::Max(1, Layers);

    const int32 Layer = SlotID / (SafeRows * SafeColumns);
    const int32 Row   = (SlotID % (SafeRows * SafeColumns)) / SafeColumns;
    const int32 Col   = SlotID % SafeColumns;

    FVector Padding;

    if (bAutoCalculatePadding && IsValid(ElementMesh))
    {
        const FBox    MeshBox    = ElementMesh->GetBounds().GetBox();
        const FVector MeshExtent = MeshBox.GetSize();
        Padding = MeshExtent * TargetElementScale;
    }
    else
    {
        Padding = ManualPadding;
    }

    const FVector GridCenter(
        (SafeColumns - 1) * Padding.X * 0.5f,
        (SafeRows    - 1) * Padding.Y * 0.5f,
        0.0f);

    const FVector LocalOffset(
        Col   * Padding.X - GridCenter.X,
        Row   * Padding.Y - GridCenter.Y,
        Layer * Padding.Z);

    FTransform SlotTransform;
    SlotTransform.SetLocation(LocalOffset);
    
    FRandomStream Stream(RandomSeed + SlotID);
    
    if (bEnableRandomRotation)
    {
        FRotator RandRot(
            Stream.FRandRange(RandomRotationMin.Pitch, RandomRotationMax.Pitch),
            Stream.FRandRange(RandomRotationMin.Yaw,   RandomRotationMax.Yaw),
            Stream.FRandRange(RandomRotationMin.Roll,  RandomRotationMax.Roll)
        );
        SlotTransform.SetRotation(RandRot.Quaternion());
    }

    if (bEnableRandomScale)
    {
        if (bUniformRandomScale)
        {
            float RandScale = Stream.FRandRange(RandomScaleMin.X, RandomScaleMax.X);
            SlotTransform.SetScale3D(TargetElementScale * RandScale);
        }
        else
        {
            FVector RandScale(
                Stream.FRandRange(RandomScaleMin.X, RandomScaleMax.X),
                Stream.FRandRange(RandomScaleMin.Y, RandomScaleMax.Y),
                Stream.FRandRange(RandomScaleMin.Z, RandomScaleMax.Z)
            );
            SlotTransform.SetScale3D(TargetElementScale * RandScale);
        }
    }
    else
    {
        SlotTransform.SetScale3D(TargetElementScale);
    }

    return SlotTransform;
}

void USCStackComponent::BuildCachedCurve()
{
    CachedCurve.Points.Empty();
    CachedRotations.Empty();
    
    if (CurveMode != ESCStackCurveMode::ManualCurve || ControlPointComponents.IsEmpty())
    {
        return;
    }

    CachedCurve.AddPoint(0.0f, FVector::ZeroVector);
    CachedRotations.Add(TPair<float, FQuat>(0.0f, FQuat::Identity));
    
    struct FPointData
    {
        float Z;
        FVector XY;
        FQuat Rot;
    };
    TArray<FPointData> Points;

    for (const FComponentReference& Ref : ControlPointComponents)
    {
        USceneComponent* Comp = Cast<USceneComponent>(Ref.GetComponent(GetOwner()));
        if (Comp)
        {
            FVector RelLoc = GetComponentTransform().InverseTransformPosition(Comp->GetComponentLocation());
            FQuat RelRot = GetComponentTransform().InverseTransformRotation(Comp->GetComponentRotation().Quaternion());
            if (RelLoc.Z > 1.0)
            {
                Points.Add({ static_cast<float>(RelLoc.Z), FVector(RelLoc.X, RelLoc.Y, 0.0), RelRot });
            }
        }
    }
    
    Points.Sort([](const FPointData& A, const FPointData& B) { return A.Z < B.Z; });
    
    float LastZ = 0.0f;
    for (const FPointData& Pt : Points)
    {
        float SafeZ = FMath::Max(Pt.Z, LastZ + 1.0f);
        CachedCurve.AddPoint(SafeZ, Pt.XY);
        CachedRotations.Add(TPair<float, FQuat>(SafeZ, Pt.Rot));
        LastZ = SafeZ;
    }
    
    CachedCurve.AutoSetTangents();
    
    if (CachedCurve.Points.Num() > 0)
    {
        CachedCurve.Points[0].LeaveTangent = FVector::ZeroVector;
        CachedCurve.Points[0].ArriveTangent = FVector::ZeroVector;
        CachedCurve.Points[0].InterpMode = CIM_CurveUser;
    }
}

FTransform USCStackComponent::CalculateDeformedTransform(const FTransform& GridTransform) const
{
    if (CurveMode == ESCStackCurveMode::None)
    {
        return GridTransform;
    }

    const int32 SafeLayers = FMath::Max(1, Layers);
    float MaxZ = 100.0f;
    if (bAutoCalculatePadding && IsValid(ElementMesh))
    {
        MaxZ = ElementMesh->GetBounds().GetBox().GetSize().Z * TargetElementScale.Z * SafeLayers;
    }
    else
    {
        MaxZ = ManualPadding.Z * SafeLayers;
    }
    
    if (MaxZ <= KINDA_SMALL_NUMBER)
    {
        return GridTransform;
    }

    const FVector LocalPos = GridTransform.GetLocation();
    const float Alpha = FMath::Clamp(LocalPos.Z / MaxZ, 0.0f, 1.0f);
    
    FVector CurvePos = FVector::ZeroVector;
    FQuat CurveQuat = FQuat::Identity;

    if (CurveMode == ESCStackCurveMode::Inertia)
    {
        float CurveAlpha = Alpha * Alpha;
        CurvePos = CurrentTipLag * CurveAlpha;
        CurvePos.Z = LocalPos.Z;
        
        FVector CurveTangent = FVector(2.0f * CurrentTipLag.X * Alpha, 2.0f * CurrentTipLag.Y * Alpha, MaxZ).GetSafeNormal();
        FQuat TargetQuat = FQuat::FindBetweenNormals(FVector::UpVector, CurveTangent);
        
        // Clamp the angle to InertiaMaxTiltDegrees
        float AngleRad = TargetQuat.GetAngle();
        float MaxAngleRad = FMath::DegreesToRadians(InertiaMaxTiltDegrees);
        if (AngleRad > MaxAngleRad && AngleRad > KINDA_SMALL_NUMBER)
        {
            FVector Axis = TargetQuat.GetRotationAxis();
            CurveQuat = FQuat(Axis, MaxAngleRad);
        }
        else
        {
            CurveQuat = TargetQuat;
        }
    }
    else if (CurveMode == ESCStackCurveMode::ManualCurve && CachedCurve.Points.Num() > 1)
    {
        FVector Offset = CachedCurve.Eval(LocalPos.Z);
        CurvePos = FVector(Offset.X, Offset.Y, LocalPos.Z);
        
        FQuat TargetQuat = FQuat::Identity;
        if (CachedRotations.Num() == 1)
        {
            TargetQuat = CachedRotations[0].Value;
        }
        else
        {
            for (int32 i = 0; i < CachedRotations.Num() - 1; ++i)
            {
                if (LocalPos.Z >= CachedRotations[i].Key && LocalPos.Z <= CachedRotations[i+1].Key)
                {
                    float AlphaRot = (LocalPos.Z - CachedRotations[i].Key) / FMath::Max(0.0001f, CachedRotations[i+1].Key - CachedRotations[i].Key);
                    TargetQuat = FQuat::Slerp(CachedRotations[i].Value, CachedRotations[i+1].Value, AlphaRot);
                    break;
                }
            }
            if (LocalPos.Z > CachedRotations.Last().Key)
            {
                TargetQuat = CachedRotations.Last().Value;
            }
        }
        
        CurveQuat = FQuat::Slerp(FQuat::Identity, TargetQuat, TiltScale);
    }
    else
    {
        return GridTransform;
    }
    
    FVector FinalPos = CurvePos + CurveQuat.RotateVector(FVector(LocalPos.X, LocalPos.Y, 0.0f));
    
    FTransform FinalTransform;
    FinalTransform.SetLocation(FinalPos);
    FinalTransform.SetRotation(CurveQuat * GridTransform.GetRotation());
    FinalTransform.SetScale3D(GridTransform.GetScale3D());
    
    return FinalTransform;
}

void USCStackComponent::UpdateInertiaSimulation(float DeltaTime)
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner) || DeltaTime <= 0.0f) return;

    FVector CurrentOwnerLocation = Owner->GetActorLocation();
    FVector CurrentVelocity = (CurrentOwnerLocation - PreviousOwnerLocation) / DeltaTime;
    
    FVector LocalAcceleration = Owner->GetActorTransform().InverseTransformVectorNoScale(CurrentVelocity - PreviousOwnerVelocity) / DeltaTime;
    
    // Target lag is opposite to acceleration
    FVector TargetTipLag = -LocalAcceleration * 0.05f; 
    
    TargetTipLag.X = FMath::Clamp(TargetTipLag.X, -MaxTipLag, MaxTipLag);
    TargetTipLag.Y = FMath::Clamp(TargetTipLag.Y, -MaxTipLag, MaxTipLag);
    TargetTipLag.Z = 0.0f;
    
    FVector SpringForce = (TargetTipLag - CurrentTipLag) * InertiaStiffness;
    FVector DampingForce = -CurrentTipVelocity * InertiaDamping;
    
    FVector Acceleration = SpringForce + DampingForce;
    CurrentTipVelocity += Acceleration * DeltaTime;
    CurrentTipLag += CurrentTipVelocity * DeltaTime;
    
    if (CurrentTipLag.SizeSquared() > KINDA_SMALL_NUMBER || CurrentTipVelocity.SizeSquared() > KINDA_SMALL_NUMBER)
    {
        bNeedsTransformUpdate = true;
    }
    
    PreviousOwnerLocation = CurrentOwnerLocation;
    PreviousOwnerVelocity = CurrentVelocity;
}

void USCStackComponent::RefreshStackTransforms()
{
    if (!IsValid(StackHISM) || SlotStatuses.IsEmpty())
    {
        return;
    }
    
    for (int32 i = 0; i < SlotStatuses.Num(); ++i)
    {
        // Don't update transform of animating slots from here to prevent stutter,
        // ActiveAnimations overrides their transform in TickSlotAnimation.
        if (ActiveAnimations.Contains(i))
        {
            continue;
        }

        FTransform Deformed = CalculateDeformedTransform(CalculateSlotGridTransform(i));
        
        if (SlotStatuses[i] == ESCSlotStatus::Free)
        {
            Deformed.SetScale3D(FVector(0.0001f));
        }
        
        StackHISM->UpdateInstanceTransform(i, Deformed, false, false, false);
    }
    StackHISM->MarkRenderStateDirty();
}

// ---------------------------------------------------------------------------
// Public API — Slot Management
// ---------------------------------------------------------------------------

int32 USCStackComponent::RequestSlot()
{
    if (SlotStatuses.IsEmpty())
    {
        UE_LOG(LogSCStack, Warning, TEXT("USCStackComponent on '%s': RequestSlot called before BeginPlay — "
            "SlotStatuses is empty. Returning INDEX_NONE."), *GetOwner()->GetName());
        return INDEX_NONE;
    }

    for (int32 i = 0; i < SlotStatuses.Num(); ++i)
    {
        if (SlotStatuses[i] == ESCSlotStatus::Free)
        {
            SlotStatuses[i] = ESCSlotStatus::Reserved;
            return i;
        }
    }

    UE_LOG(LogSCStack, Verbose, TEXT("USCStackComponent on '%s': No free slots available."),
        *GetOwner()->GetName());

    return INDEX_NONE;
}

void USCStackComponent::ConfirmArrival(int32 SlotID)
{
    if (!SlotStatuses.IsValidIndex(SlotID))
    {
        UE_LOG(LogSCStack, Warning, TEXT("USCStackComponent on '%s': ConfirmArrival called with invalid SlotID %d."),
            *GetOwner()->GetName(), SlotID);
        return;
    }

    if (SlotStatuses[SlotID] != ESCSlotStatus::Reserved)
    {
        UE_LOG(LogSCStack, Warning,
            TEXT("USCStackComponent on '%s': ConfirmArrival called on SlotID %d which is not Reserved."),
            *GetOwner()->GetName(), SlotID);
        return;
    }

    SlotStatuses[SlotID] = ESCSlotStatus::Filled;

    UWorld* World = GetWorld();
    if (!ensure(IsValid(World)))
    {
        return;
    }

    FSCSlotAnimState& AnimState = ActiveAnimations.FindOrAdd(SlotID);
    AnimState.Progress = 0.0f;

    FTimerDelegate Delegate;
    Delegate.BindUObject(this, &USCStackComponent::TickSlotAnimation, SlotID);

    World->GetTimerManager().SetTimer(
        AnimState.TimerHandle,
        Delegate,
        AnimationTickInterval,
        true);
}

void USCStackComponent::ReleaseSlot(int32 SlotID)
{
    if (!SlotStatuses.IsValidIndex(SlotID))
    {
        UE_LOG(LogSCStack, Warning, TEXT("USCStackComponent on '%s': ReleaseSlot called with invalid SlotID %d."),
            *GetOwner()->GetName(), SlotID);
        return;
    }

    if (SlotStatuses[SlotID] == ESCSlotStatus::Reserved)
    {
        SlotStatuses[SlotID] = ESCSlotStatus::Free;
    }
}

// ---------------------------------------------------------------------------
// Public API — Explosion
// ---------------------------------------------------------------------------

void USCStackComponent::Explode()
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    ClearAllAnimations();

    if (IsValid(ExplosionActorClass))
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        for (int32 i = 0; i < SlotStatuses.Num(); ++i)
        {
            if (SlotStatuses[i] != ESCSlotStatus::Filled)
            {
                continue;
            }

            FTransform InstanceWorldTransform;
            StackHISM->GetInstanceTransform(i, InstanceWorldTransform, true);
            World->SpawnActor<AActor>(ExplosionActorClass, InstanceWorldTransform, SpawnParams);
        }
    }

    for (int32 i = 0; i < SlotStatuses.Num(); ++i)
    {
        FTransform HiddenTransform = CalculateDeformedTransform(CalculateSlotGridTransform(i));
        HiddenTransform.SetScale3D(FVector::ZeroVector);
        StackHISM->UpdateInstanceTransform(i, HiddenTransform, false, false);
        SlotStatuses[i] = ESCSlotStatus::Free;
    }

    StackHISM->MarkRenderStateDirty();
}

// ---------------------------------------------------------------------------
// Public API — Getters
// ---------------------------------------------------------------------------

FTransform USCStackComponent::GetSlotWorldTransform(int32 SlotID) const
{
    if (IsValid(StackHISM) && StackHISM->GetInstanceCount() > 0 && SlotStatuses.IsValidIndex(SlotID))
    {
        FTransform WorldTransform;
        StackHISM->GetInstanceTransform(SlotID, WorldTransform, true);
        return WorldTransform;
    }

    AActor* Owner = GetOwner();
    if (IsValid(Owner) && SlotID >= 0)
    {
        return Owner->GetActorTransform() * CalculateDeformedTransform(CalculateSlotGridTransform(SlotID));
    }

    return FTransform::Identity;
}

int32 USCStackComponent::GetFilledSlotCount() const
{
    int32 Count = 0;
    for (const ESCSlotStatus& Status : SlotStatuses)
    {
        if (Status == ESCSlotStatus::Filled)
        {
            ++Count;
        }
    }
    return Count;
}

int32 USCStackComponent::GetTotalCapacity() const
{
    return FMath::Max(1, Rows) * FMath::Max(1, Columns) * FMath::Max(1, Layers);
}


// ---------------------------------------------------------------------------
// Public API — Fill Level
// ---------------------------------------------------------------------------

void USCStackComponent::SetFillLevel(float InFillLevel)
{
    if (!IsValid(StackHISM) || SlotStatuses.IsEmpty())
    {
        UE_LOG(LogSCStack, Warning, TEXT("USCStackComponent on '%s': SetFillLevel called before "
            "initialization. Ensure BeginPlay has run."), *GetOwner()->GetName());
        return;
    }

    float NewClampedLevel = FMath::Clamp(InFillLevel, 0.f, 1.f);
    if (FMath::IsNearlyEqual(NewClampedLevel, FillLevel, KINDA_SMALL_NUMBER) && PendingFillSlots.IsEmpty())
    {
        // Already at this target and no pending fills, do nothing (protects against Event Tick spam)
        return;
    }

    FillLevel = NewClampedLevel;

    if (bEnableFillAnimation)
    {
        return;
    }

    const int32 TargetCount    = FMath::RoundToInt(FillLevel * static_cast<float>(GetTotalCapacity()));
    const int32 CurrentFilled  = GetFilledSlotCount();

    CancelPendingFill();

    if (TargetCount > CurrentFilled)
    {
        int32 SlotsNeeded = TargetCount - CurrentFilled;

        for (int32 i = 0; i < SlotStatuses.Num() && SlotsNeeded > 0; ++i)
        {
            if (SlotStatuses[i] == ESCSlotStatus::Free)
            {
                SlotStatuses[i] = ESCSlotStatus::Reserved;
                PendingFillSlots.Enqueue(i);
                --SlotsNeeded;
            }
        }

        if (!PendingFillSlots.IsEmpty())
        {
            ProcessNextPendingSlot();
        }
    }
    else if (TargetCount < CurrentFilled)
    {
        int32 SlotsToRelease = CurrentFilled - TargetCount;

        for (int32 i = SlotStatuses.Num() - 1; i >= 0 && SlotsToRelease > 0; --i)
        {
            if (SlotStatuses[i] == ESCSlotStatus::Filled)
            {
                SlotStatuses[i] = ESCSlotStatus::Free;
                FTransform HiddenTransform = CalculateDeformedTransform(CalculateSlotGridTransform(i));
                HiddenTransform.SetScale3D(FVector(0.0001f));
                StackHISM->UpdateInstanceTransform(i, HiddenTransform, false, false);
                --SlotsToRelease;
            }
        }
        StackHISM->MarkRenderStateDirty();
    }
}

void USCStackComponent::ProcessNextPendingSlot()
{
    int32 SlotID = INDEX_NONE;
    if (!PendingFillSlots.Dequeue(SlotID))
    {
        return;
    }

    ConfirmArrival(SlotID);

    if (!PendingFillSlots.IsEmpty())
    {
        UWorld* World = GetWorld();
        if (IsValid(World) && FillStaggerDelay > 0.f)
        {
            FTimerDelegate Delegate;
            Delegate.BindUObject(this, &USCStackComponent::ProcessNextPendingSlot);
            World->GetTimerManager().SetTimer(FillStaggerTimerHandle, Delegate, FillStaggerDelay, false);
        }
        else
        {
            ProcessNextPendingSlot();
        }
    }
}

void USCStackComponent::CancelPendingFill()
{
    UWorld* World = GetWorld();
    if (IsValid(World))
    {
        World->GetTimerManager().ClearTimer(FillStaggerTimerHandle);
    }

    int32 SlotID = INDEX_NONE;
    while (PendingFillSlots.Dequeue(SlotID))
    {
        if (SlotStatuses.IsValidIndex(SlotID) && SlotStatuses[SlotID] == ESCSlotStatus::Reserved)
        {
            SlotStatuses[SlotID] = ESCSlotStatus::Free;
        }
    }
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

void USCStackComponent::TickSlotAnimation(int32 SlotID)
{
    FSCSlotAnimState* AnimState = ActiveAnimations.Find(SlotID);
    if (AnimState == nullptr)
    {
        return;
    }

    AnimState->Progress = FMath::Clamp(
        AnimState->Progress + (AnimationTickInterval / ScaleAnimationDuration),
        0.0f, 1.0f);

    const FVector CurrentScale = FMath::Lerp(FVector(0.0001f), TargetElementScale, AnimState->Progress);

    FTransform SlotTransform = CalculateDeformedTransform(CalculateSlotGridTransform(SlotID));
    SlotTransform.SetScale3D(CurrentScale);
    StackHISM->UpdateInstanceTransform(SlotID, SlotTransform, false, true);

    if (AnimState->Progress >= 1.0f)
    {
        FTimerHandle HandleToClose = AnimState->TimerHandle;
        ActiveAnimations.Remove(SlotID);

        UWorld* World = GetWorld();
        if (IsValid(World))
        {
            World->GetTimerManager().ClearTimer(HandleToClose);
        }

        OnSlotFilled(SlotID);
    }
}

void USCStackComponent::ClearAllAnimations()
{
    UWorld* World = GetWorld();

    for (auto& Pair : ActiveAnimations)
    {
        if (IsValid(World))
        {
            World->GetTimerManager().ClearTimer(Pair.Value.TimerHandle);
        }
    }

    ActiveAnimations.Empty();
}
