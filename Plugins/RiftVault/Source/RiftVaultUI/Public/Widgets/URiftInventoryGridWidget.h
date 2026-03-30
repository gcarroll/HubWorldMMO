#pragma once

#include "CoreMinimal.h"
#include "Widgets/URiftBaseWidget.h"
#include "URiftInventoryGridWidget.generated.h"

class URiftItemSlotWidget;
class URiftInventoryComponent;
class URiftItemInstance;
class URiftContainer;
class URiftContainerDefinition;
class UPanelWidget;

/**
 * Base class for inventory container widgets. Supports both Grid and List layouts
 * driven by the EContainerLayoutType on the assigned URiftContainerDefinition.
 *
 * Handles all slot management in C++:
 * - Displays one slot widget per slot in the tracked container
 * - Wires delegates so the widget stays in sync as items are added, removed, or moved
 * - Reads SlotCount, ColumnCount, and LayoutType from ContainerDefinition
 *
 * Blueprint subclasses must:
 * - Set ItemSlotWidgetClass
 * - Set ContainerDefinition
 * - Override GetItemPanel and return the panel widget to populate:
 *     Grid layout — UUniformGridPanel or UGridPanel (row/column set automatically)
 *     List layout — UVerticalBox, UHorizontalBox, UWrapBox, etc. (AddChild only)
 * - Call SetInventoryOwner with any actor that implements IRiftInventoryInterface.
 */
UCLASS(Abstract, DisplayName = "Rift Container Widget")
class RIFTVAULTUI_API URiftContainerWidget : public URiftBaseWidget
{
    GENERATED_BODY()

public:

    URiftContainerWidget(const FObjectInitializer& ObjectInitializer);

    // -- Begin UUserWidget interface
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    // -- End UUserWidget interface

    /**
     * Wires this grid to the inventory owned by the given object.
     * The object must implement IRiftInventoryInterface — any actor class can do this
     * in C++ or Blueprint regardless of where it stores its URiftInventoryComponent.
     * Calling this a second time switches to the new owner cleanly.
     *
     * @param InventoryOwner  Any UObject that implements IRiftInventoryInterface.
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|Grid")
    void SetInventoryOwner(UObject* InventoryOwner);

protected:

    /** The widget class to spawn for each item slot. Must be a subclass of URiftItemSlotWidget. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RiftVault|Grid")
    TSubclassOf<URiftItemSlotWidget> ItemSlotWidgetClass;

    /** The container definition this grid displays. Drives both the definition lookup and capacity. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RiftVault|Grid")
    TObjectPtr<URiftContainerDefinition> ContainerDefinition;

    /**
     * Return the panel widget to populate with slots.
     * Override in Blueprint and return whichever panel you placed in your layout —
     * UniformGridPanel, WrapBox, VerticalBox, etc. No naming convention required.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "RiftVault|Grid")
    UPanelWidget* GetItemPanel() const;

    UFUNCTION()
    void OnItemAdded(URiftItemInstance* Item, URiftContainer* Container);

    UFUNCTION()
    void OnItemRemoved(URiftItemInstance* Item, URiftContainer* Container);

    UFUNCTION()
    void OnItemMoved(URiftItemInstance* Item, URiftContainer* FromContainer, URiftContainer* ToContainer);

    UFUNCTION()
    void OnInventoryInitialized(bool bSuccess);

private:

    TWeakObjectPtr<URiftInventoryComponent> InventoryComponent;
    TWeakObjectPtr<URiftContainer> TrackedContainer;

    /** All spawned slot widgets. */
    UPROPERTY()
    TArray<TObjectPtr<URiftItemSlotWidget>> SlotWidgets;

    void BuildSlots();
    void BindToInventory();
    void UnbindFromInventory();
    int32 FindSlotIndexOfItem(URiftItemInstance* Item) const;
    void AppendListSlot(URiftItemInstance* Item, int32 SlotIndex);
};
