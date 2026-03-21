#pragma once

#include "CoreMinimal.h"
#include "Widgets/URiftBaseWidget.h"
#include "Templates/SubclassOf.h"
#include "URiftItemSlotWidget.generated.h"

class URiftItemInstance;
class URiftContainer;
class URiftDragDropOperation;
class URiftViewModel_ItemDisplay;

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
    // -- End UUserWidget interface

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
     * Override this in Blueprint or C++ to customize drag drop operation class.
     * Falls back to the class set in URiftInventorySettings if null.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RiftVault|Widgets|Item Slot")
    TSubclassOf<URiftDragDropOperation> DragDropOperationClass;

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

private:

    TWeakObjectPtr<URiftItemInstance> ItemInstance;
    TWeakObjectPtr<URiftContainer> Container;
    int32 SlotIndex = INDEX_NONE;

    TSubclassOf<URiftDragDropOperation> GetEffectiveDragDropClass() const;
    void WireViewModel();
};
