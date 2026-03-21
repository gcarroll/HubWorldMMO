#include "ViewModels/URiftViewModel_ItemDisplay.h"

#include "GameFramework/Fragments/URiftFragment_Display.h"

void URiftViewModel_ItemDisplay::ShutdownViewModel_Implementation()
{
    ClearItemInstance();
}

void URiftViewModel_ItemDisplay::SetItemInstance(URiftItemInstance* NewItemInstance)
{
    ItemInstance = NewItemInstance;

    if (ItemInstance.IsValid())
    {
        RefreshFromFragment();
    }
    else
    {
        ResetToDefaults();
    }

    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HasItem);
}

void URiftViewModel_ItemDisplay::ClearItemInstance()
{
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

FGameplayTag URiftViewModel_ItemDisplay::GetRarityTag() const
{
    return RarityTag;
}


void URiftViewModel_ItemDisplay::RefreshFromFragment()
{
    if (!ItemInstance.IsValid())
    {
        return;
    }

    // Use StaticClass form to avoid MSVC C2275 template parse errors
    const URiftFragment_Display* Fragment = Cast<URiftFragment_Display>(
        ItemInstance->FindFragmentByClass(URiftFragment_Display::StaticClass()));

    if (!IsValid(Fragment))
    {
        ResetToDefaults();
        return;
    }

    UE_MVVM_SET_PROPERTY_VALUE(DisplayName, Fragment->GetDisplayName());
    UE_MVVM_SET_PROPERTY_VALUE(Description, Fragment->GetDescription());
    UE_MVVM_SET_PROPERTY_VALUE(Icon, Fragment->GetIcon());
    UE_MVVM_SET_PROPERTY_VALUE(RarityTag, Fragment->GetRarityTag());
}

void URiftViewModel_ItemDisplay::ResetToDefaults()
{
    UE_MVVM_SET_PROPERTY_VALUE(DisplayName, FText::GetEmpty());
    UE_MVVM_SET_PROPERTY_VALUE(Description, FText::GetEmpty());
    UE_MVVM_SET_PROPERTY_VALUE(Icon, nullptr);
    UE_MVVM_SET_PROPERTY_VALUE(RarityTag, FGameplayTag::EmptyTag);
}
