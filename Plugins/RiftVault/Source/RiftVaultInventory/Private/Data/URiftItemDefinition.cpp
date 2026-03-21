#include "Data/URiftItemDefinition.h"

#include "Tags/RiftVaultTags.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

URiftItemDefinition::URiftItemDefinition()
{
    InternalName = "Item";
    GameplayTags = FGameplayTagContainer::EmptyContainer;
    DynamicTags = FGameplayTagContainer::EmptyContainer;

    // All items get the base item trait tag by default so containers
    // with a default compatibility query will accept them.
    GameplayTags.AddTagFast(Tag_Rift_Item_Trait);
}

void URiftItemDefinition::PostLoad()
{
    Super::PostLoad();
    RefreshDynamicTags();
    RefreshFragmentCache();
}

FPrimaryAssetId URiftItemDefinition::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("RiftItemDefinition"), GetFName());
}

void URiftItemDefinition::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
    TagContainer.AppendTags(GetGameplayTags());
}

FGameplayTagContainer URiftItemDefinition::GetGameplayTags() const
{
    FGameplayTagContainer AllTags;
    AllTags.AppendTags(GameplayTags);
    AllTags.AppendTags(DynamicTags);
    return AllTags;
}

TArray<URiftItemFragment*> URiftItemDefinition::GetFragments() const
{
    TArray<URiftItemFragment*> Result;
    for (const TObjectPtr<URiftItemFragment>& Fragment : Fragments)
    {
        if (IsValid(Fragment))
        {
            Result.Add(Fragment.Get());
        }
    }
    return Result;
}

bool URiftItemDefinition::HasFragmentOfClass(const TSubclassOf<URiftItemFragment>& FragmentClass) const
{
    return IsValid(FragmentClass) && FragmentCache.Contains(FragmentClass);
}

URiftItemFragment* URiftItemDefinition::FindFragmentByClass(const TSubclassOf<URiftItemFragment> FragmentClass) const
{
    if (!IsValid(FragmentClass))
    {
        return nullptr;
    }

    // Exact match first — O(1)
    const TObjectPtr<URiftItemFragment>* Found = FragmentCache.Find(FragmentClass);
    if (Found != nullptr)
    {
        return Found->Get();
    }

    // Subclass fallback — allows querying by base fragment class
    for (const TPair<TSubclassOf<URiftItemFragment>, TObjectPtr<URiftItemFragment>>& Pair : FragmentCache)
    {
        if (IsValid(Pair.Key) && Pair.Key->IsChildOf(FragmentClass))
        {
            return Pair.Value.Get();
        }
    }

    return nullptr;
}

TArray<URiftItemFragment*> URiftItemDefinition::FindFragmentsByInterface(const TSubclassOf<UInterface> FragmentInterface) const
{
    TArray<URiftItemFragment*> Result;
    if (!IsValid(FragmentInterface))
    {
        return Result;
    }

    for (const TObjectPtr<URiftItemFragment>& Fragment : Fragments)
    {
        if (IsValid(Fragment) && Fragment->GetClass()->ImplementsInterface(FragmentInterface))
        {
            Result.Add(Fragment.Get());
        }
    }
    return Result;
}

void URiftItemDefinition::AddFragment(URiftItemFragment* Fragment)
{
    if (IsValid(Fragment) && !Fragments.Contains(Fragment))
    {
        Fragments.Add(Fragment);
        RefreshDynamicTags();
        RefreshFragmentCache();
    }
}

void URiftItemDefinition::AppendGameplayTags(const FGameplayTagContainer& NewTags)
{
    if (NewTags.IsValid())
    {
        GameplayTags.AppendTags(NewTags);
    }
}

void URiftItemDefinition::RefreshDynamicTags()
{
    DynamicTags.Reset();
    for (const URiftItemFragment* Fragment : Fragments)
    {
        if (IsValid(Fragment))
        {
            // Pass nullptr for item — definition-level tag collection has no instance context
            Fragment->AppendFragmentTags(nullptr, DynamicTags);
        }
    }
}

void URiftItemDefinition::RefreshFragmentCache()
{
    FragmentCache.Empty(Fragments.Num());
    for (URiftItemFragment* Fragment : Fragments)
    {
        if (IsValid(Fragment))
        {
            FragmentCache.Add(Fragment->GetClass(), Fragment);
        }
    }
}

#if WITH_EDITOR
void URiftItemDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, Fragments))
    {
        RefreshDynamicTags();
        RefreshFragmentCache();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

EDataValidationResult URiftItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
    if (InternalName.IsNone())
    {
        Context.AddWarning(NSLOCTEXT("RiftVault", "MissingInternalName", "Item Definition has no InternalName set. This makes debugging harder."));
    }

    if (GameplayTags.IsEmpty())
    {
        Context.AddWarning(NSLOCTEXT("RiftVault", "NoItemTags", "Item Definition has no GameplayTags. It may not be accepted by any container."));
    }

    for (int32 Idx = 0; Idx < Fragments.Num(); ++Idx)
    {
        const URiftItemFragment* Fragment = Fragments[Idx];
        if (!IsValid(Fragment))
        {
            FFormatNamedArguments Args;
            Args.Add(TEXT("Index"), FText::AsNumber(Idx + 1));
            Context.AddError(FText::Format(
                NSLOCTEXT("RiftVault", "NullFragment", "Fragment at index {Index} is null or invalid."), Args));
        }
    }

    return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
