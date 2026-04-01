#include "ViewModels/URiftViewModel_ItemDisplay.h"

#include "Components/URiftInventoryComponent.h"
#include "GameFramework/Fragments/URiftFragment_Display.h"
#include "GameFramework/Fragments/URiftFragment_Stack.h"
#include "Tags/RiftVaultTags.h"

void URiftViewModel_ItemDisplay::ShutdownViewModel_Implementation()
{
    UnbindFromInventoryEvents();
    ClearItemInstance();
}

void URiftViewModel_ItemDisplay::SetItemInstance_Implementation(URiftItemInstance* NewItemInstance)
{
    UnbindFromInventoryEvents();

    ItemInstance = NewItemInstance;

    if (ItemInstance.IsValid())
    {
        BindToInventoryEvents();
        RefreshFromFragment();
    }
    else
    {
        ResetToDefaults();
    }

    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HasItem);
}

void URiftViewModel_ItemDisplay::ClearItemInstance_Implementation()
{
    UnbindFromInventoryEvents();
    ItemInstance.Reset();
    ResetToDefaults();
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HasItem);
}

bool URiftViewModel_ItemDisplay::HasItem() const
{
    return ItemInstance.IsValid();
}

FText URiftViewModel_ItemDisplay::GetDisplayName() const
{
    return DisplayName;
}

FText URiftViewModel_ItemDisplay::GetDescription() const
{
    return Description;
}

UTexture2D* URiftViewModel_ItemDisplay::GetIcon() const
{
    return Icon;
}

FSlateBrush URiftViewModel_ItemDisplay::GetIconBrush() const
{
    FSlateBrush Brush;
    if (IsValid(Icon))
    {
        Brush.SetResourceObject(Icon);
        Brush.ImageSize = FVector2D(Icon->GetSizeX(), Icon->GetSizeY());
        Brush.DrawAs = ESlateBrushDrawType::Image;
    }
    return Brush;
}

FGameplayTag URiftViewModel_ItemDisplay::GetRarityTag() const
{
    return RarityTag;
}

int32 URiftViewModel_ItemDisplay::GetCurrentQuantity() const
{
    return CurrentQuantity;
}

bool URiftViewModel_ItemDisplay::IsStackCountVisible() const
{
    return bIsStackCountVisible;
}

void URiftViewModel_ItemDisplay::RefreshFromFragment()
{
    if (!ItemInstance.IsValid())
    {
        return;
    }

    const URiftFragment_Display* Fragment = ItemInstance->FindFragment<URiftFragment_Display>();

    if (!IsValid(Fragment))
    {
        ResetToDefaults();
        return;
    }

    UE_MVVM_SET_PROPERTY_VALUE(DisplayName, Fragment->GetDisplayName());
    UE_MVVM_SET_PROPERTY_VALUE(Description, Fragment->GetDescription());
    UE_MVVM_SET_PROPERTY_VALUE(Icon, Fragment->GetIcon());
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetIconBrush);
    UE_MVVM_SET_PROPERTY_VALUE(RarityTag, Fragment->GetRarityTag());

    RefreshStackFields();
}

void URiftViewModel_ItemDisplay::RefreshStackFields()
{
    if (!ItemInstance.IsValid())
    {
        return;
    }

    const URiftFragment_Stack* StackFragment = ItemInstance->FindFragment<URiftFragment_Stack>();

    const int32 Quantity = IsValid(StackFragment)
        ? StackFragment->GetCurrentQuantity(ItemInstance.Get())
        : 1;

    const bool bShowCount = IsValid(StackFragment) && StackFragment->ShouldShowStackCount();

    UE_MVVM_SET_PROPERTY_VALUE(CurrentQuantity, Quantity);
    UE_MVVM_SET_PROPERTY_VALUE(bIsStackCountVisible, bShowCount);
}

void URiftViewModel_ItemDisplay::BindToInventoryEvents()
{
    if (!ItemInstance.IsValid())
    {
        return;
    }

    URiftInventoryComponent* InvComp = ItemInstance->GetTypedOuter<URiftInventoryComponent>();
    if (IsValid(InvComp))
    {
        BoundInventory = InvComp;
        InvComp->OnItemEvent.AddDynamic(this, &URiftViewModel_ItemDisplay::OnInventoryItemEvent);
    }
}

void URiftViewModel_ItemDisplay::UnbindFromInventoryEvents()
{
    if (BoundInventory.IsValid())
    {
        BoundInventory->OnItemEvent.RemoveDynamic(this, &URiftViewModel_ItemDisplay::OnInventoryItemEvent);
        BoundInventory.Reset();
    }
}

void URiftViewModel_ItemDisplay::OnInventoryItemEvent(URiftItemInstance* Item, FGameplayTag EventTag)
{
    if (Item == ItemInstance.Get() && EventTag == Tag_Rift_Event_Item_StackChanged)
    {
        RefreshStackFields();
    }
}

void URiftViewModel_ItemDisplay::ResetToDefaults()
{
    UE_MVVM_SET_PROPERTY_VALUE(DisplayName, FText::GetEmpty());
    UE_MVVM_SET_PROPERTY_VALUE(Description, FText::GetEmpty());
    UE_MVVM_SET_PROPERTY_VALUE(Icon, nullptr);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetIconBrush);
    UE_MVVM_SET_PROPERTY_VALUE(RarityTag, FGameplayTag::EmptyTag);
    UE_MVVM_SET_PROPERTY_VALUE(CurrentQuantity, 0);
    UE_MVVM_SET_PROPERTY_VALUE(bIsStackCountVisible, false);
}
