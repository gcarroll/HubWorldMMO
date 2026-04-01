#include "GameFramework/Containers/URiftContainer.h"

#include "Components/URiftInventoryComponent.h"
#include "GameFramework/Fragments/URiftFragment_Stack.h"
#include "Net/UnrealNetwork.h"

URiftContainer::URiftContainer(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Slots.Owner = this;
}

void URiftContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(URiftContainer, ContainerDefinition, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(URiftContainer, Slots,              COND_OwnerOnly);
}

// ------------------------------------------------------------------
// FRiftSlotList fast-array callbacks
// ------------------------------------------------------------------

void FRiftSlotList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
    URiftInventoryComponent* InventoryComp = Owner ? Cast<URiftInventoryComponent>(Owner->GetOuter()) : nullptr;

    for (const int32 Idx : AddedIndices)
    {
        FRiftSlotEntry& Entry = Entries[Idx];

        // Empty slot — nothing to reconstruct.
        if (!IsValid(Entry.ItemDefinition) || Entry.Quantity <= 0)
        {
            continue;
        }

        if (!IsValid(InventoryComp))
        {
            continue;
        }

        URiftItemInstance* NewItem = InventoryComp->ReconstructItemInstance(Entry.ItemDefinition.Get(), Entry.Quantity);
        if (IsValid(NewItem))
        {
            Entry.Item = NewItem;
            InventoryComp->OnItemAdded.Broadcast(NewItem, Owner);
        }
    }
}

void FRiftSlotList::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
    URiftInventoryComponent* InventoryComp = Owner ? Cast<URiftInventoryComponent>(Owner->GetOuter()) : nullptr;

    // Process removes before adds so OnItemRemoved fires before OnItemAdded for the same slot
    // (e.g. same-container swap: old item removed, new item added in one replication frame).
    TArray<URiftItemInstance*, TInlineAllocator<8>> RemovedItems;
    TArray<TPair<URiftItemInstance*, URiftContainer*>, TInlineAllocator<8>> AddedItems;

    for (const int32 Idx : ChangedIndices)
    {
        FRiftSlotEntry& Entry = Entries[Idx];

        const bool bSlotNowEmpty = !IsValid(Entry.ItemDefinition) || Entry.Quantity <= 0;

        if (bSlotNowEmpty)
        {
            // Slot was cleared.
            if (IsValid(Entry.Item))
            {
                RemovedItems.Add(Entry.Item.Get());
                if (IsValid(InventoryComp)) { InventoryComp->EvictItemFromCache(Entry.Item.Get()); }
                Entry.Item = nullptr;
            }
            continue;
        }

        // Slot has content. Check whether the definition changed (item swapped) or just quantity changed.
        if (IsValid(Entry.Item))
        {
            if (Entry.Item->GetDefinition() != Entry.ItemDefinition.Get())
            {
                // Definition changed — evict old, reconstruct new.
                RemovedItems.Add(Entry.Item.Get());
                if (IsValid(InventoryComp)) { InventoryComp->EvictItemFromCache(Entry.Item.Get()); }
                Entry.Item = nullptr;

                if (IsValid(InventoryComp))
                {
                    URiftItemInstance* NewItem = InventoryComp->ReconstructItemInstance(Entry.ItemDefinition.Get(), Entry.Quantity);
                    if (IsValid(NewItem))
                    {
                        Entry.Item = NewItem;
                        AddedItems.Add(TPair<URiftItemInstance*, URiftContainer*>(NewItem, Owner));
                    }
                }
            }
            else
            {
                // Same definition — update quantity only.
                if (IsValid(InventoryComp))
                {
                    InventoryComp->UpdateItemInstanceQuantity(Entry.Item.Get(), Entry.Quantity);
                }
            }
        }
        else
        {
            // Slot previously had no item cached (e.g. first descriptor arrived with Qty=0,
            // now the full descriptor is here). Reconstruct fresh.
            if (IsValid(InventoryComp))
            {
                URiftItemInstance* NewItem = InventoryComp->ReconstructItemInstance(Entry.ItemDefinition.Get(), Entry.Quantity);
                if (IsValid(NewItem))
                {
                    Entry.Item = NewItem;
                    AddedItems.Add(TPair<URiftItemInstance*, URiftContainer*>(NewItem, Owner));
                }
            }
        }
    }

    if (IsValid(InventoryComp))
    {
        for (URiftItemInstance* Item : RemovedItems)
        {
            InventoryComp->OnItemRemoved.Broadcast(Item, Owner);
        }
        for (const TPair<URiftItemInstance*, URiftContainer*>& Pair : AddedItems)
        {
            InventoryComp->OnItemAdded.Broadcast(Pair.Key, Pair.Value);
        }
    }
}

void FRiftSlotList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
    URiftInventoryComponent* InventoryComp = Owner ? Cast<URiftInventoryComponent>(Owner->GetOuter()) : nullptr;
    if (!IsValid(InventoryComp))
    {
        return;
    }

    for (const int32 Idx : RemovedIndices)
    {
        FRiftSlotEntry& Entry = Entries[Idx];
        if (IsValid(Entry.Item))
        {
            InventoryComp->EvictItemFromCache(Entry.Item.Get());
            InventoryComp->OnItemRemoved.Broadcast(Entry.Item.Get(), Owner);
        }
    }
}

bool FRiftSlotList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
{
    return FFastArraySerializer::FastArrayDeltaSerialize<FRiftSlotEntry, FRiftSlotList>(Entries, DeltaParams, *this);
}

FGameplayTag URiftContainer::GetContainerTag() const
{
    return IsValid(ContainerDefinition) ? ContainerDefinition->GetContainerTag() : FGameplayTag::EmptyTag;
}

int32 URiftContainer::GetCapacity() const
{
    return IsValid(ContainerDefinition) ? ContainerDefinition->GetCapacity() : 0;
}

int32 URiftContainer::CountItems() const
{
    int32 Count = 0;
    for (const FRiftSlotEntry& Entry : Slots.Entries)
    {
        if (IsValid(Entry.Item))
        {
            ++Count;
        }
    }
    return Count;
}

int32 URiftContainer::CountAvailableSlots() const
{
    return GetCapacity() - CountItems();
}

bool URiftContainer::HasAvailableSlot() const
{
    return CountAvailableSlots() > 0;
}

bool URiftContainer::IsFull() const
{
    return CountAvailableSlots() <= 0;
}

bool URiftContainer::HasItem(const URiftItemInstance* Item) const
{
    return GetSlotIndexOfItem(Item) != INDEX_NONE;
}

bool URiftContainer::CanAcceptItem(const URiftItemInstance* Item) const
{
    if (!IsValid(Item) || !IsValid(ContainerDefinition))
    {
        return false;
    }

    if (IsFull())
    {
        return false;
    }

    // Check the item's tags against the container's compatibility query
    FGameplayTagContainer ItemTags;
    Item->GetOwnedGameplayTags(ItemTags);
    return ContainerDefinition->AcceptsItemWithTags(ItemTags);
}

TArray<URiftItemInstance*> URiftContainer::GetAllItems() const
{
    TArray<URiftItemInstance*> Result;
    for (const FRiftSlotEntry& Entry : Slots.Entries)
    {
        if (IsValid(Entry.Item))
        {
            Result.Add(Entry.Item.Get());
        }
    }
    return Result;
}

URiftItemInstance* URiftContainer::GetItemAtSlot(const int32 SlotIndex) const
{
    if (Slots.Entries.IsValidIndex(SlotIndex))
    {
        return Slots.Entries[SlotIndex].Item.Get();
    }
    return nullptr;
}

int32 URiftContainer::GetSlotIndexOfItem(const URiftItemInstance* Item) const
{
    if (!IsValid(Item))
    {
        return INDEX_NONE;
    }

    for (int32 i = 0; i < Slots.Entries.Num(); ++i)
    {
        if (Slots.Entries[i].Item == Item)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

void URiftContainer::RefreshSlotEntry(const int32 SlotIndex, URiftItemInstance* Item)
{
    if (!Slots.Entries.IsValidIndex(SlotIndex))
    {
        return;
    }

    FRiftSlotEntry& Entry = Slots.Entries[SlotIndex];
    Entry.Item           = Item;
    Entry.ItemDefinition = IsValid(Item) ? Item->GetDefinition() : nullptr;
    Entry.Quantity       = IsValid(Item) ? Item->GetCurrentQuantity() : 0;
    Slots.MarkItemDirty(Entry);
}

int32 URiftContainer::AddItem(URiftItemInstance* Item)
{
    if (!IsValid(Item) || IsFull())
    {
        return INDEX_NONE;
    }

    // Find the first empty slot and fill it.
    for (int32 i = 0; i < Slots.Entries.Num(); ++i)
    {
        if (!IsValid(Slots.Entries[i].Item))
        {
            Slots.Entries[i].Item           = Item;
            Slots.Entries[i].ItemDefinition = Item->GetDefinition();
            Slots.Entries[i].Quantity       = 0;  // placeholder; URiftInventoryComponent calls RefreshSlotDescriptor after full init
            Slots.MarkItemDirty(Slots.Entries[i]);
            return i;
        }
    }

    // No empty slots found — append if under capacity.
    if (Slots.Entries.Num() < GetCapacity())
    {
        FRiftSlotEntry NewEntry;
        NewEntry.Item           = Item;
        NewEntry.ItemDefinition = Item->GetDefinition();
        NewEntry.Quantity       = 0;  // placeholder
        const int32 Idx = Slots.Entries.Add(NewEntry);
        Slots.MarkItemDirty(Slots.Entries[Idx]);
        return Idx;
    }

    return INDEX_NONE;
}

bool URiftContainer::RemoveItem(URiftItemInstance* Item)
{
    const int32 SlotIndex = GetSlotIndexOfItem(Item);
    if (SlotIndex == INDEX_NONE)
    {
        return false;
    }

    const bool bIsList = IsValid(ContainerDefinition)
        && ContainerDefinition->GetLayoutType() == EContainerLayoutType::List;

    if (bIsList)
    {
        // List mode: collapse the array — no empty gaps, order is preserved.
        Slots.Entries.RemoveAt(SlotIndex);
        Slots.MarkArrayDirty();
    }
    else
    {
        // Grid mode: null the slot to preserve slot identity (visual position).
        RefreshSlotEntry(SlotIndex, nullptr);
    }
    return true;
}

bool URiftContainer::MoveItemToSlot(URiftItemInstance* Item, const int32 TargetSlotIndex)
{
    if (!IsValid(Item) || !Slots.Entries.IsValidIndex(TargetSlotIndex))
    {
        return false;
    }

    const int32 SourceSlotIndex = GetSlotIndexOfItem(Item);
    if (SourceSlotIndex == INDEX_NONE)
    {
        return false;
    }

    // Per-slot tag requirements: reject if the incoming item doesn't belong in the target
    // slot, or if the displaced item doesn't belong in the source slot.
    if (SlotTagRequirements.IsValidIndex(TargetSlotIndex) && SlotTagRequirements[TargetSlotIndex].IsValid())
    {
        FGameplayTagContainer ItemTags;
        Item->GetOwnedGameplayTags(ItemTags);
        if (!ItemTags.HasTag(SlotTagRequirements[TargetSlotIndex]))
        {
            return false;
        }
    }
    const URiftItemInstance* DisplacedItem = Slots.Entries[TargetSlotIndex].Item.Get();
    if (IsValid(DisplacedItem) && SlotTagRequirements.IsValidIndex(SourceSlotIndex) && SlotTagRequirements[SourceSlotIndex].IsValid())
    {
        FGameplayTagContainer DisplacedTags;
        DisplacedItem->GetOwnedGameplayTags(DisplacedTags);
        if (!DisplacedTags.HasTag(SlotTagRequirements[SourceSlotIndex]))
        {
            return false;
        }
    }

    // Swap source and target slots.
    TObjectPtr<URiftItemInstance> TargetItem = Slots.Entries[TargetSlotIndex].Item;
    RefreshSlotEntry(TargetSlotIndex, Item);
    RefreshSlotEntry(SourceSlotIndex, TargetItem.Get());
    return true;
}

bool URiftContainer::PlaceItemAtSlot(URiftItemInstance* Item, const int32 SlotIndex)
{
    if (!IsValid(Item) || !Slots.Entries.IsValidIndex(SlotIndex))
    {
        return false;
    }

    RefreshSlotEntry(SlotIndex, Item);
    return true;
}

void URiftContainer::SetDefinition(URiftContainerDefinition* NewDefinition)
{
    ContainerDefinition = NewDefinition;

    if (!IsValid(ContainerDefinition))
    {
        return;
    }

    if (ContainerDefinition->GetLayoutType() == EContainerLayoutType::Grid)
    {
        // Grid: pre-allocate to capacity so slot identity is stable across replication.
        // Empty slots are preserved as null entries — their index is their visual position.
        Slots.Entries.SetNum(ContainerDefinition->GetCapacity());
    }
    // List: don't pre-allocate. Slots grow as items are added up to capacity.
}

void URiftContainer::SetContainerId(const FGuid& NewContainerId)
{
    ContainerId = NewContainerId;
}
