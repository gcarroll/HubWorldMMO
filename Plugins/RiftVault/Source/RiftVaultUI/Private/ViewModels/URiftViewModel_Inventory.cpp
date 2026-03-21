#include "ViewModels/URiftViewModel_Inventory.h"

#include "Components/URiftInventoryComponent.h"

void URiftViewModel_Inventory::InitializeViewModel_Implementation(const UUserWidget* UserWidget)
{
    bIsSingleton = true;
}

void URiftViewModel_Inventory::ShutdownViewModel_Implementation()
{
    UnbindFromInventoryComponent();
    Items.Empty();
}

void URiftViewModel_Inventory::SetInventoryComponent(URiftInventoryComponent* NewInventoryComponent)
{
    UnbindFromInventoryComponent();
    InventoryComponent = NewInventoryComponent;
    BindToInventoryComponent();
    RefreshItems();
}

TArray<URiftItemInstance*> URiftViewModel_Inventory::GetItems() const
{
    TArray<URiftItemInstance*> Result;
    for (const TObjectPtr<URiftItemInstance>& Item : Items)
    {
        if (IsValid(Item))
        {
            Result.Add(Item.Get());
        }
    }
    return Result;
}

int32 URiftViewModel_Inventory::GetItemCount() const
{
    return Items.Num();
}

void URiftViewModel_Inventory::OnItemAdded(URiftItemInstance* Item, URiftContainer* Container)
{
    if (IsValid(Item))
    {
        Items.AddUnique(Item);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItems);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItemCount);
    }
}

void URiftViewModel_Inventory::OnItemRemoved(URiftItemInstance* Item, URiftContainer* Container)
{
    if (Items.Remove(Item) > 0)
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItems);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItemCount);
    }
}

void URiftViewModel_Inventory::BindToInventoryComponent()
{
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    InventoryComponent->OnItemAdded.AddDynamic(this, &URiftViewModel_Inventory::OnItemAdded);
    InventoryComponent->OnItemRemoved.AddDynamic(this, &URiftViewModel_Inventory::OnItemRemoved);
}

void URiftViewModel_Inventory::UnbindFromInventoryComponent()
{
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    InventoryComponent->OnItemAdded.RemoveDynamic(this, &URiftViewModel_Inventory::OnItemAdded);
    InventoryComponent->OnItemRemoved.RemoveDynamic(this, &URiftViewModel_Inventory::OnItemRemoved);
}

void URiftViewModel_Inventory::RefreshItems()
{
    Items.Empty();

    if (!InventoryComponent.IsValid())
    {
        return;
    }

    for (URiftItemInstance* Item : InventoryComponent->GetAllItems())
    {
        if (IsValid(Item))
        {
            Items.Add(Item);
        }
    }

    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItems);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItemCount);
}
