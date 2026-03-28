#include "Components/URiftMutableEquipmentComponent.h"
#include "Components/URiftEquipmentComponent.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "GameFramework/Fragments/URiftFragment_Equippable.h"
#include "Data/URiftItemDefinition.h"
#include "Tags/RiftVaultTags.h"
#include "MuCO/CustomizableSkeletalComponent.h"
#include "MuCO/CustomizableObjectInstance.h"

URiftMutableEquipmentComponent::URiftMutableEquipmentComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URiftMutableEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    BodyMeshComponent = FindBodyMeshComp();

    // Clone the CO instance so each character gets its own independent Mutable state.
    // Without this, all characters share the default instance on the CO asset, meaning
    // equipping an item on one character visually affects all others (multiplayer bug),
    // and the instance retains stale state from the previous PIE session in the editor.
    if (UCustomizableSkeletalComponent* BodyMesh = BodyMeshComponent.Get())
    {
        if (UCustomizableObjectInstance* ExistingInstance = BodyMesh->GetCustomizableObjectInstance())
        {
            UCustomizableObjectInstance* UniqueInstance = ExistingInstance->Clone();
            if (IsValid(UniqueInstance))
            {
                BodyMesh->SetCustomizableObjectInstance(UniqueInstance);
                UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent::BeginPlay — Cloned CO instance for %s."), *GetOwner()->GetName());
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent::BeginPlay — Owner: %s, BodyMeshComponent: %s"),
        *GetOwner()->GetName(),
        BodyMeshComponent.IsValid() ? *BodyMeshComponent->GetName() : TEXT("NULL"));

    if (URiftEquipmentComponent* EquipComp = GetOwner()->FindComponentByClass<URiftEquipmentComponent>())
    {
        EquipComp->OnItemEquipped.AddDynamic(this, &URiftMutableEquipmentComponent::OnItemEquipped);
        EquipComp->OnItemUnequipped.AddDynamic(this, &URiftMutableEquipmentComponent::OnItemUnequipped);
        UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent::BeginPlay — Bound to URiftEquipmentComponent."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent: No URiftEquipmentComponent found on %s."), *GetOwner()->GetName());
    }
}

void URiftMutableEquipmentComponent::OnItemEquipped(URiftItemInstance* Item, FGameplayTag SlotTag, URiftItemDefinition* ItemDefinition)
{
    UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent::OnItemEquipped — Item: %s, Slot: %s, Def: %s"),
        IsValid(Item) ? *Item->GetName() : TEXT("NULL"),
        *SlotTag.ToString(),
        IsValid(ItemDefinition) ? *ItemDefinition->GetName() : TEXT("NULL"));

    // Resolve the fragment from the item instance (server / owning client) or the
    // definition passed directly in the delegate (simulated proxies). The definition is
    // passed from the slot entry itself so it is always the correct one for this event —
    // never looked up from the live array, which can be in mid-swap state when FastArray
    // fires additions before removals in the same replication packet.
    const URiftFragment_Equippable* Fragment = nullptr;
    if (IsValid(Item))
    {
        Fragment = Item->FindFragment<URiftFragment_Equippable>();
    }
    else if (IsValid(ItemDefinition))
    {
        Fragment = ItemDefinition->FindFragment<URiftFragment_Equippable>();
    }

    UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent::OnItemEquipped — Fragment: %s, Param count: %d"),
        Fragment ? TEXT("found") : TEXT("NULL"),
        Fragment ? Fragment->GetActiveMutableParameters().Num() : -1);

    if (Fragment && Fragment->GetActiveMutableParameters().Num() > 0)
    {
        ApplyMutableParameters(Fragment->GetActiveMutableParameters());
    }
}

void URiftMutableEquipmentComponent::OnItemUnequipped(URiftItemInstance* Item, FGameplayTag SlotTag, URiftItemDefinition* ItemDefinition)
{
    UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent::OnItemUnequipped — Item: %s, Slot: %s, Def: %s"),
        IsValid(Item) ? *Item->GetName() : TEXT("NULL"),
        *SlotTag.ToString(),
        IsValid(ItemDefinition) ? *ItemDefinition->GetName() : TEXT("NULL"));

    const URiftFragment_Equippable* Fragment = nullptr;
    if (IsValid(Item))
    {
        Fragment = Item->FindFragment<URiftFragment_Equippable>();
    }
    else if (IsValid(ItemDefinition))
    {
        Fragment = ItemDefinition->FindFragment<URiftFragment_Equippable>();
    }

    if (Fragment && Fragment->GetActiveMutableParameters().Num() > 0)
    {
        ResetMutableParameters(Fragment->GetActiveMutableParameters());
    }
}

void URiftMutableEquipmentComponent::ApplyMutableParameters(const TArray<FRiftMutableParameter>& Parameters)
{
    UCustomizableObjectInstance* Instance = GetObjectInstance();
    UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent::ApplyMutableParameters — Instance: %s, ParamCount: %d"),
        Instance ? *Instance->GetName() : TEXT("NULL"),
        Parameters.Num());

    if (!Instance)
    {
        return;
    }

    // NOTE: Verify exact Mutable API method names for your UE5 version.
    // If string-name overloads are unavailable, use FindIntParameterNameIndex(Name)
    // to get the parameter index and pass it to the index-based setter instead.
    for (const FRiftMutableParameter& Param : Parameters)
    {
        switch (Param.ParameterType)
        {
        case ERiftMutableParameterType::Int32:
            UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent: Setting Int param '%s' = '%s'"),
                *Param.ParameterName.ToString(), *Param.IntOptionName);
            Instance->SetIntParameterSelectedOption(Param.ParameterName.ToString(), Param.IntOptionName);
            break;
        case ERiftMutableParameterType::Float:
            UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent: Setting Float param '%s' = %f"),
                *Param.ParameterName.ToString(), Param.FloatValue);
            Instance->SetFloatParameterSelectedOption(Param.ParameterName.ToString(), Param.FloatValue);
            break;
        case ERiftMutableParameterType::Color:
            UE_LOG(LogTemp, Warning, TEXT("URiftMutableEquipmentComponent: Setting Color param '%s'"),
                *Param.ParameterName.ToString());
            Instance->SetColorParameterSelectedOption(Param.ParameterName.ToString(), Param.ColorValue);
            break;
        }
    }

    ScheduleMutableUpdate();
}

void URiftMutableEquipmentComponent::ResetMutableParameters(const TArray<FRiftMutableParameter>& Parameters)
{
    UCustomizableObjectInstance* Instance = GetObjectInstance();
    if (!Instance)
    {
        return;
    }

    for (const FRiftMutableParameter& Param : Parameters)
    {
        switch (Param.ParameterType)
        {
        case ERiftMutableParameterType::Int32:
            // Pass "None" to revert to the Mutable object's default/empty option.
            Instance->SetIntParameterSelectedOption(Param.ParameterName.ToString(), FString(TEXT("None")));
            break;
        case ERiftMutableParameterType::Float:
            Instance->SetFloatParameterSelectedOption(Param.ParameterName.ToString(), 0.f);
            break;
        case ERiftMutableParameterType::Color:
            Instance->SetColorParameterSelectedOption(Param.ParameterName.ToString(), FLinearColor::White);
            break;
        }
    }

    ScheduleMutableUpdate();
}

void URiftMutableEquipmentComponent::ScheduleMutableUpdate()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            MutableUpdateTimer,
            this,
            &URiftMutableEquipmentComponent::FlushMutableUpdate,
            KINDA_SMALL_NUMBER,
            false);
    }
}

void URiftMutableEquipmentComponent::FlushMutableUpdate()
{
    if (UCustomizableObjectInstance* Instance = GetObjectInstance())
    {
        Instance->UpdateSkeletalMeshAsync();
    }
}

UCustomizableSkeletalComponent* URiftMutableEquipmentComponent::FindBodyMeshComp() const
{
    APawn* OwningPawn = GetOwner<APawn>();
    if (!OwningPawn)
    {
        return nullptr;
    }

    const FName BodyMeshTagName = Tag_Rift_Component_BodyMesh.GetTag().GetTagName();
    TArray<UCustomizableSkeletalComponent*> MutableComps;
    OwningPawn->GetComponents<UCustomizableSkeletalComponent>(MutableComps);

    for (UCustomizableSkeletalComponent* Comp : MutableComps)
    {
        if (Comp->ComponentTags.Contains(BodyMeshTagName))
        {
            return Comp;
        }
    }

    // Tag not found — fall back to the first available Mutable component so the
    // equipment component works out-of-the-box without requiring the tag to be set.
    if (MutableComps.Num() > 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("URiftMutableEquipmentComponent: No component tagged '%s' found on %s — falling back to first UCustomizableSkeletalComponent ('%s'). Add the tag to silence this warning."),
            *BodyMeshTagName.ToString(),
            *GetOwner()->GetName(),
            *MutableComps[0]->GetName());
        return MutableComps[0];
    }

    return nullptr;
}

UCustomizableObjectInstance* URiftMutableEquipmentComponent::GetObjectInstance() const
{
    if (UCustomizableSkeletalComponent* BodyMesh = BodyMeshComponent.Get())
    {
        return BodyMesh->GetCustomizableObjectInstance();
    }
    return nullptr;
}
