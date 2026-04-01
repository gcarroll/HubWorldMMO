#include "Data/URiftContainerDefinition.h"

#include "Tags/RiftVaultTags.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

URiftContainerDefinition::URiftContainerDefinition()
{
    DisplayName  = NSLOCTEXT("RiftVault", "DefaultContainerName", "Container");
    LayoutType   = EContainerLayoutType::Grid;
    SlotCount    = 20;
    ColumnCount  = 5;

    // Default compatibility query accepts any item that has the base Rift.Item tag.
    // Designers can override this to restrict containers to specific item traits.
    ItemCompatibilityQuery.Build(
        FGameplayTagQueryExpression()
            .AllTagsMatch()
            .AddTag(Tag_Rift_Item_Trait),
        TEXT("Accepts any item with a Rift item trait tag.")
    );
}

FPrimaryAssetId URiftContainerDefinition::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("RiftContainerDefinition"), GetFName());
}

bool URiftContainerDefinition::AcceptsItemWithTags(const FGameplayTagContainer& ItemTags) const
{
    // An empty query accepts everything.
    if (ItemCompatibilityQuery.IsEmpty())
    {
        return true;
    }

    const bool bMatches = ItemCompatibilityQuery.Matches(ItemTags);
    if (!bMatches)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftContainerDefinition::AcceptsItemWithTags — REJECTED. Container: %s | Item tags: %s | Query: %s"),
            *ContainerTag.ToString(),
            *ItemTags.ToString(),
            *ItemCompatibilityQuery.GetDescription());
    }
    return bMatches;
}

#if WITH_EDITOR
EDataValidationResult URiftContainerDefinition::IsDataValid(FDataValidationContext& Context) const
{
    if (!ContainerTag.IsValid())
    {
        Context.AddError(NSLOCTEXT("RiftVault", "MissingContainerTag", "Container Definition is missing a ContainerTag. Set a Rift.Container.* tag."));
    }

    if (SlotCount <= 0)
    {
        Context.AddError(NSLOCTEXT("RiftVault", "InvalidSlotCount", "Container SlotCount must be at least 1."));
    }

    if (LayoutType == EContainerLayoutType::Grid && ColumnCount <= 0)
    {
        Context.AddError(NSLOCTEXT("RiftVault", "InvalidColumnCount", "Container ColumnCount must be at least 1 for Grid layout."));
    }

    if (ItemCompatibilityQuery.IsEmpty())
    {
        Context.AddWarning(NSLOCTEXT("RiftVault", "EmptyCompatibilityQuery", "Item Compatibility Query is empty — this container will accept all items."));
    }

    return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
