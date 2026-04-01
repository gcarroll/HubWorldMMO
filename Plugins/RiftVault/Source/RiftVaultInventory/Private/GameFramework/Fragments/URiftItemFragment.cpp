#include "GameFramework/Fragments/URiftItemFragment.h"

#include "GameFramework/Items/URiftItemInstance.h"

URiftItemFragment::URiftItemFragment()
{
    FragmentTags = FGameplayTagContainer::EmptyContainer;
    WatchedEventTags = FGameplayTagContainer::EmptyContainer;
}

bool URiftItemFragment::ItemHasState(const URiftItemInstance* Item) const
{
    return IsValid(Item) && Item->HasStateForFragment(GetClass());
}

bool URiftItemFragment::ItemHasAuthority(const URiftItemInstance* Item)
{
    return IsValid(Item) && Item->HasAuthority();
}

bool URiftItemFragment::IsRelevantEvent(const FGameplayTag EventTag) const
{
    return WatchedEventTags.HasTagExact(EventTag);
}

void URiftItemFragment::InitializeState_Implementation(URiftItemInstance* Item)
{
    // No default implementation. Subclasses override this to create their state.
}

void URiftItemFragment::ActivateForItem_Implementation(URiftItemInstance* Item)
{
    // No default implementation.
}

void URiftItemFragment::DeactivateForItem_Implementation(URiftItemInstance* Item)
{
    // No default implementation.
}

void URiftItemFragment::HandleItemEvent_Implementation(URiftItemInstance* Item, FGameplayTag EventTag)
{
    // No default implementation. Subclasses override to respond to watched events.
}

void URiftItemFragment::AppendFragmentTags(const URiftItemInstance* Item, FGameplayTagContainer& TagContainer) const
{
    TagContainer.AppendTags(FragmentTags);
}
