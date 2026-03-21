#include "Widgets/URiftItemSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/URiftInventoryComponent.h"
#include "DragDrop/URiftDragDropOperation.h"
#include "GameFramework/Containers/URiftContainer.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "Settings/URiftInventorySettings.h"
#include "ViewModels/URiftViewModel_ItemDisplay.h"
#include "View/MVVMView.h"

URiftItemSlotWidget::URiftItemSlotWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , SlotIndex(INDEX_NONE)
{
}

void URiftItemSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void URiftItemSlotWidget::SetItem(URiftItemInstance* NewItemInstance, URiftContainer* NewContainer, int32 NewSlotIndex)
{
    ItemInstance = NewItemInstance;
    Container = NewContainer;
    SlotIndex = NewSlotIndex;
    WireViewModel();
    OnItemSet(NewItemInstance);
}

void URiftItemSlotWidget::ClearItem()
{
    ItemInstance.Reset();
    Container.Reset();
    SlotIndex = INDEX_NONE;

    // Clear the ViewModel via the MVVM view
    if (UMVVMView* View = GetExtension<UMVVMView>())
    {
        TScriptInterface<INotifyFieldValueChanged> VM = View->GetViewModel(FName("ItemDisplayViewModel"));
        if (URiftViewModel_ItemDisplay* DisplayVM = Cast<URiftViewModel_ItemDisplay>(VM.GetObject()))
        {
            DisplayVM->ClearItemInstance();
        }
    }

    OnItemCleared();
}

FReply URiftItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && HasItem())
    {
        return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void URiftItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    if (!HasItem())
    {
        return;
    }

    OutOperation = URiftDragDropOperation::CreateRiftDragDropOperation(
        ItemInstance.Get(),
        Container.Get(),
        SlotIndex,
        GetEffectiveDragDropClass()
    );
}

bool URiftItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    URiftDragDropOperation* RiftOperation = Cast<URiftDragDropOperation>(InOperation);
    if (!IsValid(RiftOperation))
    {
        return false;
    }

    OnItemDropped(RiftOperation);
    return true;
}

void URiftItemSlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    URiftDragDropOperation* RiftOperation = Cast<URiftDragDropOperation>(InOperation);
    if (IsValid(RiftOperation))
    {
        OnDragEntered(RiftOperation);
    }
}

void URiftItemSlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    URiftDragDropOperation* RiftOperation = Cast<URiftDragDropOperation>(InOperation);
    if (IsValid(RiftOperation))
    {
        OnDragLeft(RiftOperation);
    }
}

void URiftItemSlotWidget::OnItemDropped_Implementation(URiftDragDropOperation* Operation)
{
    if (!IsValid(Operation) || !Operation->SourceContainer || !Container.IsValid())
    {
        return;
    }

    URiftItemInstance* DraggedItem = Operation->ItemInstance;
    if (!IsValid(DraggedItem))
    {
        return;
    }

    // Container's outer is the URiftInventoryComponent that created it
    URiftInventoryComponent* InventoryComponent = Cast<URiftInventoryComponent>(Operation->SourceContainer->GetOuter());
    if (!IsValid(InventoryComponent))
    {
        return;
    }

    if (HasItem())
    {
        // Swap — move dragged item here, move this item to source
        InventoryComponent->MoveItemToContainer(DraggedItem, Container.Get());
        InventoryComponent->MoveItemToContainer(ItemInstance.Get(), Operation->SourceContainer);
    }
    else
    {
        // Move to empty slot
        InventoryComponent->MoveItemToContainer(DraggedItem, Container.Get());
    }
}

void URiftItemSlotWidget::OnDragEntered_Implementation(URiftDragDropOperation* Operation)
{
    // Override in Blueprint to show highlight
}

void URiftItemSlotWidget::OnDragLeft_Implementation(URiftDragDropOperation* Operation)
{
    // Override in Blueprint to clear highlight
}

TSubclassOf<URiftDragDropOperation> URiftItemSlotWidget::GetEffectiveDragDropClass() const
{
    if (IsValid(DragDropOperationClass))
    {
        return DragDropOperationClass;
    }

    const URiftInventorySettings* Settings = GetDefault<URiftInventorySettings>();
    if (IsValid(Settings) && IsValid(Settings->DragDropOperationClass))
    {
        return Settings->DragDropOperationClass;
    }

    return URiftDragDropOperation::StaticClass();
}

void URiftItemSlotWidget::WireViewModel()
{
    if (!ItemInstance.IsValid())
    {
        return;
    }

    if (UMVVMView* View = GetExtension<UMVVMView>())
    {
        TScriptInterface<INotifyFieldValueChanged> VM = View->GetViewModel(FName("ItemDisplayViewModel"));
        if (URiftViewModel_ItemDisplay* DisplayVM = Cast<URiftViewModel_ItemDisplay>(VM.GetObject()))
        {
            DisplayVM->SetItemInstance(ItemInstance.Get());
            UE_LOG(LogTemp, Log, TEXT("URiftItemSlotWidget::WireViewModel — ViewModel updated for item at slot %d"), SlotIndex);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("URiftItemSlotWidget::WireViewModel — No ItemDisplayViewModel found on MVVMView"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftItemSlotWidget::WireViewModel — No MVVMView found on widget"));
    }
}
