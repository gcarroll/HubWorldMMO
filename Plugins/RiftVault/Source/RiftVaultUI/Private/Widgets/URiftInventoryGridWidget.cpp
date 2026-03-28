#include "Widgets/URiftInventoryGridWidget.h"

#include "Components/PanelWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/URiftInventoryComponent.h"
#include "Data/URiftContainerDefinition.h"
#include "GameFramework/Containers/URiftContainer.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/IRiftInventoryInterface.h"
#include "Widgets/URiftItemSlotWidget.h"

URiftInventoryGridWidget::URiftInventoryGridWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URiftInventoryGridWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Auto-wire to the owning player's inventory if any nearby actor implements
    // IRiftInventoryInterface. Checks Pawn first, then PlayerState.
    // For non-player inventories (vendors, chests) call SetInventoryOwner explicitly instead.
    if (InventoryComponent.IsValid())
    {
        if (SlotWidgets.IsEmpty())
        {
            // Widget was cached and re-added to viewport after RemoveFromParent.
            // NativeDestruct unbound delegates and emptied SlotWidgets — restore both.
            BindToInventory();
            BuildSlots();
        }
        return;
    }

    if (APlayerController* PC = GetOwningPlayer())
    {
        if (APawn* Pawn = PC->GetPawn(); IsValid(Pawn) && Pawn->Implements<URiftInventoryInterface>())
        {
            SetInventoryOwner(Pawn);
        }
        else if (APlayerState* PS = PC->GetPlayerState<APlayerState>(); IsValid(PS) && PS->Implements<URiftInventoryInterface>())
        {
            SetInventoryOwner(PS);
        }
    }
}

void URiftInventoryGridWidget::SetInventoryOwner(UObject* InventoryOwner)
{
    URiftInventoryComponent* NewComponent = nullptr;

    if (IsValid(InventoryOwner) && InventoryOwner->Implements<URiftInventoryInterface>())
    {
        NewComponent = Cast<URiftInventoryComponent>(
            IRiftInventoryInterface::Execute_GetInventoryComponent(InventoryOwner));
    }

    if (InventoryComponent.Get() == NewComponent)
    {
        return;
    }

    // Tear down any previous binding.
    UnbindFromInventory();
    SlotWidgets.Empty();
    if (IsValid(ItemGrid))
    {
        ItemGrid->ClearChildren();
    }

    InventoryComponent = NewComponent;

    if (!InventoryComponent.IsValid())
    {
        return;
    }

    BindToInventory();
    InventoryComponent->WaitForInitialized(
        FOnRiftInventoryInitializedDelegate::CreateUObject(this, &URiftInventoryGridWidget::OnInventoryInitialized));
}

void URiftInventoryGridWidget::NativeDestruct()
{
    UnbindFromInventory();
    SlotWidgets.Empty();
    Super::NativeDestruct();
}

void URiftInventoryGridWidget::OnInventoryInitialized(bool bSuccess)
{
    if (!bSuccess || !InventoryComponent.IsValid())
    {
        return;
    }

    // Build the full grid now that the inventory is ready.
    BuildSlots();
}

void URiftInventoryGridWidget::OnItemAdded(URiftItemInstance* Item, URiftContainer* Container)
{
    if (!IsValid(Item) || !IsValid(Container) || !IsValid(ContainerDefinition))
    {
        return;
    }

    // Ignore events from containers other than the one this grid is displaying.
    // Compare by definition pointer so two containers with the same tag are told apart.
    if (Container->GetDefinition() != ContainerDefinition)
    {
        return;
    }

    if (!TrackedContainer.IsValid())
    {
        return;
    }

    // The item already occupies a slot — find which one and update that widget.
    const int32 SlotIndex = TrackedContainer->GetSlotIndexOfItem(Item);
    if (SlotWidgets.IsValidIndex(SlotIndex) && IsValid(SlotWidgets[SlotIndex]))
    {
        SlotWidgets[SlotIndex]->SetItem(Item, TrackedContainer.Get(), SlotIndex);
    }
}

void URiftInventoryGridWidget::OnItemRemoved(URiftItemInstance* Item, URiftContainer* Container)
{
    if (!IsValid(Item) || !IsValid(Container) || !IsValid(ContainerDefinition))
    {
        return;
    }

    // Compare by definition pointer so two containers with the same tag are told apart.
    if (Container->GetDefinition() != ContainerDefinition)
    {
        return;
    }

    // Find whichever slot widget is showing this item and clear it back to empty.
    for (URiftItemSlotWidget* SlotWidget : SlotWidgets)
    {
        if (IsValid(SlotWidget) && SlotWidget->GetItemInstance() == Item)
        {
            SlotWidget->ClearItem();
            break;
        }
    }
}

void URiftInventoryGridWidget::BuildSlots()
{
    if (!IsValid(ItemGrid) || !IsValid(ItemSlotWidgetClass) || !InventoryComponent.IsValid() || !IsValid(ContainerDefinition))
    {
        return;
    }

    ItemGrid->ClearChildren();
    SlotWidgets.Empty();

    TrackedContainer = InventoryComponent->GetContainerByDefinition(ContainerDefinition);
    if (!TrackedContainer.IsValid())
    {
        return;
    }

    const int32 GridWidth = ContainerDefinition->GetGridWidth();
    const int32 Capacity  = ContainerDefinition->GetCapacity();

    // Create one slot widget per slot (empty and occupied alike).
    // The slot index is the canonical position — row and column are derived from it.
    for (int32 SlotIndex = 0; SlotIndex < Capacity; ++SlotIndex)
    {
        URiftItemSlotWidget* SlotWidget = CreateWidget<URiftItemSlotWidget>(GetOwningPlayer(), ItemSlotWidgetClass);
        if (!IsValid(SlotWidget))
        {
            // Keep a null entry so SlotWidgets indices stay aligned with container slots.
            SlotWidgets.Add(nullptr);
            continue;
        }

        SlotWidgets.Add(SlotWidget);

        const int32 Column = SlotIndex % GridWidth;
        const int32 Row    = SlotIndex / GridWidth;

        UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(ItemGrid->AddChild(SlotWidget));
        if (IsValid(GridSlot))
        {
            GridSlot->SetColumn(Column);
            GridSlot->SetRow(Row);
            GridSlot->SetHorizontalAlignment(HAlign_Fill);
            GridSlot->SetVerticalAlignment(VAlign_Fill);
        }

        // Always initialise container + index so empty slots can accept drops.
        SlotWidget->InitSlot(TrackedContainer.Get(), SlotIndex);

        // If an item already occupies this slot (e.g. loaded from save), wire it up now.
        URiftItemInstance* Item = TrackedContainer->GetItemAtSlot(SlotIndex);
        if (IsValid(Item))
        {
            SlotWidget->SetItem(Item, TrackedContainer.Get(), SlotIndex);
        }
    }
}

void URiftInventoryGridWidget::OnItemMoved(URiftItemInstance* Item, URiftContainer* FromContainer, URiftContainer* ToContainer)
{
    if (!TrackedContainer.IsValid() || !IsValid(ContainerDefinition))
    {
        return;
    }

    // Compare by definition pointer so two containers with the same tag are told apart.
    const bool bFromTracked = IsValid(FromContainer) && FromContainer->GetDefinition() == ContainerDefinition;
    const bool bToTracked   = IsValid(ToContainer)   && ToContainer->GetDefinition()   == ContainerDefinition;

    if (!bFromTracked && !bToTracked)
    {
        return;
    }

    // Only refresh slots whose content has changed since the last widget update.
    // Skipping unchanged slots avoids redundant WireViewModel calls between drags.
    for (int32 i = 0; i < SlotWidgets.Num(); ++i)
    {
        URiftItemSlotWidget* SlotWidget = SlotWidgets[i];
        if (!IsValid(SlotWidget))
        {
            continue;
        }

        URiftItemInstance* ItemAtSlot = TrackedContainer->GetItemAtSlot(i);
        if (SlotWidget->GetItemInstance() == ItemAtSlot)
        {
            continue; // No change — skip.
        }

        if (IsValid(ItemAtSlot))
        {
            SlotWidget->SetItem(ItemAtSlot, TrackedContainer.Get(), i);
        }
        else
        {
            SlotWidget->ClearItem();
        }
    }
}

void URiftInventoryGridWidget::BindToInventory()
{
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    InventoryComponent->OnItemAdded.AddDynamic(this, &URiftInventoryGridWidget::OnItemAdded);
    InventoryComponent->OnItemRemoved.AddDynamic(this, &URiftInventoryGridWidget::OnItemRemoved);
    InventoryComponent->OnItemMoved.AddDynamic(this, &URiftInventoryGridWidget::OnItemMoved);
}

void URiftInventoryGridWidget::UnbindFromInventory()
{
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    InventoryComponent->OnItemAdded.RemoveDynamic(this, &URiftInventoryGridWidget::OnItemAdded);
    InventoryComponent->OnItemRemoved.RemoveDynamic(this, &URiftInventoryGridWidget::OnItemRemoved);
    InventoryComponent->OnItemMoved.RemoveDynamic(this, &URiftInventoryGridWidget::OnItemMoved);
    InventoryComponent->OnInventoryInitialized.RemoveDynamic(this, &URiftInventoryGridWidget::OnInventoryInitialized);
}

int32 URiftInventoryGridWidget::FindSlotIndexOfItem(URiftItemInstance* Item) const
{
    if (!TrackedContainer.IsValid() || !IsValid(Item))
    {
        return INDEX_NONE;
    }

    return TrackedContainer->GetSlotIndexOfItem(Item);
}
