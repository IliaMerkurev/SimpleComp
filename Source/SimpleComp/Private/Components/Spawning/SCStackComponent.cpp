#include "Components/Spawning/SCStackComponent.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Core/Interfaces/SCMessageInterface.h"
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
    
    StackHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("SCStackHISM"));
    StackHISM->SetupAttachment(this);
    StackHISM->SetVisibility(true);
    StackHISM->SetHiddenInGame(false);
    StackHISM->BoundsScale =
        10000.0f; // Existing all-hidden HISM workaround; changing it requires visual culling validation.
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void USCStackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bRuntimeInitialized || bShuttingDown)
    {
        return;
    }
    RefreshSettings();
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
}

void USCStackComponent::OnRegister()
{
    PrimaryComponentTick.bCanEverTick = true;
    Super::OnRegister();

    if (StackHISM)
    {
        if (StackHISM->GetAttachParent() != this)
        {
            StackHISM->SetupAttachment(this);
        }
        
        if (!StackHISM->IsRegistered())
        {
            StackHISM->RegisterComponent();
        }
    }

#if WITH_EDITOR
    UpdateEditorPreview();
#endif
}

void USCStackComponent::BeginPlay()
{
    Super::BeginPlay();
    bShuttingDown = false;
    bRuntimeInitialized = true;
    InitializeRuntimeState();
    PreviousOwnerLocation = GetComponentLocation();
    RefreshSettings();
    if (!bEnableFillAnimation)
    {
        SetFillLevel(InitialFillLevel);
    }
    // Settings can be written directly by Blueprint or Sequencer while Tick is disabled.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SettingsTimerHandle, this, &USCStackComponent::RefreshSettings, 0.1f, true);
    }
}

void USCStackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bShuttingDown = true;
    CancelSpawning();
    CancelPendingFill();
    ClearAllAnimations();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SettingsTimerHandle);
    }
    bRuntimeInitialized = false;
    Super::EndPlay(EndPlayReason);
}

void USCStackComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    bShuttingDown = true;
    CancelSpawning();
    CancelPendingFill();
    ClearAllAnimations();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SettingsTimerHandle);
    }
    bRuntimeInitialized = false;
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

    if (bEnableCollision)
    {
        StackHISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    else
    {
        StackHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    UpdateEditorPreview();
}

void USCStackComponent::OnComponentCreated()
{
    Super::OnComponentCreated();

#if WITH_EDITOR
    if (bEnableCollision)
    {
        StackHISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    else
    {
        StackHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    UpdateEditorPreview();
#endif
}
#endif

// ---------------------------------------------------------------------------
// Initialization & Preview
// ---------------------------------------------------------------------------

void USCStackComponent::InitializeRuntimeState()
{
    if (!IsValid(StackHISM))
    {
        return;
    }
    int32 Capacity = GetTotalCapacity();
    const int32 Occupied = TicketToIndex.Num();
    if (Capacity <= 0 || Capacity < Occupied)
    {
        // A layout edit must not destroy resources or invalidate in-flight reservations.
        UE_LOG(LogSCStack, Warning, TEXT("Stack resize rejected: capacity must fit all occupied slots."));
        Rows = AppliedDimensions.X;
        Columns = AppliedDimensions.Y;
        Layers = AppliedDimensions.Z;
        Capacity = GetTotalCapacity();
    }
    if (Capacity < SlotStatuses.Num())
    {
        SlotStatuses.RemoveAll(
            [](const FSCSlotData& Slot)
            {
                return Slot.Status == ESCSlotStatus::Free;
            });
    }
    SlotStatuses.SetNum(Capacity);
    RebuildTicketLookup();
    AppliedDimensions = FIntVector(FMath::Max(1, Rows), FMath::Max(1, Columns), FMath::Max(1, Layers));
    StackHISM->SetStaticMesh(ElementMesh);
    StackHISM->ClearInstances();
    TArray<FTransform> Transforms;
    Transforms.Reserve(Capacity);
    for (int32 Index = 0; Index < Capacity; ++Index)
    {
        Transforms.Add(CalculateDeformedTransform(CalculateSlotGridTransform(Index)));
    }
    StackHISM->AddInstances(Transforms, false);
    RefreshStackTransforms();
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

    if (CurveMode == ESCStackCurveMode::ManualCurve)
    {
        BuildCachedCurve();
    }

    StackHISM->SetStaticMesh(ElementMesh);

    const int32 TotalSlots = GetTotalCapacity();

    TArray<FTransform> PreviewTransforms;
    PreviewTransforms.Reserve(TotalSlots);

    for (int32 i = 0; i < TotalSlots; ++i)
    {
        FTransform SlotTransform = CalculateDeformedTransform(CalculateSlotGridTransform(i));
        PreviewTransforms.Add(SlotTransform);
    }

    StackHISM->AddInstances(PreviewTransforms, false);
    
    StackHISM->BuildTreeIfOutdated(true, false);
    StackHISM->MarkRenderStateDirty();
}

// ---------------------------------------------------------------------------
// Slot geometry
// ---------------------------------------------------------------------------

FTransform USCStackComponent::CalculateSlotGridTransform(int32 SlotID) const
{
    if (GetTotalCapacity() == 0)
    {
        return FTransform::Identity;
    }
    const int32 SafeRows    = FMath::Max(1, Rows);
    const int32 SafeColumns = FMath::Max(1, Columns);

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

    const int32 Identity = SlotStatuses.IsValidIndex(SlotID) && SlotStatuses[SlotID].TicketID != INDEX_NONE
                               ? SlotStatuses[SlotID].TicketID
                               : SlotID;
    FRandomStream Stream(static_cast<int32>(static_cast<uint32>(RandomSeed) + static_cast<uint32>(Identity)));

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
    Points.Reserve(ControlPointComponents.Num());

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

int32 USCStackComponent::FindIndexByTicket(int32 TicketID) const
{
    const int32* Index = TicketToIndex.Find(TicketID);
    return Index ? *Index : INDEX_NONE;
}

void USCStackComponent::RebuildTicketLookup()
{
    TicketToIndex.Reset();
    for (int32 Index = 0; Index < SlotStatuses.Num(); ++Index)
    {
        if (SlotStatuses[Index].Status != ESCSlotStatus::Free)
        {
            TicketToIndex.Add(SlotStatuses[Index].TicketID, Index);
        }
    }
}

uint32 USCStackComponent::CalculateSettingsHash() const
{
    uint32 Hash = GetTypeHash(ElementMesh.Get());
    const auto Add = [&Hash](const auto& Value)
    {
        Hash = HashCombine(Hash, GetTypeHash(Value));
    };
    Add(Rows);
    Add(Columns);
    Add(Layers);
    Add(bAutoCalculatePadding);
    Add(ManualPadding);
    Add(TargetElementScale);
    Add(RandomSeed);
    Add(bEnableRandomRotation);
    Add(RandomRotationMin.Pitch);
    Add(RandomRotationMin.Yaw);
    Add(RandomRotationMin.Roll);
    Add(RandomRotationMax.Pitch);
    Add(RandomRotationMax.Yaw);
    Add(RandomRotationMax.Roll);
    Add(bEnableRandomScale);
    Add(bUniformRandomScale);
    Add(RandomScaleMin);
    Add(RandomScaleMax);
    Add(static_cast<uint8>(CurveMode));
    Add(TiltScale);
    Add(InertiaMaxTiltDegrees);
    return Hash;
}

void USCStackComponent::RefreshSettings()
{
    if (!bRuntimeInitialized || bShuttingDown || bSynchronizingSettings || !IsValid(StackHISM))
    {
        return;
    }
    TGuardValue<bool> Guard(bSynchronizingSettings, true);
    StackHISM->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics
                                                    : ECollisionEnabled::NoCollision);
    SetComponentTickEnabled(bEnableFillAnimation || CurveMode != ESCStackCurveMode::None);
    if (AppliedCurveMode != CurveMode)
    {
        PreviousOwnerLocation = GetComponentLocation();
        PreviousOwnerVelocity = FVector::ZeroVector;
        CurrentTipLag = CurrentTipVelocity = FVector::ZeroVector;
        AppliedCurveMode = CurveMode;
    }
    const uint32 SettingsHash = CalculateSettingsHash();
    if (SettingsHash != AppliedSettingsHash)
    {
        BuildCachedCurve();
        if (GetTotalCapacity() != SlotStatuses.Num())
        {
            InitializeRuntimeState();
        }
        else
        {
            StackHISM->SetStaticMesh(ElementMesh);
        }
        AppliedDimensions = FIntVector(FMath::Max(1, Rows), FMath::Max(1, Columns), FMath::Max(1, Layers));
        AppliedSettingsHash = CalculateSettingsHash();
        RefreshStackTransforms();
    }
    if (bEnableFillAnimation && FMath::IsFinite(FillLevel) && !FMath::IsNearlyEqual(FillLevel, LastAppliedFillLevel))
    {
        SetFillLevel(FillLevel);
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
        float MaxAngleRad = FMath::DegreesToRadians(FMath::Clamp(InertiaMaxTiltDegrees, 0.f, 90.f));
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

        CurveQuat = TargetQuat;
    }
    else
    {
        return GridTransform;
    }

    CurveQuat = FQuat::Slerp(FQuat::Identity, CurveQuat, FMath::Clamp(TiltScale, 0.f, 1.f));
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
    if (!IsValid(Owner) || !FMath::IsFinite(DeltaTime) || DeltaTime <= UE_SMALL_NUMBER)
    {
        return;
    }

    FVector CurrentOwnerLocation = GetComponentLocation();
    FVector CurrentVelocity = (CurrentOwnerLocation - PreviousOwnerLocation) / DeltaTime;

    FVector LocalAcceleration =
        GetComponentTransform().InverseTransformVectorNoScale(CurrentVelocity - PreviousOwnerVelocity) / DeltaTime;

    // Target lag is opposite to acceleration
    FVector TargetTipLag = -LocalAcceleration * 0.05f;

    const double SafeMaxLag = FMath::IsFinite(MaxTipLag) ? FMath::Max(0.f, MaxTipLag) : 0.f;
    TargetTipLag.X = FMath::Clamp(TargetTipLag.X, -SafeMaxLag, SafeMaxLag);
    TargetTipLag.Y = FMath::Clamp(TargetTipLag.Y, -SafeMaxLag, SafeMaxLag);
    TargetTipLag.Z = 0.0f;

    const FVector PreviousTipLag = CurrentTipLag;
    const double Step = DeltaTime;
    const double Stiffness = FMath::IsFinite(InertiaStiffness) ? FMath::Max(0.f, InertiaStiffness) : 0.f;
    const double Damping = FMath::IsFinite(InertiaDamping) ? FMath::Max(0.f, InertiaDamping) : 0.f;
    // Implicit Euler solves the spring and damping together, avoiding explicit-step energy growth on long frames.
    CurrentTipVelocity = (CurrentTipVelocity + Step * Stiffness * (TargetTipLag - CurrentTipLag)) /
                         (1.0 + Step * Damping + Step * Step * Stiffness);
    CurrentTipLag += CurrentTipVelocity * Step;
    if (CurrentTipLag.ContainsNaN() || CurrentTipVelocity.ContainsNaN())
    {
        CurrentTipLag = CurrentTipVelocity = FVector::ZeroVector;
    }
    if (!CurrentTipLag.Equals(PreviousTipLag, UE_SMALL_NUMBER))
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
    TArray<FTransform> Transforms;
    Transforms.Reserve(SlotStatuses.Num());
    for (int32 Index = 0; Index < SlotStatuses.Num(); ++Index)
    {
        FTransform Transform = CalculateDeformedTransform(CalculateSlotGridTransform(Index));
        const FSCSlotData& Slot = SlotStatuses[Index];
        if (Slot.Status != ESCSlotStatus::Filled)
        {
            Transform.SetScale3D(FVector(0.0001f));
        }
        else if (const FSCSlotAnimState* Animation = ActiveAnimations.Find(Slot.TicketID))
        {
            Transform.SetScale3D(FMath::Lerp(FVector(0.0001f), Transform.GetScale3D(), Animation->Progress));
        }
        Transforms.Add(Transform);
    }
    StackHISM->BatchUpdateInstancesTransforms(0, Transforms, false, true, false);
}

// ---------------------------------------------------------------------------
// Public API — Slot Management
// ---------------------------------------------------------------------------

int32 USCStackComponent::RequestSlot()
{
    if (!bRuntimeInitialized || bShuttingDown || NextTicketID == MAX_int32)
    {
        return INDEX_NONE;
    }
    for (int32 Index = 0; Index < SlotStatuses.Num(); ++Index)
    {
        FSCSlotData& Slot = SlotStatuses[Index];
        if (Slot.Status == ESCSlotStatus::Free)
        {
            Slot.Status = ESCSlotStatus::Reserved;
            Slot.TicketID = NextTicketID++;
            TicketToIndex.Add(Slot.TicketID, Index);
            return Slot.TicketID;
        }
    }
    return INDEX_NONE;
}

void USCStackComponent::ConfirmArrival(int32 TicketID)
{
    if (bShuttingDown)
    {
        return;
    }
    StartSlotAnimation(FindIndexByTicket(TicketID));
}

void USCStackComponent::ReleaseSlot(int32 TicketID)
{
    const int32 Index = FindIndexByTicket(TicketID);
    if (Index != INDEX_NONE && SlotStatuses[Index].Status == ESCSlotStatus::Reserved)
    {
        FreeSlot(Index);
    }
}

void USCStackComponent::FreeSlot(int32 SlotIndex)
{
    if (!SlotStatuses.IsValidIndex(SlotIndex))
    {
        return;
    }
    const int32 Ticket = SlotStatuses[SlotIndex].TicketID;
    ActiveAnimations.Remove(Ticket);
    TicketToIndex.Remove(Ticket);
    SlotStatuses[SlotIndex] = FSCSlotData();
    if (IsValid(StackHISM) && !bShuttingDown)
    {
        FTransform Hidden = CalculateDeformedTransform(CalculateSlotGridTransform(SlotIndex));
        Hidden.SetScale3D(FVector(0.0001f));
        StackHISM->UpdateInstanceTransform(SlotIndex, Hidden, false, true);
    }
    if (ActiveAnimations.IsEmpty())
    {
        ClearAllAnimations();
    }
}

// ---------------------------------------------------------------------------
// Public API — Actor Spawning
// ---------------------------------------------------------------------------

void USCStackComponent::SpawnActors()
{
    SpawnActorsWithSettings(DefaultSpawnSettings);
}

void USCStackComponent::Explode()
{
    SpawnActors();
}

void USCStackComponent::CancelSpawning()
{
    UWorld* World = GetWorld();
    if (IsValid(World))
    {
        World->GetTimerManager().ClearTimer(SpawnWaveTimerHandle);
    }

    PendingSpawnWaves.Empty();
    ++SpawnRevision;
}

int32 USCStackComponent::CalculateSlotWaveIndex(int32 SlotID, const FSCStackSpawnSettings& Settings) const
{
    if (GetTotalCapacity() == 0)
    {
        return 0;
    }
    const int32 SafeRows = FMath::Max(1, Rows);
    const int32 SafeColumns = FMath::Max(1, Columns);
    const int32 SafeLayers = FMath::Max(1, Layers);

    const int32 Layer = SlotID / (SafeRows * SafeColumns);
    const int32 Row   = (SlotID % (SafeRows * SafeColumns)) / SafeColumns;
    const int32 Col   = SlotID % SafeColumns;

    int32 VerticalDist = 0;
    if (Settings.bFromTop && Settings.bFromBottom)
    {
        VerticalDist = FMath::Min(Layer, (SafeLayers - 1) - Layer);
    }
    else if (Settings.bFromTop)
    {
        VerticalDist = (SafeLayers - 1) - Layer;
    }
    else if (Settings.bFromBottom)
    {
        VerticalDist = Layer;
    }

    int32 HorizontalDist = 0;
    const int32 DistToEdgeCol = FMath::Min(Col, (SafeColumns - 1) - Col);
    const int32 DistToEdgeRow = FMath::Min(Row, (SafeRows - 1) - Row);
    const int32 DistToEdge = FMath::Min(DistToEdgeCol, DistToEdgeRow);

    const float CenterCol = (SafeColumns - 1) * 0.5f;
    const float CenterRow = (SafeRows - 1) * 0.5f;
    const int32 DistToCenter = FMath::RoundToInt(FMath::Max(FMath::Abs(Col - CenterCol), FMath::Abs(Row - CenterRow)));

    if (Settings.bFromEdges && Settings.bFromCenter)
    {
        HorizontalDist = FMath::Min(DistToEdge, DistToCenter);
    }
    else if (Settings.bFromEdges)
    {
        HorizontalDist = DistToEdge;
    }
    else if (Settings.bFromCenter)
    {
        HorizontalDist = DistToCenter;
    }

    return VerticalDist + HorizontalDist;
}

void USCStackComponent::SpawnActorsWithSettings(const FSCStackSpawnSettings& Settings)
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    if (bShuttingDown || !IsValid(SpawnActorClass))
    {
        return;
    }
    CancelSpawning();
    const uint64 Revision = SpawnRevision;

    ActiveSpawnSettings = Settings;

    TMap<int32, TArray<int32>> WaveBuckets;
    for (int32 i = 0; i < SlotStatuses.Num(); ++i)
    {
        if (SlotStatuses[i].Status == ESCSlotStatus::Filled && !ConvertingTickets.Contains(SlotStatuses[i].TicketID))
        {
            const int32 WaveIdx = CalculateSlotWaveIndex(i, Settings);
            WaveBuckets.FindOrAdd(WaveIdx).Add(SlotStatuses[i].TicketID);
        }
    }

    if (WaveBuckets.IsEmpty())
    {
        return;
    }

    TArray<int32> SortedWaveIndices;
    WaveBuckets.GetKeys(SortedWaveIndices);
    SortedWaveIndices.Sort();

    PendingSpawnWaves.Empty(SortedWaveIndices.Num());
    for (int32 WaveIdx : SortedWaveIndices)
    {
        PendingSpawnWaves.Add(MoveTemp(WaveBuckets[WaveIdx]));
    }

    if (!FMath::IsFinite(Settings.SpawnInterval) || Settings.SpawnInterval <= 0.0f)
    {
        while (IsValid(this) && !bShuttingDown && Revision == SpawnRevision && !PendingSpawnWaves.IsEmpty())
        {
            ProcessNextSpawnWave();
        }
    }
    else
    {
        ProcessNextSpawnWave();

        if (IsValid(this) && !bShuttingDown && Revision == SpawnRevision && !PendingSpawnWaves.IsEmpty())
        {
            World->GetTimerManager().SetTimer(
                SpawnWaveTimerHandle,
                this,
                &USCStackComponent::ProcessNextSpawnWave,
                Settings.SpawnInterval,
                true);
        }
    }
}

void USCStackComponent::ProcessNextSpawnWave()
{
    UWorld* World = GetWorld();
    if (!IsValid(World) || bShuttingDown || PendingSpawnWaves.IsEmpty())
    {
        return;
    }
    const uint64 Revision = SpawnRevision;
    const FSCStackSpawnSettings Settings = ActiveSpawnSettings;
    TArray<int32> Wave = MoveTemp(PendingSpawnWaves[0]);
    PendingSpawnWaves.RemoveAt(0);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (int32 Ticket : Wave)
    {
        int32 Index = FindIndexByTicket(Ticket);
        if (Index == INDEX_NONE || SlotStatuses[Index].Status != ESCSlotStatus::Filled ||
            ConvertingTickets.Contains(Ticket))
        {
            continue;
        }
        const FTransform Transform = GetSlotWorldTransform(Ticket);
        // SpawnActor runs construction/BeginPlay synchronously; reserve this conversion against reentrant extraction.
        ConvertingTickets.Add(Ticket);
        AActor* Actor =
            IsValid(SpawnActorClass) ? World->SpawnActor<AActor>(SpawnActorClass, Transform, Params) : nullptr;
        ConvertingTickets.Remove(Ticket);
        if (!IsValid(this) || bShuttingDown)
        {
            return;
        }
        if (IsValid(Actor))
        {
            Index = FindIndexByTicket(Ticket);
            if (Index != INDEX_NONE)
            {
                FreeSlot(Index);
            }
            // Commit removal before calling owner code. A failed spawn leaves the ticket and resource intact.
            if (Actor->Implements<USCMessageInterface>())
            {
                FSCMessagePayload Payload;
                Payload.Value = Settings.MessageValue;
                Payload.StringMessage = Settings.MessageNote;
                Payload.Sender = GetOwner();
                Payload.TransformData = Transform;
                ISCMessageInterface::Execute_OnReceiveSCMessage(Actor, Payload);
            }
            if (!IsValid(this) || bShuttingDown || Revision != SpawnRevision)
            {
                return;
            }
            if (IsValid(Actor))
            {
                OnActorSpawned(Actor, Index);
            }
        }
        else
        {
            UE_LOG(LogSCStack, Warning, TEXT("Actor spawn failed for ticket %d; resource retained."), Ticket);
        }
        if (!IsValid(this) || bShuttingDown || Revision != SpawnRevision)
        {
            return;
        }
    }
    if (PendingSpawnWaves.IsEmpty())
    {
        CancelSpawning();
        OnSpawningCompleted();
    }
}

bool USCStackComponent::ExtractSlot(ESCStackExtractionOrder Order, int32& OutTicketID, FTransform& OutTransform)
{
    OutTicketID = INDEX_NONE;

    if (bShuttingDown || SlotStatuses.IsEmpty() || GetTotalCapacity() == 0)
    {
        return false;
    }

    TArray<int32> CandidateSlots;

    if (Order == ESCStackExtractionOrder::RandomFromAll)
    {
        for (int32 i = 0; i < SlotStatuses.Num(); ++i)
        {
            if (SlotStatuses[i].Status == ESCSlotStatus::Filled &&
                !ConvertingTickets.Contains(SlotStatuses[i].TicketID))
            {
                CandidateSlots.Add(i);
            }
        }
    }
    else if (Order == ESCStackExtractionOrder::FromLastAdded)
    {
        // Preserve the existing reverse-sequential extraction strategy.
        for (int32 Index = SlotStatuses.Num() - 1; Index >= 0; --Index)
        {
            const FSCSlotData& Slot = SlotStatuses[Index];
            if (Slot.Status == ESCSlotStatus::Filled && !ConvertingTickets.Contains(Slot.TicketID))
            {
                CandidateSlots.Add(Index);
                break;
            }
        }
    }
    else
    {
        // Top layer strategies
        const int32 SafeRows = FMath::Max(1, Rows);
        const int32 SafeColumns = FMath::Max(1, Columns);
        const int32 ItemsPerLayer = SafeRows * SafeColumns;

        // Find highest layer with at least one filled slot
        int32 HighestLayer = -1;
        for (int32 i = SlotStatuses.Num() - 1; i >= 0; --i)
        {
            if (SlotStatuses[i].Status == ESCSlotStatus::Filled &&
                !ConvertingTickets.Contains(SlotStatuses[i].TicketID))
            {
                HighestLayer = i / ItemsPerLayer;
                break;
            }
        }

        if (HighestLayer >= 0)
        {
            const int32 StartIdx = HighestLayer * ItemsPerLayer;
            const int32 EndIdx = FMath::Min(StartIdx + ItemsPerLayer, SlotStatuses.Num());

            for (int32 i = StartIdx; i < EndIdx; ++i)
            {
                if (SlotStatuses[i].Status == ESCSlotStatus::Filled &&
                    !ConvertingTickets.Contains(SlotStatuses[i].TicketID))
                {
                    CandidateSlots.Add(i);
                }
            }
        }
    }

    if (CandidateSlots.IsEmpty())
    {
        return false;
    }

    if (Order == ESCStackExtractionOrder::FromFirstOnTopLayer || Order == ESCStackExtractionOrder::FromLastAdded)
    {
        OutTicketID = SlotStatuses[CandidateSlots[0]].TicketID;
    }
    else // Random variations
    {
        const int32 RandomIndex = FMath::RandRange(0, CandidateSlots.Num() - 1);
        OutTicketID = SlotStatuses[CandidateSlots[RandomIndex]].TicketID;
    }

    return ExtractSpecificSlot(OutTicketID, OutTransform);
}

bool USCStackComponent::ExtractSpecificSlot(int32 TicketID, FTransform& OutTransform)
{
    const int32 Index = FindIndexByTicket(TicketID);
    if (bShuttingDown || Index == INDEX_NONE || SlotStatuses[Index].Status != ESCSlotStatus::Filled ||
        ConvertingTickets.Contains(TicketID))
    {
        return false;
    }
    OutTransform = GetSlotWorldTransform(TicketID);
    FreeSlot(Index);
    SlotStatuses.RemoveAt(Index);
    SlotStatuses.AddDefaulted();
    RebuildTicketLookup();
    RefreshStackTransforms();
    return true;
}

// ---------------------------------------------------------------------------
// Public API — Getters
// ---------------------------------------------------------------------------

FTransform USCStackComponent::GetSlotWorldTransform(int32 TicketID) const
{
    const int32 Index = FindIndexByTicket(TicketID);
    // Destinations and extracted actors use full element scale, independent of the visibility animation.
    return Index == INDEX_NONE
               ? GetComponentTransform()
               : CalculateDeformedTransform(CalculateSlotGridTransform(Index)) * GetComponentTransform();
}

int32 USCStackComponent::GetFilledSlotCount() const
{
    int32 Count = 0;
    for (const FSCSlotData& Data : SlotStatuses)
    {
        if (Data.Status == ESCSlotStatus::Filled)
        {
            ++Count;
        }
    }
    return Count;
}

int32 USCStackComponent::GetTotalCapacity() const
{
    const int64 Plane = static_cast<int64>(FMath::Max(1, Rows)) * FMath::Max(1, Columns);
    if (Plane > MAX_int32)
    {
        return 0;
    }
    const int64 Capacity = Plane * FMath::Max(1, Layers);
    return Capacity <= MAX_int32 ? static_cast<int32>(Capacity) : 0;
}


// ---------------------------------------------------------------------------
// Public API — Fill Level
// ---------------------------------------------------------------------------

void USCStackComponent::SetFillLevel(float InFillLevel)
{
    if (!FMath::IsFinite(InFillLevel) || bShuttingDown)
    {
        return;
    }
    FillLevel = FMath::Clamp(InFillLevel, 0.f, 1.f);
    if (!bRuntimeInitialized || !IsValid(StackHISM))
    {
        return;
    }
    LastAppliedFillLevel = FillLevel;
    const int32 Target = FMath::RoundToInt(static_cast<double>(FillLevel) * SlotStatuses.Num());
    // Repeated requests compare occupancy, not the last requested value (slots may have been extracted).
    if (Target == TicketToIndex.Num())
    {
        return;
    }
    CancelPendingFill();
    int32 Occupied = TicketToIndex.Num();
    if (Occupied > Target)
    {
        for (int32 Index = SlotStatuses.Num() - 1; Index >= 0 && Occupied > Target; --Index)
        {
            const FSCSlotData& Slot = SlotStatuses[Index];
            if (Slot.Status == ESCSlotStatus::Filled && !ConvertingTickets.Contains(Slot.TicketID))
            {
                FreeSlot(Index);
                --Occupied;
            }
        }
    }
    else
    {
        for (; Occupied < Target; ++Occupied)
        {
            const int32 Ticket = RequestSlot();
            if (Ticket == INDEX_NONE)
            {
                break;
            }
            PendingFillTickets.Enqueue(Ticket);
        }
        ProcessNextPendingSlot();
    }
}

void USCStackComponent::ProcessNextPendingSlot()
{
    const uint64 Revision = FillRevision;
    int32 Ticket = INDEX_NONE;
    while (!bShuttingDown && PendingFillTickets.Dequeue(Ticket))
    {
        StartSlotAnimation(FindIndexByTicket(Ticket));
        if (!IsValid(this) || bShuttingDown || Revision != FillRevision)
        {
            return;
        }
        if (PendingFillTickets.IsEmpty())
        {
            return;
        }
        if (FMath::IsFinite(FillStaggerDelay) && FillStaggerDelay > 0.f)
        {
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(FillStaggerTimerHandle, this,
                                                  &USCStackComponent::ProcessNextPendingSlot, FillStaggerDelay, false);
            }
            return;
        }
    }
}

void USCStackComponent::CancelPendingFill()
{
    ++FillRevision;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FillStaggerTimerHandle);
    }
    int32 Ticket = INDEX_NONE;
    while (PendingFillTickets.Dequeue(Ticket))
    {
        const int32 Index = FindIndexByTicket(Ticket);
        if (Index != INDEX_NONE && SlotStatuses[Index].Status == ESCSlotStatus::Reserved)
        {
            FreeSlot(Index);
        }
    }
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

void USCStackComponent::StartSlotAnimation(int32 SlotIndex)
{
    UWorld* World = GetWorld();
    if (bShuttingDown || !IsValid(World) || !SlotStatuses.IsValidIndex(SlotIndex) ||
        SlotStatuses[SlotIndex].Status != ESCSlotStatus::Reserved)
    {
        return;
    }
    FSCSlotData& Slot = SlotStatuses[SlotIndex];
    Slot.Status = ESCSlotStatus::Filled;
    FSCSlotAnimState& Animation = ActiveAnimations.FindOrAdd(Slot.TicketID);
    Animation.Progress = 0.f;
    Animation.LastUpdateTime = World->GetTimeSeconds();
    if (!World->GetTimerManager().IsTimerActive(AnimationTimerHandle))
    {
        World->GetTimerManager().SetTimer(AnimationTimerHandle, this, &USCStackComponent::TickSlotAnimations,
                                          AnimationTickInterval, true);
    }
}

void USCStackComponent::TickSlotAnimations()
{
    UWorld* World = GetWorld();
    if (!IsValid(World) || !IsValid(StackHISM) || bShuttingDown)
    {
        return;
    }
    const double Now = World->GetTimeSeconds();
    TArray<int32> Completed;
    for (auto It = ActiveAnimations.CreateIterator(); It; ++It)
    {
        const int32 Index = FindIndexByTicket(It.Key());
        if (Index == INDEX_NONE || SlotStatuses[Index].Status != ESCSlotStatus::Filled)
        {
            It.RemoveCurrent();
            continue;
        }
        FSCSlotAnimState& Animation = It.Value();
        const double Elapsed = FMath::Max(0.0, Now - Animation.LastUpdateTime);
        Animation.LastUpdateTime = Now;
        Animation.Progress =
            FMath::IsFinite(ScaleAnimationDuration) && ScaleAnimationDuration > 0.f
                ? FMath::Min(1.f, Animation.Progress + static_cast<float>(Elapsed / ScaleAnimationDuration))
                : 1.f;
        FTransform Transform = CalculateDeformedTransform(CalculateSlotGridTransform(Index));
        Transform.SetScale3D(FMath::Lerp(FVector(0.0001f), Transform.GetScale3D(), Animation.Progress));
        StackHISM->UpdateInstanceTransform(Index, Transform, false, false);
        if (Animation.Progress >= 1.f)
        {
            Completed.Add(It.Key());
            It.RemoveCurrent();
        }
    }
    StackHISM->MarkRenderStateDirty();
    if (ActiveAnimations.IsEmpty())
    {
        ClearAllAnimations();
    }
    // No map references survive user callbacks; earlier completions may extract later ones.
    for (int32 Ticket : Completed)
    {
        if (!IsValid(this) || bShuttingDown)
        {
            return;
        }
        const int32 Index = FindIndexByTicket(Ticket);
        if (Index != INDEX_NONE && SlotStatuses[Index].Status == ESCSlotStatus::Filled)
        {
            OnSlotFilled(Ticket);
        }
    }
}

void USCStackComponent::ClearAllAnimations()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AnimationTimerHandle);
    }
    ActiveAnimations.Empty();
}
