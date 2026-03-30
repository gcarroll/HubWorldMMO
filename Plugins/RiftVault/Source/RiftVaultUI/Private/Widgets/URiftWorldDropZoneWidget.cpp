#include "Widgets/URiftWorldDropZoneWidget.h"
#include "Actors/ARiftPickup.h"
#include "Components/URiftInventoryComponent.h"
#include "DragDrop/URiftDragDropOperation.h"
#include "GameFramework/Fragments/URiftFragment_Drop.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "GameFramework/PlayerState.h"

bool URiftWorldDropZoneWidget::NativeOnDrop(const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    URiftDragDropOperation* RiftOp = Cast<URiftDragDropOperation>(InOperation);
    if (!IsValid(RiftOp) || !IsValid(RiftOp->ItemInstance))
    {
        return false;
    }

    URiftItemInstance* Item = RiftOp->ItemInstance;

    URiftFragment_Drop* DropFrag = Item->FindFragment<URiftFragment_Drop>();

    const bool bCanDrop = DropFrag && DropFrag->IsDroppable();

    if (!bCanDrop)
    {
        OnDropRejected(Item);
        return true; // Consume the drag so it doesn't fall through.
    }

    APawn* Pawn = GetOwningPlayerPawn();
    APlayerState* PS = Pawn ? Pawn->GetPlayerState() : nullptr;

    URiftInventoryComponent* InventoryComp = PS
        ? PS->FindComponentByClass<URiftInventoryComponent>()
        : nullptr;

    if (!InventoryComp && Pawn)
    {
        InventoryComp = Pawn->FindComponentByClass<URiftInventoryComponent>();
    }

    if (IsValid(InventoryComp) && IsValid(RiftOp->SourceContainer))
    {
        InventoryComp->Server_DropItemByObject(
            RiftOp->SourceContainer,
            RiftOp->SourceSlotIndex,
            ConsolidationRadius,
            PickupClass);
        OnItemDroppedToWorld(Item);
    }

    return true;
}

void URiftWorldDropZoneWidget::NativeOnDragEnter(const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (URiftDragDropOperation* RiftOp = Cast<URiftDragDropOperation>(InOperation))
    {
        OnDragEntered(RiftOp);
    }
}

void URiftWorldDropZoneWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    if (URiftDragDropOperation* RiftOp = Cast<URiftDragDropOperation>(InOperation))
    {
        OnDragLeft(RiftOp);
    }
}

void URiftWorldDropZoneWidget::OnDragEntered_Implementation(URiftDragDropOperation* Operation) {}
void URiftWorldDropZoneWidget::OnDragLeft_Implementation(URiftDragDropOperation* Operation) {}
void URiftWorldDropZoneWidget::OnItemDroppedToWorld_Implementation(URiftItemInstance* DroppedItem) {}
void URiftWorldDropZoneWidget::OnDropRejected_Implementation(URiftItemInstance* RejectedItem) {}
