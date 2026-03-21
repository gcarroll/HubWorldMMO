#include "GameFramework/Containers/URiftContainer.h"

#include "Net/UnrealNetwork.h"

URiftContainer::URiftContainer(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URiftContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(URiftContainer, ContainerDefinition);
    DOREPLIFETIME(URiftContainer, Items);
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
    for (const TObjectPtr<URiftItemInstance>& Item : Items)
    {
        if (IsValid(Item))
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
    for (const TObjectPtr<URiftItemInstance>& Item : Items)
    {
        if (IsValid(Item))
        {
            Result.Add(Item.Get());
        }
    }
    return Result;
}

URiftItemInstance* URiftContainer::GetItemAtSlot(const int32 SlotIndex) const
{
    if (Items.IsValidIndex(SlotIndex))
    {
        return Items[SlotIndex].Get();
    }
    return nullptr;
}

int32 URiftContainer::GetSlotIndexOfItem(const URiftItemInstance* Item) const
{
    if (!IsValid(Item))
    {
        return INDEX_NONE;
    }

    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (Items[i] == Item)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 URiftContainer::AddItem(URiftItemInstance* Item)
{
    if (!IsValid(Item) || IsFull())
    {
        return INDEX_NONE;
    }

    // Find the first null slot and fill it
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (!IsValid(Items[i]))
        {
            Items[i] = Item;
            return i;
        }
    }

    // No null slots found — append if under capacity
    if (Items.Num() < GetCapacity())
    {
        return Items.Add(Item);
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

    Items[SlotIndex] = nullptr;
    return true;
}

bool URiftContainer::MoveItemToSlot(URiftItemInstance* Item, const int32 TargetSlotIndex)
{
    if (!IsValid(Item) || !Items.IsValidIndex(TargetSlotIndex))
    {
        return false;
    }

    const int32 SourceSlotIndex = GetSlotIndexOfItem(Item);
    if (SourceSlotIndex == INDEX_NONE)
    {
        return false;
    }

    // Swap source and target slots
    TObjectPtr<URiftItemInstance> TargetItem = Items[TargetSlotIndex];
    Items[TargetSlotIndex] = Item;
    Items[SourceSlotIndex] = TargetItem;
    return true;
}

void URiftContainer::SetDefinition(URiftContainerDefinition* NewDefinition)
{
    ContainerDefinition = NewDefinition;

    // Pre-allocate the Items array to capacity so slots exist from the start
    if (IsValid(ContainerDefinition))
    {
        Items.SetNum(ContainerDefinition->GetCapacity());
    }
}

void URiftContainer::SetContainerId(const FGuid& NewContainerId)
{
    ContainerId = NewContainerId;
}
