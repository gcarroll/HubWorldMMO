#include "Data/URiftGroundContainerDefinition.h"
#include "Tags/RiftVaultTags.h"

URiftGroundContainerDefinition::URiftGroundContainerDefinition()
{
    DisplayName = NSLOCTEXT("RiftVault", "GroundContainerName", "Ground");
    ContainerTag = Tag_Rift_Container_Ground;
    LayoutType  = EContainerLayoutType::Grid;
    SlotCount   = 100;
    ColumnCount = 10;
    // ItemCompatibilityQuery left empty — accepts all item types.
}
