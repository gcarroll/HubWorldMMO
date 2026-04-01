#include "Widgets/URiftItemSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/URiftInventoryComponent.h"
#include "DragDrop/URiftDragDropOperation.h"
#include "GameFramework/Containers/URiftContainer.h"
#include "GameFramework/Fragments/URiftFragment_Display.h"
#include "GameFramework/Fragments/URiftFragment_Drop.h"
#include "GameFramework/Fragments/URiftFragment_Stack.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "Settings/URiftInventorySettings.h"
#include "ViewModels/URiftViewModel_Fragment.h"
#include "ViewModels/URiftViewModel_ItemDisplay.h"
#include "Data/URiftItemDefinition.h"
#include "View/MVVMView.h"

URiftItemSlotWidget::URiftItemSlotWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , SlotIndex(INDEX_NONE)
{
}

void URiftItemSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (HasItem())
    {
        // Widget was pre-populated before construction (drag visual path).
        // MVVMView extension is now available so wire the ViewModel.
        WireViewModel();
    }
    else
    {
        if (ItemIcon)       { ItemIcon->SetRenderOpacity(0.f); }
        if (ItemDescription){ ItemDescription->SetVisibility(ESlateVisibility::Collapsed); }
    }
}

void URiftItemSlotWidget::InitSlot(URiftContainer* InContainer, int32 InSlotIndex)
{
    Container  = InContainer;
    SlotIndex  = InSlotIndex;
}

void URiftItemSlotWidget::InitAsDragVisual(URiftItemInstance* InItemInstance, URiftContainer* InContainer, int32 InSlotIndex)
{
    ItemInstance = InItemInstance;
    Container    = InContainer;
    SlotIndex    = InSlotIndex;

    // Show the icon/description without calling WireViewModel or OnItemSet.
    // The ViewModel is not available until the widget is added to the viewport,
    // so calling SetItem here would cause a Blueprint null-access on ItemDisplayViewModel.
    if (ItemIcon)       { ItemIcon->SetRenderOpacity(1.f); }
    if (ItemDescription){ ItemDescription->SetVisibility(ESlateVisibility::HitTestInvisible); }
}

void URiftItemSlotWidget::SetItem(URiftItemInstance* NewItemInstance, URiftContainer* NewContainer, int32 NewSlotIndex)
{
    ItemInstance = NewItemInstance;
    Container    = NewContainer;
    SlotIndex    = NewSlotIndex;

    if (ItemIcon)        { ItemIcon->SetRenderOpacity(1.f); }
    if (ItemDescription) { ItemDescription->SetVisibility(ESlateVisibility::HitTestInvisible); }

    // Auto-populate icon and description from the Display fragment.
    // This removes the need for any Blueprint code in OnItemSet for basic display.
    if (IsValid(NewItemInstance))
    {
        if (const URiftFragment_Display* DisplayFrag = NewItemInstance->FindFragment<URiftFragment_Display>())
        {
            if (ItemIcon)
            {
                ItemIcon->SetBrushFromTexture(DisplayFrag->GetIcon(), false);
            }
            if (ItemDescription)
            {
                ItemDescription->SetText(DisplayFrag->GetDisplayName());
            }
        }
    }

    WireViewModel();
    OnItemSet(NewItemInstance);
}

void URiftItemSlotWidget::ClearItem()
{
    ItemInstance.Reset();
    // Preserve Container and SlotIndex so empty slots can still accept drops.

    if (ItemIcon)
    {
        ItemIcon->SetRenderOpacity(0.f);
        ItemIcon->SetBrushFromTexture(nullptr, false);
    }
    if (ItemDescription)
    {
        ItemDescription->SetVisibility(ESlateVisibility::Collapsed);
        ItemDescription->SetText(FText::GetEmpty());
    }

    ClearViewModels();
    OnItemCleared();
}

FReply URiftItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasItem())
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
        -1,
        GetEffectiveDragDropClass()
    );

    if (IsValid(OutOperation))
    {
        // If a dedicated drag visual class is set, create an instance of it.
        // Otherwise fall back to using this slot widget itself as the visual.
        if (DragVisualClass)
        {
            UUserWidget* DragVisual = CreateWidget<UUserWidget>(GetOwningPlayer(), DragVisualClass);
            if (DragVisual)
            {
                // Always initialise base state (opacity, item data) first so the
                // icon is visible regardless of which path runs next.
                if (URiftItemSlotWidget* SlotVisual = Cast<URiftItemSlotWidget>(DragVisual))
                {
                    SlotVisual->InitAsDragVisual(ItemInstance.Get(), Container.Get(), SlotIndex);
                }

                // Let the interface override with custom visuals (e.g. icon texture).
                if (DragVisual->Implements<URiftDragVisualInterface>())
                {
                    IRiftDragVisualInterface::Execute_OnDragItemSet(DragVisual, ItemInstance.Get());
                }

                OutOperation->DefaultDragVisual = DragVisual;
            }
            else
            {
                OutOperation->DefaultDragVisual = this;
            }
        }
        else
        {
            // Create a fresh copy of this widget class as the visual.
            // Do NOT use 'this' — OnItemMoved fires ClearItem/SetItem on the source slot
            // during the drop, which corrupts the drag visual and breaks subsequent drags.
            UUserWidget* DragVisual = CreateWidget<UUserWidget>(GetOwningPlayer(), GetClass());
            if (IsValid(DragVisual))
            {
                if (DragVisual->Implements<URiftDragVisualInterface>())
                {
                    IRiftDragVisualInterface::Execute_OnDragItemSet(DragVisual, ItemInstance.Get());
                }
                else if (URiftItemSlotWidget* SlotVisual = Cast<URiftItemSlotWidget>(DragVisual))
                {
                    SlotVisual->InitAsDragVisual(ItemInstance.Get(), Container.Get(), SlotIndex);
                }
                OutOperation->DefaultDragVisual = DragVisual;
            }
            else
            {
                OutOperation->DefaultDragVisual = this;
            }
        }
        OutOperation->Pivot = EDragPivot::MouseDown;
    }

}

bool URiftItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    URiftDragDropOperation* RiftOperation = Cast<URiftDragDropOperation>(InOperation);
    if (!IsValid(RiftOperation))
    {
        return false;
    }

    // Call _Implementation directly to bypass BlueprintNativeEvent dispatch.
    // A Blueprint subclass with an OnItemDropped stub (even an empty one) would
    // intercept the virtual call and prevent the C++ logic from running.
    // Blueprints that need to react to drops should override OnItemSet / OnItemCleared.
    OnItemDropped_Implementation(RiftOperation);
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

void URiftItemSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

    // Restore visuals — the item was not moved so this slot still owns it.
    if (HasItem())
    {
        if (ItemIcon)       { ItemIcon->SetRenderOpacity(1.f); }
        if (ItemDescription){ ItemDescription->SetVisibility(ESlateVisibility::HitTestInvisible); }
    }

    // If the item is droppable and landed nowhere, send it to the world.
    URiftDragDropOperation* RiftOp = Cast<URiftDragDropOperation>(InOperation);
    if (!IsValid(RiftOp) || !IsValid(RiftOp->ItemInstance)) return;

    URiftFragment_Drop* DropFrag = RiftOp->ItemInstance->FindFragment<URiftFragment_Drop>();

    if (DropFrag && DropFrag->IsDroppable())
    {
        OnDragCancelledOutside(RiftOp);
    }
}

void URiftItemSlotWidget::OnItemDropped_Implementation(URiftDragDropOperation* Operation)
{
    if (!IsValid(Operation) || !IsValid(Operation->SourceContainer) || !Container.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("OnItemDropped: early exit — invalid operation or container"));
        return;
    }

    URiftItemInstance* DraggedItem = Operation->ItemInstance;
    if (!IsValid(DraggedItem))
    {
        UE_LOG(LogTemp, Warning, TEXT("OnItemDropped: early exit — DraggedItem is null"));
        return;
    }

    URiftInventoryComponent* InventoryComponent = Cast<URiftInventoryComponent>(Operation->SourceContainer->GetOuter());
    if (!IsValid(InventoryComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("OnItemDropped: early exit — SourceContainer outer is not URiftInventoryComponent (got %s)"),
            Operation->SourceContainer->GetOuter() ? *Operation->SourceContainer->GetOuter()->GetClass()->GetName() : TEXT("null"));
        return;
    }

    const bool bSameContainer = (Operation->SourceContainer == Container.Get());

    if (HasItem())
    {
        URiftItemInstance* TargetItem = ItemInstance.Get();

        // Check if we should merge stacks instead of swapping.
        URiftFragment_Stack* DraggedStackFrag = DraggedItem->FindFragment<URiftFragment_Stack>();
        URiftFragment_Stack* TargetStackFrag = TargetItem->FindFragment<URiftFragment_Stack>();

        const bool bCanMerge = IsValid(DraggedStackFrag)
            && IsValid(TargetStackFrag)
            && DraggedStackFrag->CanMergeWith(DraggedItem, TargetItem);

        if (bCanMerge)
        {
            InventoryComponent->Server_MergeStacks(
                Operation->SourceContainer->GetContainerTag(), Operation->SourceSlotIndex,
                Container->GetContainerTag(), SlotIndex,
                Operation->QuantityToDrag);
        }
        else if (bSameContainer)
        {
            InventoryComponent->Server_MoveItemToSlotByObject(
                Operation->SourceContainer,
                Operation->SourceSlotIndex,
                SlotIndex);
        }
        else
        {
            InventoryComponent->Server_MoveItemToContainerAtSlotByObject(
                Operation->SourceContainer, Operation->SourceSlotIndex,
                Container.Get(), SlotIndex);
        }
    }
    else
    {
        if (bSameContainer)
        {
            InventoryComponent->Server_MoveItemToSlotByObject(
                Operation->SourceContainer,
                Operation->SourceSlotIndex,
                SlotIndex);
        }
        else
        {
            InventoryComponent->Server_MoveItemToContainerAtSlotByObject(
                Operation->SourceContainer, Operation->SourceSlotIndex,
                Container.Get(), SlotIndex);
        }
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

void URiftItemSlotWidget::OnDragCancelledOutside_Implementation(URiftDragDropOperation* Operation)
{
    APawn* Pawn = GetOwningPlayerPawn();
    APlayerState* PS = Pawn ? Pawn->GetPlayerState() : nullptr;

    URiftInventoryComponent* InventoryComp = PS
        ? PS->FindComponentByClass<URiftInventoryComponent>()
        : nullptr;

    if (!InventoryComp && Pawn)
    {
        InventoryComp = Pawn->FindComponentByClass<URiftInventoryComponent>();
    }

    if (IsValid(InventoryComp) && IsValid(Operation->SourceContainer))
    {
        InventoryComp->Server_DropItemByObject(
            Operation->SourceContainer,
            Operation->SourceSlotIndex,
            200.f,
            nullptr);
    }
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
    ActiveFragmentViewModelNames.Empty();

    if (!ItemInstance.IsValid())
    {
        return;
    }

    UMVVMView* View = GetExtension<UMVVMView>();
    if (!View)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftItemSlotWidget::WireViewModel — No MVVMView found on widget"));
        return;
    }

    const URiftItemDefinition* Definition = ItemInstance->GetDefinition();
    if (!IsValid(Definition))
    {
        return;
    }

    for (URiftItemFragment* Fragment : Definition->GetFragments())
    {
        if (!IsValid(Fragment) || Fragment->GetViewModelName().IsNone())
        {
            continue;
        }

        const FName VMName = Fragment->GetViewModelName();
        TScriptInterface<INotifyFieldValueChanged> VM = View->GetViewModel(VMName);
        if (URiftViewModel_Fragment* FragVM = Cast<URiftViewModel_Fragment>(VM.GetObject()))
        {
            FragVM->SetItemInstance(ItemInstance.Get());
            ActiveFragmentViewModelNames.Add(VMName);
            UE_LOG(LogTemp, Log, TEXT("URiftItemSlotWidget::WireViewModel — wired '%s' for slot %d"), *VMName.ToString(), SlotIndex);
        }
    }
}

void URiftItemSlotWidget::ClearViewModels()
{
    if (UMVVMView* View = GetExtension<UMVVMView>())
    {
        for (const FName& VMName : ActiveFragmentViewModelNames)
        {
            TScriptInterface<INotifyFieldValueChanged> VM = View->GetViewModel(VMName);
            if (URiftViewModel_Fragment* FragVM = Cast<URiftViewModel_Fragment>(VM.GetObject()))
            {
                FragVM->ClearItemInstance();
            }
        }
    }

    ActiveFragmentViewModelNames.Empty();
}
