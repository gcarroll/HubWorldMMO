#include "GameFramework/Fragments/URiftFragment_Drop.h"
#include "Tags/RiftVaultTags.h"
#include "Engine/StaticMesh.h"

UStaticMesh* URiftFragment_Drop::GetDropMesh() const
{
    return DropMesh.LoadSynchronous();
}

void URiftFragment_Drop::AppendFragmentTags(const URiftItemInstance* Item, FGameplayTagContainer& TagContainer) const
{
    Super::AppendFragmentTags(Item, TagContainer);

    if (bDroppable)
    {
        TagContainer.AddTag(Tag_Rift_Item_Trait_Droppable);
    }

    if (bDeletable)
    {
        TagContainer.AddTag(Tag_Rift_Item_Trait_Deletable);
    }
}
