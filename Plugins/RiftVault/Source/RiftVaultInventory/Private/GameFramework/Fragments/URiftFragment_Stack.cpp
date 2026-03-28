#include "GameFramework/Fragments/URiftFragment_Stack.h"

#include "Components/URiftInventoryComponent.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "Tags/RiftVaultTags.h"

#if ENGINE_MINOR_VERSION < 5
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif

URiftFragment_Stack::URiftFragment_Stack()
{
    // Contribute the stackable trait tag so the inventory component and UI
    // can quickly identify whether an item supports stacking without needing
    // to find the fragment explicitly.
    FragmentTags.AddTagFast(Tag_Rift_Item_Trait_Stackable);
}

void URiftFragment_Stack::InitializeState_Implementation(URiftItemInstance* Item)
{
    if (!IsValid(Item))
    {
        return;
    }

    // Create a fresh stack state for this instance. Quantity starts at 1 —
    // the caller (URiftInventoryComponent::AddItem) is responsible for calling
    // AddQuantity if more units are being added at creation time.
    TInstancedStruct<FRiftFragmentState> StateWrapper;
    StateWrapper.InitializeAs<FRiftStackState>();

    FRiftStackState& State = StateWrapper.GetMutable<FRiftStackState>();
    State.CurrentQuantity = 1;
    State.LastQuantity = 0;

    Item->SaveStateForFragment(StaticClass(), StateWrapper);
}

int32 URiftFragment_Stack::GetCurrentQuantity(const URiftItemInstance* Item) const
{
    FRiftStackState State;
    if (!GetStackState(Item, State))
    {
        return 0;
    }
    return State.CurrentQuantity;
}

int32 URiftFragment_Stack::GetAvailableSpace(const URiftItemInstance* Item) const
{
    FRiftStackState State;
    if (!GetStackState(Item, State))
    {
        return 0;
    }
    return FMath::Max(0, MaxStackSize - State.CurrentQuantity);
}

bool URiftFragment_Stack::IsFull(const URiftItemInstance* Item) const
{
    return GetAvailableSpace(Item) == 0;
}

bool URiftFragment_Stack::CanMergeWith(const URiftItemInstance* Source, const URiftItemInstance* Target) const
{
    if (!IsValid(Source) || !IsValid(Target))
    {
        return false;
    }

    // Items must be the same type to merge.
    if (!Source->IsSameType(Target))
    {
        return false;
    }

    // Target must have room.
    return !IsFull(Target);
}

int32 URiftFragment_Stack::AddQuantity(URiftItemInstance* Item, int32 Amount)
{
    if (!ItemHasAuthority(Item) || Amount <= 0 || !IsValid(Item))
    {
        return Amount;
    }

    FRiftStackState State;
    if (!GetStackState(Item, State))
    {
        return Amount;
    }

    const int32 Available = FMath::Max(0, MaxStackSize - State.CurrentQuantity);
    const int32 ToAdd = FMath::Min(Amount, Available);
    const int32 Overflow = Amount - ToAdd;

    if (ToAdd > 0)
    {
        State.CurrentQuantity += ToAdd;

        TInstancedStruct<FRiftFragmentState> StateWrapper;
        StateWrapper.InitializeAs<FRiftStackState>(State);
        Item->SaveStateForFragment(StaticClass(), StateWrapper);

        // Notify fragments and external systems that the stack size changed.
        if (URiftInventoryComponent* Inventory = Cast<URiftInventoryComponent>(Item->GetOuter()))
        {
            Inventory->BroadcastItemEvent(Item, Tag_Rift_Event_Item_StackChanged);
        }
    }

    return Overflow;
}

int32 URiftFragment_Stack::RemoveQuantity(URiftItemInstance* Item, int32 Amount)
{
    if (!ItemHasAuthority(Item) || Amount <= 0 || !IsValid(Item))
    {
        return 0;
    }

    FRiftStackState State;
    if (!GetStackState(Item, State))
    {
        return 0;
    }

    // Never go below 1 — the inventory component destroys the item when it
    // should reach zero. This method only handles the decrement.
    const int32 ToRemove = FMath::Min(Amount, State.CurrentQuantity - 1);

    if (ToRemove > 0)
    {
        State.CurrentQuantity -= ToRemove;

        TInstancedStruct<FRiftFragmentState> StateWrapper;
        StateWrapper.InitializeAs<FRiftStackState>(State);
        Item->SaveStateForFragment(StaticClass(), StateWrapper);

        // Notify fragments and external systems that the stack size changed.
        if (URiftInventoryComponent* Inventory = Cast<URiftInventoryComponent>(Item->GetOuter()))
        {
            Inventory->BroadcastItemEvent(Item, Tag_Rift_Event_Item_StackChanged);
        }
    }

    return ToRemove;
}

bool URiftFragment_Stack::GetStackState(const URiftItemInstance* Item, FRiftStackState& OutState) const
{
    if (!IsValid(Item))
    {
        return false;
    }

    TInstancedStruct<FRiftFragmentState> StateWrapper;
    if (!Item->GetStateForFragment(StaticClass(), StateWrapper))
    {
        return false;
    }

    const FRiftStackState* StatePtr = StateWrapper.GetPtr<FRiftStackState>();
    if (!StatePtr)
    {
        return false;
    }

    OutState = *StatePtr;
    return true;
}
