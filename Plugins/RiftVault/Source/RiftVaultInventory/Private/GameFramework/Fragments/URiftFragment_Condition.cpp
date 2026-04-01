#include "GameFramework/Fragments/URiftFragment_Condition.h"

#include "Components/URiftInventoryComponent.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "Tags/RiftVaultTags.h"

#if ENGINE_MINOR_VERSION < 5
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif

URiftFragment_Condition::URiftFragment_Condition()
{
    FragmentTags.AddTagFast(Tag_Rift_Item_Trait_HasCondition);
    WatchedEventTags.AddTagFast(Tag_Rift_Event_Item_ConditionChanged);
}

void URiftFragment_Condition::InitializeState_Implementation(URiftItemInstance* Item)
{
    if (!IsValid(Item))
    {
        return;
    }

    TInstancedStruct<FRiftFragmentState> StateWrapper;
    StateWrapper.InitializeAs<FRiftConditionState>();

    FRiftConditionState& State = StateWrapper.GetMutable<FRiftConditionState>();
    State.CurrentCondition = bStartAtFullCondition ? MaxCondition : 0.f;
    State.bIsBroken        = !bStartAtFullCondition;
    State.LastCondition    = 0.f;

    Item->SaveStateForFragment(StaticClass(), StateWrapper);
}

float URiftFragment_Condition::GetCurrentCondition(const URiftItemInstance* Item) const
{
    FRiftConditionState State;
    return GetConditionState(Item, State) ? State.CurrentCondition : 0.f;
}

float URiftFragment_Condition::GetConditionPercent(const URiftItemInstance* Item) const
{
    if (MaxCondition <= 0.f)
    {
        return 0.f;
    }
    return FMath::Clamp(GetCurrentCondition(Item) / MaxCondition, 0.f, 1.f);
}

bool URiftFragment_Condition::IsBroken(const URiftItemInstance* Item) const
{
    FRiftConditionState State;
    return GetConditionState(Item, State) ? State.bIsBroken : false;
}

void URiftFragment_Condition::ApplyDamage(URiftItemInstance* Item, float Amount)
{
    if (!ItemHasAuthority(Item) || Amount <= 0.f || !IsValid(Item))
    {
        return;
    }

    FRiftConditionState State;
    if (!GetConditionState(Item, State))
    {
        return;
    }

    if (State.bIsBroken)
    {
        return;
    }

    State.CurrentCondition = FMath::Max(0.f, State.CurrentCondition - Amount);
    if (State.CurrentCondition <= 0.f)
    {
        State.bIsBroken = true;
    }

    SaveConditionState(Item, State);
}

void URiftFragment_Condition::Repair(URiftItemInstance* Item, float Amount)
{
    if (!ItemHasAuthority(Item) || Amount <= 0.f || !IsValid(Item))
    {
        return;
    }

    FRiftConditionState State;
    if (!GetConditionState(Item, State))
    {
        return;
    }

    State.CurrentCondition = FMath::Min(MaxCondition, State.CurrentCondition + Amount);
    if (State.CurrentCondition > 0.f)
    {
        State.bIsBroken = false;
    }

    SaveConditionState(Item, State);
}

bool URiftFragment_Condition::GetConditionState(const URiftItemInstance* Item, FRiftConditionState& OutState) const
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

    const FRiftConditionState* StatePtr = StateWrapper.GetPtr<FRiftConditionState>();
    if (!StatePtr)
    {
        return false;
    }

    OutState = *StatePtr;
    return true;
}

void URiftFragment_Condition::SaveConditionState(URiftItemInstance* Item, const FRiftConditionState& State) const
{
    TInstancedStruct<FRiftFragmentState> StateWrapper;
    StateWrapper.InitializeAs<FRiftConditionState>(State);
    Item->SaveStateForFragment(StaticClass(), StateWrapper);

    if (URiftInventoryComponent* Inventory = Cast<URiftInventoryComponent>(Item->GetOuter()))
    {
        Inventory->BroadcastItemEvent(Item, Tag_Rift_Event_Item_ConditionChanged);
    }
}
