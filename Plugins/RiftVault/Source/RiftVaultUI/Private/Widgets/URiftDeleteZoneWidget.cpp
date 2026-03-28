#include "Widgets/URiftDeleteZoneWidget.h"
#include "Components/URiftInventoryComponent.h"
#include "DragDrop/URiftDragDropOperation.h"
#include "GameFramework/Fragments/URiftFragment_Drop.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "GameFramework/PlayerState.h"

bool URiftDeleteZoneWidget::NativeOnDrop(const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    URiftDragDropOperation* RiftOp = Cast<URiftDragDropOperation>(InOperation);
    if (!IsValid(RiftOp) || !IsValid(RiftOp->ItemInstance))
    {
        return false;
    }

    URiftItemInstance* Item = RiftOp->ItemInstance;

    URiftFragment_Drop* DropFrag = Cast<URiftFragment_Drop>(
        Item->FindFragmentByClass(URiftFragment_Drop::StaticClass()));

    const bool bCanDelete = !DropFrag || DropFrag->IsDeletable();

    if (!bCanDelete)
    {
        OnDeleteRejected(Item);
        return true; // Consume the drag so it doesn't fall through to other widgets.
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
        InventoryComp->Server_DeleteItem(
            RiftOp->SourceContainer->GetContainerTag(),
            RiftOp->SourceSlotIndex);
        OnItemDeleted(Item);
    }

    return true;
}

void URiftDeleteZoneWidget::NativeOnDragEnter(const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (URiftDragDropOperation* RiftOp = Cast<URiftDragDropOperation>(InOperation))
    {
        OnDragEntered(RiftOp);
    }
}

void URiftDeleteZoneWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    if (URiftDragDropOperation* RiftOp = Cast<URiftDragDropOperation>(InOperation))
    {
        OnDragLeft(RiftOp);
    }
}

void URiftDeleteZoneWidget::OnDragEntered_Implementation(URiftDragDropOperation* Operation) {}
void URiftDeleteZoneWidget::OnDragLeft_Implementation(URiftDragDropOperation* Operation) {}
void URiftDeleteZoneWidget::OnItemDeleted_Implementation(URiftItemInstance* DeletedItem) {}
void URiftDeleteZoneWidget::OnDeleteRejected_Implementation(URiftItemInstance* RejectedItem) {}
