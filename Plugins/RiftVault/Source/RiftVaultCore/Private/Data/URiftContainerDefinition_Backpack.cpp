#include "Data/URiftContainerDefinition_Backpack.h"
#include "Tags/RiftVaultTags.h"

URiftContainerDefinition_Backpack::URiftContainerDefinition_Backpack()
{
    DisplayName  = NSLOCTEXT("RiftVault", "BackpackContainerName", "Backpack");
    ContainerTag = Tag_Rift_Container_Backpack;
    ContainerType = EContainerType::PlayerInventory;
    LayoutType   = EContainerLayoutType::Grid;
    SlotCount    = 40;
    ColumnCount  = 5;
    // ItemCompatibilityQuery left as the base default — accepts all items with a Rift trait tag.
}
