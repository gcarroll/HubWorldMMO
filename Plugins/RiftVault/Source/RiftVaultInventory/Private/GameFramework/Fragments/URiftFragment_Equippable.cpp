#include "GameFramework/Fragments/URiftFragment_Equippable.h"

#include "Tags/RiftVaultTags.h"

URiftFragment_Equippable::URiftFragment_Equippable()
{
    // Contribute the equippable trait tag so containers with an equipment
    // compatibility query will accept this item.
    FragmentTags.AddTagFast(Tag_Rift_Item_Trait_Equippable);
}

void URiftFragment_Equippable::AppendFragmentTags(const URiftItemInstance* Item, FGameplayTagContainer& TagContainer) const
{
    Super::AppendFragmentTags(Item, TagContainer);

    // Also emit the slot tag so per-slot container queries (e.g. ANY(Rift.Slot.Armor.Head))
    // can accept or reject this item based on which slot it belongs to.
    if (EquipmentSlotTag.IsValid())
    {
        TagContainer.AddTag(EquipmentSlotTag);
    }
}
