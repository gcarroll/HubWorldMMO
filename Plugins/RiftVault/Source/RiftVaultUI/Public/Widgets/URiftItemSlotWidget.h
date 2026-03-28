#pragma once

#include "CoreMinimal.h"
#include "Widgets/URiftBaseWidget.h"
#include "Templates/SubclassOf.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "URiftItemSlotWidget.generated.h"

class URiftItemInstance;
class URiftContainer;
class URiftDragDropOperation;
class URiftViewModel_ItemDisplay;

/**
 * Optional interface for drag visual widgets.
 *
 * Implement this on your drag visual widget Blueprint to receive the item
 * being dragged so you can display its icon, name, or quantity.
 * If the drag visual widget does not implement this interface, it is still
 * used as the visual — it just won't receive the item data automatically.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class URiftDragVisualInterface : public UInterface
{
    GENERATED_BODY()
};

class RIFTVAULTUI_API IRiftDragVisualInterface
{
    GENERATED_BODY()

public:

    /**
     * Called immediately after the drag visual widget is created.
     * Use this to populate the visual with the dragged item's icon, name, quantity, etc.
     *
     * @param Item  The item instance being dragged.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RiftVault|UI|Drag and Drop")
    void OnDragItemSet(URiftItemInstance* Item);
};

/**
 * Base class for all RiftVault item slot widgets.
 *
 * Handles all drag and drop plumbing in C++:
 * - Tracks the item instance, container and slot index this widget represents
 * - Detects drag start and creates URiftDragDropOperation
 * - Handles drop and executes move or swap via URiftInventoryComponent
 * - Wires URiftViewModel_ItemDisplay automatically on SetItem
 *
 * Blueprint subclasses only need to handle visuals.
 * Override DragDropOperationClass to customize drag behavior per widget type.
 */
UCLASS(Abstract, DisplayName = "Rift Item Slot Widget")
class RIFTVAULTUI_API URiftItemSlotWidget : public URiftBaseWidget
{
    GENERATED_BODY()

public:

    URiftItemSlotWidget(const FObjectInitializer& ObjectInitializer);

    // -- Begin UUserWidget interface
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    // -- End UUserWidget interface

    /**
     * Initialises the slot's container and index without setting an item.
     * Called by URiftInventoryGridWidget for every slot (empty or occupied) so
     * the drop handler always knows which container and slot index to target.
     */
    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    void InitSlot(URiftContainer* InContainer, int32 InSlotIndex);

    /**
     * Lightweight initialisation used when this widget is spawned as a drag visual.
     * Sets the item data and makes the icon/description visible, but does NOT call
     * WireViewModel or OnItemSet — the ViewModel is not available on a widget that
     * has not yet been added to the viewport.
     *
     * To display the correct icon texture on the drag visual, implement OnDragItemSet
     * (IRiftDragVisualInterface) in your Blueprint and read directly from the item.
     */
    void InitAsDragVisual(URiftItemInstance* InItemInstance, URiftContainer* InContainer, int32 InSlotIndex);

    /**
     * Sets the item this slot represents.
     * Automatically wires URiftViewModel_ItemDisplay via the subsystem.
     */
    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    void SetItem(URiftItemInstance* NewItemInstance, URiftContainer* NewContainer, int32 NewSlotIndex);

    /**
     * Clears the item from this slot and resets the ViewModel.
     */
    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    void ClearItem();

    UFUNCTION(BlueprintPure, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    URiftItemInstance* GetItemInstance() const { return ItemInstance.Get(); }

    UFUNCTION(BlueprintPure, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    URiftContainer* GetContainer() const { return Container.Get(); }

    UFUNCTION(BlueprintPure, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    int32 GetSlotIndex() const { return SlotIndex; }

    UFUNCTION(BlueprintPure, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    bool HasItem() const { return ItemInstance.IsValid(); }

    /**
     * Called after the item and ViewModel are set.
     * Implement in Blueprint to update visuals — icon, text, etc.
     * The URiftViewModel_ItemDisplay is already wired at this point.
     */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    void OnItemSet(URiftItemInstance* NewItemInstance);

    /**
     * Called after the item is cleared.
     * Implement in Blueprint to reset visuals to empty state.
     */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    void OnItemCleared();

protected:

    /**
     * Optional icon image widget. Name it "ItemIcon" in your Blueprint.
     * Automatically shown when an item is set and hidden (Collapsed) when the slot is empty.
     * Must be set to HitTestInvisible in the Blueprint so it does not block drag input.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RiftVault|Widgets|Item Slot")
    TObjectPtr<UImage> ItemIcon;

    /**
     * Optional description/name text widget. Name it "ItemDescription" in your Blueprint.
     * Automatically shown when an item is set and hidden (Collapsed) when the slot is empty.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RiftVault|Widgets|Item Slot")
    TObjectPtr<UTextBlock> ItemDescription;

    /**
     * Override this in Blueprint or C++ to customize drag drop operation class.
     * Falls back to the class set in URiftInventorySettings if null.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RiftVault|Widgets|Item Slot")
    TSubclassOf<URiftDragDropOperation> DragDropOperationClass;

    /**
     * Optional widget class to use as the drag visual when this slot is dragged.
     * The created widget is passed the item instance via OnSetDragItem so it can
     * display the icon, name, or quantity.
     * If left unset, this slot widget itself is used as the drag visual.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RiftVault|Widgets|Item Slot")
    TSubclassOf<UUserWidget> DragVisualClass;

    /**
     * Called when a valid drop lands on this slot.
     * Default behavior is move or swap via URiftInventoryComponent.
     * Override in Blueprint to add custom logic.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    void OnItemDropped(URiftDragDropOperation* Operation);
    virtual void OnItemDropped_Implementation(URiftDragDropOperation* Operation);

    /**
     * Called when a drag enters this slot.
     * Override in Blueprint to show highlight or valid drop indicator.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    void OnDragEntered(URiftDragDropOperation* Operation);
    virtual void OnDragEntered_Implementation(URiftDragDropOperation* Operation);

    /**
     * Called when a drag leaves this slot.
     * Override in Blueprint to clear highlight.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    void OnDragLeft(URiftDragDropOperation* Operation);
    virtual void OnDragLeft_Implementation(URiftDragDropOperation* Operation);

    /**
     * Called from NativeOnDragCancelled when the dragged item has URiftFragment_Drop
     * with bDroppable = true and the drag ended without a widget accepting it.
     *
     * Default C++ implementation:
     *   Finds URiftInventoryComponent on the owning PlayerState (or pawn) and calls
     *   Server_DropItem on it. Works on standalone, listen-server, and dedicated server.
     *
     * Override in Blueprint to:
     *   - Suppress the world drop in specific contexts (e.g. a drag cancelled by Escape).
     *   - Add VFX or sound before the drop fires.
     *
     * @param Operation  The drag operation carrying the item that was dropped outside.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|Item Slot")
    void OnDragCancelledOutside(URiftDragDropOperation* Operation);
    virtual void OnDragCancelledOutside_Implementation(URiftDragDropOperation* Operation);

private:

    TWeakObjectPtr<URiftItemInstance> ItemInstance;
    TWeakObjectPtr<URiftContainer> Container;
    int32 SlotIndex = INDEX_NONE;

    /** Names of ViewModels currently wired on this slot. Cleared when the item is cleared. */
    TArray<FName> ActiveFragmentViewModelNames;

    TSubclassOf<URiftDragDropOperation> GetEffectiveDragDropClass() const;
    void WireViewModel();
    void ClearViewModels();
};
