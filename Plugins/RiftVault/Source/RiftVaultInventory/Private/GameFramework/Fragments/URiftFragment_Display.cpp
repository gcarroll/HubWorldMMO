#include "GameFramework/Fragments/URiftFragment_Display.h"

#include "Engine/Texture2D.h"

URiftFragment_Display::URiftFragment_Display()
{
    DisplayName   = NSLOCTEXT("RiftVault", "DefaultItemName", "Item");
    Description   = NSLOCTEXT("RiftVault", "DefaultItemDescription", "No description.");
    Quality       = 0;
    ViewModelName = FName("ItemDisplayViewModel");
}

UTexture2D* URiftFragment_Display::GetIcon() const
{
    return Icon.LoadSynchronous();
}
