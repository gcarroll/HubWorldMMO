#include "GameFramework/Fragments/URiftFragment_Equippable.h"

#include "Tags/RiftVaultTags.h"

URiftFragment_Equippable::URiftFragment_Equippable()
{
    // Contribute the equippable trait tag so containers with an equipment
    // compatibility query will accept this item.
    FragmentTags.AddTagFast(Tag_Rift_Item_Trait_Equippable);
}
