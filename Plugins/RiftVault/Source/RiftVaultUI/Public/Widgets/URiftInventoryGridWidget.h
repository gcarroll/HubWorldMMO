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
 * - Finds the inventory component from the owning player
 * - Spawns slot widgets for each item as they are added
 * - Removes slot widgets when items are removed
 * - Reads container tag and capacity from ContainerDefinition
 *
 * Blueprint subclasses only need to:
 * - Set ItemSlotWidgetClass
 * - Set ContainerDefinition
 * - Bind any UPanelWidget (UniformGridPanel, WrapBox, etc) to ItemGrid
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

protected:

    /** The widget class to spawn for each item slot. Must be a subclass of URiftItemSlotWidget. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RiftVault|Grid")
    TSubclassOf<URiftItemSlotWidget> ItemSlotWidgetClass;

    /** The container definition this grid displays. Drives both the tag and capacity. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RiftVault|Grid")
    TObjectPtr<URiftContainerDefinition> ContainerDefinition;

    /** Number of columns per row in the grid. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RiftVault|Grid")
    int32 ColumnsPerRow = 2;

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
    void OnInventoryInitialized(bool bSuccess);

private:

    TWeakObjectPtr<URiftInventoryComponent> InventoryComponent;
    TWeakObjectPtr<URiftContainer> TrackedContainer;

    /** All spawned slot widgets. */
    UPROPERTY()
    TArray<TObjectPtr<URiftItemSlotWidget>> SlotWidgets;

    void BuildSlots();
    void AddSlotForItem(URiftItemInstance* Item);
    void BindToInventory();
    void UnbindFromInventory();
    URiftInventoryComponent* FindInventoryComponent() const;
    int32 FindSlotIndexOfItem(URiftItemInstance* Item) const;
};
