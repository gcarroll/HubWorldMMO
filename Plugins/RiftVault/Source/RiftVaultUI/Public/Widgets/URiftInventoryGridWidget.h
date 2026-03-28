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
 * Base class for inventory grid widgets.
 *
 * Handles all slot management in C++:
 * - Displays one slot widget per slot in the tracked container
 * - Wires delegates so the grid stays in sync as items are added, removed, or moved
 * - Reads container capacity and grid width from ContainerDefinition
 *
 * Blueprint subclasses must:
 * - Set ItemSlotWidgetClass
 * - Set ContainerDefinition
 * - Bind a UPanelWidget (UniformGridPanel, WrapBox, etc.) to ItemGrid
 * - Call SetInventoryComponent with the component that owns the inventory to display.
 *   This can be on a PlayerState, Pawn, vendor actor, or any other actor — the grid
 *   widget does not assume any particular location.
 */
UCLASS(Abstract, DisplayName = "Rift Inventory Grid Widget")
class RIFTVAULTUI_API URiftInventoryGridWidget : public URiftBaseWidget
{
    GENERATED_BODY()

public:

    URiftInventoryGridWidget(const FObjectInitializer& ObjectInitializer);

    // -- Begin UUserWidget interface
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
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
     * The panel to populate with slots. Bind this to any UPanelWidget subclass
     * in your Blueprint — UniformGridPanel, WrapBox, etc.
     */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UPanelWidget> ItemGrid;

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
};
