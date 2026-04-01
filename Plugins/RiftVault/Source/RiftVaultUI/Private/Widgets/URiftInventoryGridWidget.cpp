#include "Widgets/URiftInventoryGridWidget.h"

#include "DragDrop/URiftDragDropOperation.h"
#include "Components/PanelWidget.h"
#include "Components/UniformGridSlot.h"
#include "Components/GridSlot.h"
#include "Components/URiftInventoryComponent.h"
#include "Data/URiftContainerDefinition.h"
#include "GameFramework/Containers/URiftContainer.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/IRiftInventoryInterface.h"
#include "Types/EContainerLayoutType.h"
#include "Widgets/URiftItemSlotWidget.h"

URiftContainerWidget::URiftContainerWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URiftContainerWidget::NativeConstruct()
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

void URiftContainerWidget::SetInventoryOwner(UObject* InventoryOwner)
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
    if (UPanelWidget* ItemPanel = GetItemPanel())
    {
        ItemPanel->ClearChildren();
    }

    InventoryComponent = NewComponent;

    if (!InventoryComponent.IsValid())
    {
        return;
    }

    BindToInventory();
    InventoryComponent->WaitForInitialized(
        FOnRiftInventoryInitializedDelegate::CreateUObject(this, &URiftContainerWidget::OnInventoryInitialized));
}

void URiftContainerWidget::NativeDestruct()
{
    UnbindFromInventory();
    SlotWidgets.Empty();
    Super::NativeDestruct();
}

void URiftContainerWidget::OnInventoryInitialized(bool bSuccess)
{
    if (!bSuccess || !InventoryComponent.IsValid())
    {
        return;
    }

    // Build the full grid now that the inventory is ready.
    BuildSlots();
}

void URiftContainerWidget::OnItemAdded(URiftItemInstance* Item, URiftContainer* Container)
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

    const int32 SlotIndex = TrackedContainer->GetSlotIndexOfItem(Item);
    if (SlotIndex == INDEX_NONE)
    {
        return;
    }

    if (ContainerDefinition->GetLayoutType() == EContainerLayoutType::List)
    {
        // List: rebuild entirely so order stays correct even when PostReplicatedChange
        // fires multiple OnItemAdded events back-to-back (e.g. after a same-container swap).
        BuildSlots();
    }
    else if (SlotWidgets.IsValidIndex(SlotIndex) && IsValid(SlotWidgets[SlotIndex]))
    {
        SlotWidgets[SlotIndex]->SetItem(Item, TrackedContainer.Get(), SlotIndex);
    }
}

void URiftContainerWidget::OnItemRemoved(URiftItemInstance* Item, URiftContainer* Container)
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

    if (ContainerDefinition->GetLayoutType() == EContainerLayoutType::List)
    {
        // List: rebuild entirely so order stays correct even when PostReplicatedChange
        // fires multiple OnItemRemoved events back-to-back (e.g. after a same-container swap).
        BuildSlots();
    }
    else
    {
        // Grid: clear the slot widget back to empty so the cell remains a drop target.
        for (URiftItemSlotWidget* SlotWidget : SlotWidgets)
        {
            if (IsValid(SlotWidget) && SlotWidget->GetItemInstance() == Item)
            {
                SlotWidget->ClearItem();
                break;
            }
        }
    }
}

void URiftContainerWidget::BuildSlots()
{
    UPanelWidget* ItemGrid = GetItemPanel();
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

    const int32 Capacity             = ContainerDefinition->GetCapacity();
    const EContainerLayoutType Layout = ContainerDefinition->GetLayoutType();
    const int32 Columns              = ContainerDefinition->GetColumnCount();

    if (Layout == EContainerLayoutType::List)
    {
        // List: only create widgets for occupied slots — the list grows as items are added.
        for (int32 SlotIndex = 0; SlotIndex < Capacity; ++SlotIndex)
        {
            URiftItemInstance* Item = TrackedContainer->GetItemAtSlot(SlotIndex);
            if (!IsValid(Item))
            {
                continue;
            }
            AppendListSlot(Item, SlotIndex);
        }
    }
    else
    {
        // Grid: create one widget per slot (empty and occupied alike) so every cell is a drop target.
        for (int32 SlotIndex = 0; SlotIndex < Capacity; ++SlotIndex)
        {
            URiftItemSlotWidget* SlotWidget = CreateWidget<URiftItemSlotWidget>(GetOwningPlayer(), ItemSlotWidgetClass);
            if (!IsValid(SlotWidget))
            {
                SlotWidgets.Add(nullptr);
                continue;
            }

            SlotWidgets.Add(SlotWidget);

            UPanelSlot* PanelSlot = ItemGrid->AddChild(SlotWidget);
            if (IsValid(PanelSlot))
            {
                const int32 Column = SlotIndex % Columns;
                const int32 Row    = SlotIndex / Columns;

                if (UUniformGridSlot* UGSlot = Cast<UUniformGridSlot>(PanelSlot))
                {
                    UGSlot->SetColumn(Column);
                    UGSlot->SetRow(Row);
                    UGSlot->SetHorizontalAlignment(HAlign_Fill);
                    UGSlot->SetVerticalAlignment(VAlign_Fill);
                }
                else if (UGridSlot* GSlot = Cast<UGridSlot>(PanelSlot))
                {
                    GSlot->SetColumn(Column);
                    GSlot->SetRow(Row);
                    GSlot->SetHorizontalAlignment(HAlign_Fill);
                    GSlot->SetVerticalAlignment(VAlign_Fill);
                }
            }

            SlotWidget->InitSlot(TrackedContainer.Get(), SlotIndex);

            URiftItemInstance* Item = TrackedContainer->GetItemAtSlot(SlotIndex);
            if (IsValid(Item))
            {
                SlotWidget->SetItem(Item, TrackedContainer.Get(), SlotIndex);
            }
        }
    }
}

void URiftContainerWidget::OnItemMoved(URiftItemInstance* Item, URiftContainer* FromContainer, URiftContainer* ToContainer)
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

    if (ContainerDefinition->GetLayoutType() == EContainerLayoutType::List)
    {
        // Any list change (reorder, cross-container in, cross-container out) — rebuild
        // entirely so slot order always matches the authoritative container state.
        BuildSlots();
        return;
    }

    // Grid: refresh only slots whose content changed.
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
            continue;
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

void URiftContainerWidget::BindToInventory()
{
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    InventoryComponent->OnItemAdded.AddDynamic(this, &URiftContainerWidget::OnItemAdded);
    InventoryComponent->OnItemRemoved.AddDynamic(this, &URiftContainerWidget::OnItemRemoved);
    InventoryComponent->OnItemMoved.AddDynamic(this, &URiftContainerWidget::OnItemMoved);
}

void URiftContainerWidget::UnbindFromInventory()
{
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    InventoryComponent->OnItemAdded.RemoveDynamic(this, &URiftContainerWidget::OnItemAdded);
    InventoryComponent->OnItemRemoved.RemoveDynamic(this, &URiftContainerWidget::OnItemRemoved);
    InventoryComponent->OnItemMoved.RemoveDynamic(this, &URiftContainerWidget::OnItemMoved);
    InventoryComponent->OnInventoryInitialized.RemoveDynamic(this, &URiftContainerWidget::OnInventoryInitialized);
}

int32 URiftContainerWidget::FindSlotIndexOfItem(URiftItemInstance* Item) const
{
    if (!TrackedContainer.IsValid() || !IsValid(Item))
    {
        return INDEX_NONE;
    }

    return TrackedContainer->GetSlotIndexOfItem(Item);
}

bool URiftContainerWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    URiftDragDropOperation* RiftOp = Cast<URiftDragDropOperation>(InOperation);
    if (!RiftOp || !TrackedContainer.IsValid() || !InventoryComponent.IsValid() || !IsValid(ContainerDefinition))
    {
        return false;
    }

    // Only handle drops for List containers — Grid slots cover every cell and handle drops themselves.
    if (ContainerDefinition->GetLayoutType() != EContainerLayoutType::List)
    {
        return false;
    }

    // Find the first empty slot within capacity.
    const int32 Capacity = ContainerDefinition->GetCapacity();
    int32 TargetSlot = INDEX_NONE;
    for (int32 i = 0; i < Capacity; ++i)
    {
        if (!IsValid(TrackedContainer->GetItemAtSlot(i)))
        {
            TargetSlot = i;
            break;
        }
    }

    if (TargetSlot == INDEX_NONE)
    {
        return false; // Container full.
    }

    InventoryComponent->Server_MoveItemToContainerAtSlotByObject(
        RiftOp->SourceContainer, RiftOp->SourceSlotIndex,
        TrackedContainer.Get(), TargetSlot);

    return true;
}

void URiftContainerWidget::AppendListSlot(URiftItemInstance* Item, int32 SlotIndex)
{
    UPanelWidget* ItemPanel = GetItemPanel();
    if (!IsValid(ItemPanel) || !IsValid(ItemSlotWidgetClass) || !TrackedContainer.IsValid())
    {
        return;
    }

    // Grow SlotWidgets to fit this index if needed.
    if (SlotWidgets.Num() <= SlotIndex)
    {
        SlotWidgets.SetNum(SlotIndex + 1);
    }

    URiftItemSlotWidget* SlotWidget = CreateWidget<URiftItemSlotWidget>(GetOwningPlayer(), ItemSlotWidgetClass);
    if (!IsValid(SlotWidget))
    {
        return;
    }

    SlotWidgets[SlotIndex] = SlotWidget;
    ItemPanel->AddChild(SlotWidget);
    SlotWidget->InitSlot(TrackedContainer.Get(), SlotIndex);

    if (IsValid(Item))
    {
        SlotWidget->SetItem(Item, TrackedContainer.Get(), SlotIndex);
    }
}
