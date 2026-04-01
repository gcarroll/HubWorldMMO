#include "Data/URiftContainerDefinition_Equipment.h"
#include "Tags/RiftVaultTags.h"

URiftContainerDefinition_Equipment::URiftContainerDefinition_Equipment()
{
    DisplayName   = NSLOCTEXT("RiftVault", "EquipmentContainerName", "Equipment");
    ContainerTag  = Tag_Rift_Container_Equipment;
    ContainerType = EContainerType::Equipment;
    LayoutType    = EContainerLayoutType::List;
    SlotCount     = 12;
    ColumnCount   = 1;

    // Restrict to items that carry the Rift.Item.Trait.Equippable tag.
    // Non-equippable items (consumables, quest items, etc.) will be rejected.
    ItemCompatibilityQuery.Build(
        FGameplayTagQueryExpression()
            .AllTagsMatch()
            .AddTag(Tag_Rift_Item_Trait_Equippable),
        TEXT("Accepts only equippable items.")
    );
}
