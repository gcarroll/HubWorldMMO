#pragma once

#include "CoreMinimal.h"
#include "Widgets/URiftBaseWidget.h"
#include "URiftDeleteZoneWidget.generated.h"

class URiftDragDropOperation;
class URiftItemInstance;

/**
 * A drag-and-drop target that permanently deletes items dropped onto it.
 *
 * Place this widget in your inventory UI (e.g. a trash can icon).
 * When a player drags an item over it and releases, the item is deleted
 * from their inventory with no world spawn.
 *
 * Deletion rules (via URiftInventoryComponent::Server_DeleteItem):
 *   - Items with URiftFragment_Drop and bDeletable = true  → deleted.
 *   - Items with no URiftFragment_Drop at all              → deleted (always deletable).
 *   - Items with URiftFragment_Drop and bDeletable = false → rejected.
 *
 * Blueprint overrides:
 *   - OnDragEntered / OnDragLeft — show/hide delete highlight.
 *   - OnItemDeleted              — play a VFX or sound after deletion.
 *   - OnDeleteRejected           — show feedback when item cannot be deleted.
 *
 * Subclass in Blueprint to add the visual (icon, label, highlight overlay).
 */
UCLASS(Abstract, DisplayName = "Rift Delete Zone Widget")
class RIFTVAULTUI_API URiftDeleteZoneWidget : public URiftBaseWidget
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
     * Called when a drag enters this zone.
     * Override in Blueprint to show a delete highlight or trash icon animation.
     *
     * @param Operation  The drag operation. Use Operation->ItemInstance to inspect the item.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|Delete Zone")
    void OnDragEntered(URiftDragDropOperation* Operation);
    virtual void OnDragEntered_Implementation(URiftDragDropOperation* Operation);

    /**
     * Called when a drag leaves this zone without dropping.
     * Override in Blueprint to clear the delete highlight.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|Delete Zone")
    void OnDragLeft(URiftDragDropOperation* Operation);
    virtual void OnDragLeft_Implementation(URiftDragDropOperation* Operation);

    /**
     * Called after an item is successfully deleted.
     * Override in Blueprint to play a destruction VFX or sound.
     *
     * @param DeletedItem  The item instance that was removed (now invalid — do not dereference).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|Delete Zone")
    void OnItemDeleted(URiftItemInstance* DeletedItem);
    virtual void OnItemDeleted_Implementation(URiftItemInstance* DeletedItem);

    /**
     * Called when an item is dragged onto this zone but cannot be deleted
     * (URiftFragment_Drop present with bDeletable = false).
     * Override in Blueprint to show a rejection indicator.
     *
     * @param RejectedItem  The item that was not deleted.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = "RiftVault|Widgets|Delete Zone")
    void OnDeleteRejected(URiftItemInstance* RejectedItem);
    virtual void OnDeleteRejected_Implementation(URiftItemInstance* RejectedItem);
};
