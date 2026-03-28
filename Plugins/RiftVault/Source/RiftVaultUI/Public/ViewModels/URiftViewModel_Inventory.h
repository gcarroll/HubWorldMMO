#pragma once

#include "CoreMinimal.h"
#include "ViewModels/URiftViewModel.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "URiftViewModel_Inventory.generated.h"

class URiftInventoryComponent;
class URiftContainer;

/**
 * ViewModel for the inventory grid UI.
 *
 * Keeps a flat list of URiftItemInstance pointers that mirrors the contents
 * of the player's URiftInventoryComponent. Widgets bind to GetItems() and
 * GetItemCount() via MVVM FieldNotify so they automatically refresh whenever
 * items are added or removed.
 *
 * Lifecycle:
 *   1. URiftViewModelResolver asks URiftInventoryUISubsystem for this ViewModel.
 *   2. The subsystem creates one instance and calls InitializeViewModel.
 *   3. The owning HUD or PlayerController calls SetInventoryComponent() once the
 *      pawn and its components are available.
 *   4. The ViewModel binds to OnItemAdded / OnItemRemoved delegates.
 *   5. On shutdown, UnbindFromInventoryComponent() is called and the Items array
 *      is emptied.
 *
 * This ViewModel is a singleton — one instance is shared across all widgets that
 * display the player's inventory (bIsSingleton = true in InitializeViewModel).
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "Inventory ViewModel")
class RIFTVAULTUI_API URiftViewModel_Inventory : public URiftViewModel
{
    GENERATED_BODY()

public:

    /** Sets bIsSingleton = true so the subsystem caches and reuses this instance. */
    virtual void InitializeViewModel_Implementation(const UUserWidget* UserWidget) override;

    /** Unbinds from the inventory component and empties the cached Items array. */
    virtual void ShutdownViewModel_Implementation() override;

    /**
     * Connects this ViewModel to a URiftInventoryComponent.
     *
     * Call this from the owning HUD or PlayerController once the pawn is ready.
     * Safe to call again on pawn respawn — it unbinds from the old component first.
     *
     * @param NewInventoryComponent  The inventory component to observe. May be null to
     *                               disconnect without providing a replacement.
     */
    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModels|Inventory")
    void SetInventoryComponent(URiftInventoryComponent* NewInventoryComponent);

    /**
     * Returns the current list of valid item instances in the inventory.
     *
     * FieldNotify: MVVM bindings will re-evaluate any Blueprint binding tied to
     * this function whenever UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItems) is
     * called (i.e. on OnItemAdded, OnItemRemoved, and RefreshItems).
     *
     * Filters out any null/invalid entries that may have accumulated due to GC.
     */
    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Inventory")
    TArray<URiftItemInstance*> GetItems() const;

    /**
     * Returns the number of items currently in the Items array.
     *
     * FieldNotify: Notified alongside GetItems so count-dependent widgets (e.g. a
     * "X items" label) stay in sync without needing a separate delegate.
     */
    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Inventory")
    int32 GetItemCount() const;

    /**
     * Delegate handler bound to URiftInventoryComponent::OnItemAdded.
     * Adds the item to the cached array and broadcasts FieldNotify changes.
     *
     * @param Item       The item that was added. Validity-checked before insertion.
     * @param Container  The container the item landed in (unused here — the ViewModel
     *                   tracks all items regardless of which container they're in).
     */
    UFUNCTION()
    void OnItemAdded(URiftItemInstance* Item, URiftContainer* Container);

    /**
     * Delegate handler bound to URiftInventoryComponent::OnItemRemoved.
     * Removes the item from the cached array and broadcasts FieldNotify changes.
     *
     * @param Item       The item that was removed.
     * @param Container  The container the item was removed from (unused here).
     */
    UFUNCTION()
    void OnItemRemoved(URiftItemInstance* Item, URiftContainer* Container);

protected:

    /**
     * The authoritative cached list of items this ViewModel exposes to the UI.
     *
     * FieldNotify: Changes to this array are broadcast to all MVVM-bound widgets via
     * UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED. Using TObjectPtr keeps items alive across
     * GC passes while they are referenced here.
     */
    UPROPERTY(FieldNotify, BlueprintReadOnly)
    TArray<TObjectPtr<URiftItemInstance>> Items;

private:

    /**
     * Weak reference to the inventory component being observed.
     * Weak so the ViewModel does not prevent the component (and its owner pawn) from
     * being garbage collected when the pawn is destroyed or replaced.
     */
    TWeakObjectPtr<URiftInventoryComponent> InventoryComponent;

    /**
     * Subscribes to OnItemAdded and OnItemRemoved on the current InventoryComponent.
     * No-op if InventoryComponent is not valid.
     */
    void BindToInventoryComponent();

    /**
     * Unsubscribes from OnItemAdded and OnItemRemoved on the current InventoryComponent.
     * No-op if InventoryComponent is not valid (handles the case where the pawn was
     * destroyed before we could unbind).
     */
    void UnbindFromInventoryComponent();

    /**
     * Clears and repopulates the Items array from the current InventoryComponent's state.
     * Called after SetInventoryComponent to ensure the initial snapshot is accurate.
     * Broadcasts both GetItems and GetItemCount FieldNotify notifications afterward.
     */
    void RefreshItems();
};
