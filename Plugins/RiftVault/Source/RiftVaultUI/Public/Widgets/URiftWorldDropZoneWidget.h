#pragma once

#include "CoreMinimal.h"
#include "Widgets/URiftBaseWidget.h"
#include "Templates/SubclassOf.h"
#include "URiftWorldDropZoneWidget.generated.h"

class ARiftPickup;
class URiftDragDropOperation;
class URiftItemInstance;

/**
 * A drag-and-drop target that drops items into the world when released onto it.
 *
 * Place this widget in your inventory UI as an explicit "drop to world" zone
 * (e.g. a ground icon or "Drop" label). This works alongside the automatic
 * world-drop that fires when an item is dragged completely outside the
 * inventory UI (via URiftItemSlotWidget::OnDragCancelledOutside).
 *
 * Drop rules (via URiftInventoryComponent::Server_DropItem):
 *   - Items must have URiftFragment_Drop with bDroppable = true.
 *   - Items without URiftFragment_Drop or with bDroppable = false are rejected.
 *
 * Blueprint overrides:
 *   - OnDragEntered / OnDragLeft — show/hide world-drop highlight.
 *   - OnItemDroppedToWorld        — play VFX or sound after the world drop.
 *   - OnDropRejected              — show feedback when item cannot be dropped.
 *
 * Subclass in Blueprint to add the visual (icon, label, highlight overlay).
 */
UCLASS(Abstract, DisplayName = "Rift World Drop Zone Widget")
class RIFTVAULTUI_API URiftWorldDropZoneWidget : public URiftBaseWidget
{
    GENERATED_BODY()

public:

    // -- Begin UUserWidget interface
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) override;
    virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) override;
    virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) override;
    // -- End UUserWidget interface

protected:

    /**
     * Radius (cm) to search for an existing ARiftPickup to consolidate into.
     * Set to 0 to always spawn a new pickup.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop", meta = (UIMin = 0, ClampMin = 0))
    float ConsolidationRadius = 200.f;

    /**
     * Pickup actor class to spawn. Leave null to use the default ARiftPickup.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop")
    TSubclassOf<ARiftPickup> PickupClass;

    /**
     * Called when a drag enters this zone.
     * Override in Blueprint to show a drop highlight.
     *
     * @param Operation  The drag operation. Use Operation->ItemInstance to inspect the item.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|World Drop Zone")
    void OnDragEntered(URiftDragDropOperation* Operation);
    virtual void OnDragEntered_Implementation(URiftDragDropOperation* Operation);

    /**
     * Called when a drag leaves this zone without dropping.
     * Override in Blueprint to clear the drop highlight.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|World Drop Zone")
    void OnDragLeft(URiftDragDropOperation* Operation);
    virtual void OnDragLeft_Implementation(URiftDragDropOperation* Operation);

    /**
     * Called after an item is successfully dropped into the world.
     * Override in Blueprint to play a VFX or sound.
     *
     * @param DroppedItem  The item instance that was removed from inventory (now invalid).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|World Drop Zone")
    void OnItemDroppedToWorld(URiftItemInstance* DroppedItem);
    virtual void OnItemDroppedToWorld_Implementation(URiftItemInstance* DroppedItem);

    /**
     * Called when an item is dragged onto this zone but is not droppable
     * (no URiftFragment_Drop or bDroppable = false).
     * Override in Blueprint to show a rejection indicator.
     *
     * @param RejectedItem  The item that was not dropped.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|World Drop Zone")
    void OnDropRejected(URiftItemInstance* RejectedItem);
    virtual void OnDropRejected_Implementation(URiftItemInstance* RejectedItem);
};
