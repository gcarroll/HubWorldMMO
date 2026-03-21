#include "GameFramework/Items/URiftItemInstance.h"

#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Data/URiftItemDefinition.h"
#include "GameFramework/Fragments/URiftItemFragment.h"

// ------------------------------------------------------------------
// FRiftFragmentStateList
// ------------------------------------------------------------------

bool FRiftFragmentStateList::HasStateForFragment(const TSubclassOf<URiftItemFragment>& FragmentClass) const
{
    return FragmentClassMap.Contains(FragmentClass);
}

bool FRiftFragmentStateList::GetStateForFragment(const TSubclassOf<URiftItemFragment>& FragmentClass, TInstancedStruct<FRiftFragmentState>& OutState) const
{
    const int32* Index = FragmentClassMap.Find(FragmentClass);
    if (Index && Entries.IsValidIndex(*Index))
    {
        OutState = Entries[*Index].State;
        return true;
    }
    return false;
}

int32 FRiftFragmentStateList::SaveStateForFragment(const TSubclassOf<URiftItemFragment>& FragmentClass, const TInstancedStruct<FRiftFragmentState>& State)
{
    const int32* ExistingIndex = FragmentClassMap.Find(FragmentClass);
    if (ExistingIndex && Entries.IsValidIndex(*ExistingIndex))
    {
        // Update existing entry
        FRiftStoredFragmentStateEntry& Entry = Entries[*ExistingIndex];
        Entry.State = State;
        Entry.StateTimestamp = ItemOwner ? ItemOwner->GetWorld()->GetTimeSeconds() : 0.f;
        MarkItemDirty(Entry);
        return *ExistingIndex;
    }

    // Add new entry
    FRiftStoredFragmentStateEntry NewEntry;
    NewEntry.FragmentClass = FragmentClass;
    NewEntry.State = State;
    NewEntry.StateTimestamp = ItemOwner ? ItemOwner->GetWorld()->GetTimeSeconds() : 0.f;

    const int32 NewIndex = Entries.Add(NewEntry);
    FragmentClassMap.Add(FragmentClass, NewIndex);
    MarkItemDirty(Entries[NewIndex]);
    return NewIndex;
}

void FRiftFragmentStateList::RemoveStateForFragment(const TSubclassOf<URiftItemFragment>& FragmentClass)
{
    const int32* Index = FragmentClassMap.Find(FragmentClass);
    if (Index && Entries.IsValidIndex(*Index))
    {
        MarkArrayDirty();
        Entries.RemoveAt(*Index);
        FragmentClassMap.Remove(FragmentClass);

        // Rebuild the cache map since indices shifted
        FragmentClassMap.Empty();
        for (int32 i = 0; i < Entries.Num(); ++i)
        {
            FragmentClassMap.Add(Entries[i].FragmentClass, i);
        }
    }
}

void FRiftFragmentStateList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    for (int32 Index : AddedIndices)
    {
        if (Entries.IsValidIndex(Index))
        {
            FragmentClassMap.Add(Entries[Index].FragmentClass, Index);
            Entries[Index].State.GetMutable<FRiftFragmentState>().SynchronizeReplicationState();
        }
    }
}

void FRiftFragmentStateList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    for (int32 Index : ChangedIndices)
    {
        if (Entries.IsValidIndex(Index))
        {
            Entries[Index].State.GetMutable<FRiftFragmentState>().SynchronizeReplicationState();
        }
    }
}

void FRiftFragmentStateList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
    for (int32 Index : RemovedIndices)
    {
        if (Entries.IsValidIndex(Index))
        {
            FragmentClassMap.Remove(Entries[Index].FragmentClass);
        }
    }
}

bool FRiftFragmentStateList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
{
    return FFastArraySerializer::FastArrayDeltaSerialize<FRiftStoredFragmentStateEntry, FRiftFragmentStateList>(Entries, DeltaParams, *this);
}

// ------------------------------------------------------------------
// URiftItemInstance
// ------------------------------------------------------------------

URiftItemInstance::URiftItemInstance(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , FragmentStates(this)
{
}

void URiftItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(URiftItemInstance, ItemId);
    DOREPLIFETIME(URiftItemInstance, ItemDefinition);
    DOREPLIFETIME(URiftItemInstance, FragmentStates);
}

void URiftItemInstance::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
    if (!IsValid(ItemDefinition))
    {
        return;
    }

    // Include the definition's own gameplay tags
    TagContainer.AppendTags(ItemDefinition->GetGameplayTags());

    // Collect tags from all fragments on the definition
    for (const URiftItemFragment* Fragment : ItemDefinition->GetFragments())
    {
        if (IsValid(Fragment))
        {
            Fragment->AppendFragmentTags(this, TagContainer);
        }
    }
}

bool URiftItemInstance::IsSameType(const URiftItemInstance* OtherItem) const
{
    return IsValid(OtherItem) && OtherItem->GetDefinition() == ItemDefinition;
}

bool URiftItemInstance::HasAuthority() const
{
    const UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return false;
    }

    const ENetMode NetMode = World->GetNetMode();
    return NetMode == NM_DedicatedServer || NetMode == NM_ListenServer || NetMode == NM_Standalone;
}

bool URiftItemInstance::HasFragmentOfClass(const TSubclassOf<URiftItemFragment> FragmentClass) const
{
    return IsValid(ItemDefinition) && IsValid(FindFragmentByClass(FragmentClass));
}

URiftItemFragment* URiftItemInstance::FindFragmentByClass(const TSubclassOf<URiftItemFragment> FragmentClass) const
{
    if (!IsValid(ItemDefinition) || !IsValid(FragmentClass))
    {
        return nullptr;
    }

    // Use the StaticClass form to avoid MSVC C2275/C2059 template parse errors
    // when the fragment type is only forward declared at the call site.
    return Cast<URiftItemFragment>(ItemDefinition->FindFragmentByClass(FragmentClass));
}

bool URiftItemInstance::HasStateForFragment(const TSubclassOf<URiftItemFragment> FragmentClass) const
{
    return FragmentStates.HasStateForFragment(FragmentClass);
}

bool URiftItemInstance::GetStateForFragment(const TSubclassOf<URiftItemFragment> FragmentClass, TInstancedStruct<FRiftFragmentState>& OutState) const
{
    return FragmentStates.GetStateForFragment(FragmentClass, OutState);
}

int32 URiftItemInstance::SaveStateForFragment(const TSubclassOf<URiftItemFragment> FragmentClass, const TInstancedStruct<FRiftFragmentState>& State)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftItemInstance::SaveStateForFragment called without authority. Item: %s"), *ItemId.ToString());
        return INDEX_NONE;
    }

    return FragmentStates.SaveStateForFragment(FragmentClass, State);
}

void URiftItemInstance::RemoveStateForFragment(const TSubclassOf<URiftItemFragment> FragmentClass)
{
    if (!HasAuthority())
    {
        return;
    }

    FragmentStates.RemoveStateForFragment(FragmentClass);
}

void URiftItemInstance::InitializeFragmentStates()
{
    if (!IsValid(ItemDefinition))
    {
        return;
    }

    for (URiftItemFragment* Fragment : ItemDefinition->GetFragments())
    {
        if (IsValid(Fragment))
        {
            Fragment->InitializeState(this);
        }
    }
}

void URiftItemInstance::ActivateFragments()
{
    if (!IsValid(ItemDefinition))
    {
        return;
    }

    for (URiftItemFragment* Fragment : ItemDefinition->GetFragments())
    {
        if (IsValid(Fragment))
        {
            Fragment->ActivateForItem(this);
        }
    }
}

void URiftItemInstance::DeactivateFragments()
{
    if (!IsValid(ItemDefinition))
    {
        return;
    }

    for (URiftItemFragment* Fragment : ItemDefinition->GetFragments())
    {
        if (IsValid(Fragment))
        {
            Fragment->DeactivateForItem(this);
        }
    }
}

void URiftItemInstance::SetDefinition(const URiftItemDefinition* NewDefinition)
{
    ItemDefinition = const_cast<URiftItemDefinition*>(NewDefinition);
}

void URiftItemInstance::SetItemId(const FGuid& NewItemId)
{
    ItemId = NewItemId;
}

void URiftItemInstance::OnRep_FragmentStates()
{
    // Fragment states have been updated from the server.
    // ViewModels and other listeners will react via the inventory component's delegates.
}
