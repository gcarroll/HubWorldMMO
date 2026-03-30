#include "ViewModels/URiftViewModel_Equipment.h"
#include "Components/URiftEquipmentComponent.h"

void URiftViewModel_Equipment::InitializeViewModel_Implementation(const UUserWidget* UserWidget)
{
    bIsSingleton = true;
}

void URiftViewModel_Equipment::ShutdownViewModel_Implementation()
{
    UnbindFromEquipmentComponent();
    EquipmentSlots.Empty();
}

void URiftViewModel_Equipment::SetEquipmentComponent(URiftEquipmentComponent* NewEquipmentComponent)
{
    UnbindFromEquipmentComponent();
    EquipmentComponent = NewEquipmentComponent;
    BindToEquipmentComponent();
    RefreshSlots();
}

TArray<FRiftEquipmentSlotViewEntry> URiftViewModel_Equipment::GetEquipmentSlots() const
{
    return EquipmentSlots;
}

int32 URiftViewModel_Equipment::GetSlotCount() const
{
    return EquipmentSlots.Num();
}

URiftItemInstance* URiftViewModel_Equipment::GetItemInSlot(FGameplayTag SlotTag) const
{
    const FRiftEquipmentSlotViewEntry* Entry = FindSlotByTag(SlotTag);
    return Entry ? Entry->Item.Get() : nullptr;
}

void URiftViewModel_Equipment::OnItemEquipped(URiftItemInstance* Item, FGameplayTag SlotTag, URiftItemDefinition* ItemDefinition)
{
    if (FRiftEquipmentSlotViewEntry* Entry = FindSlotByTag(SlotTag))
    {
        Entry->Item = Item;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEquipmentSlots);
    }
}

void URiftViewModel_Equipment::OnItemUnequipped(URiftItemInstance* Item, FGameplayTag SlotTag, URiftItemDefinition* ItemDefinition)
{
    if (FRiftEquipmentSlotViewEntry* Entry = FindSlotByTag(SlotTag))
    {
        Entry->Item = nullptr;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEquipmentSlots);
    }
}

FRiftEquipmentSlotViewEntry* URiftViewModel_Equipment::FindSlotByTag(FGameplayTag SlotTag)
{
    for (FRiftEquipmentSlotViewEntry& Entry : EquipmentSlots)
    {
        if (Entry.SlotTag == SlotTag)
        {
            return &Entry;
        }
    }
    return nullptr;
}

const FRiftEquipmentSlotViewEntry* URiftViewModel_Equipment::FindSlotByTag(FGameplayTag SlotTag) const
{
    for (const FRiftEquipmentSlotViewEntry& Entry : EquipmentSlots)
    {
        if (Entry.SlotTag == SlotTag)
        {
            return &Entry;
        }
    }
    return nullptr;
}

void URiftViewModel_Equipment::BindToEquipmentComponent()
{
    if (!EquipmentComponent.IsValid())
    {
        return;
    }

    EquipmentComponent->OnItemEquipped.AddDynamic(this, &URiftViewModel_Equipment::OnItemEquipped);
    EquipmentComponent->OnItemUnequipped.AddDynamic(this, &URiftViewModel_Equipment::OnItemUnequipped);
}

void URiftViewModel_Equipment::UnbindFromEquipmentComponent()
{
    if (!EquipmentComponent.IsValid())
    {
        return;
    }

    EquipmentComponent->OnItemEquipped.RemoveDynamic(this, &URiftViewModel_Equipment::OnItemEquipped);
    EquipmentComponent->OnItemUnequipped.RemoveDynamic(this, &URiftViewModel_Equipment::OnItemUnequipped);
}

void URiftViewModel_Equipment::RefreshSlots()
{
    EquipmentSlots.Empty();

    if (!EquipmentComponent.IsValid())
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEquipmentSlots);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSlotCount);
        return;
    }

    for (const FGameplayTag& SlotTag : EquipmentComponent->GetSupportedSlots())
    {
        FRiftEquipmentSlotViewEntry Entry;
        Entry.SlotTag = SlotTag;
        Entry.Item    = EquipmentComponent->GetItemInSlot(SlotTag);
        EquipmentSlots.Add(Entry);
    }

    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEquipmentSlots);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSlotCount);
}
