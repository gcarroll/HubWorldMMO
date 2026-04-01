#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "URiftItemFragment.generated.h"

class URiftItemInstance;

/**
 * Base class for all RiftVault item fragments.
 *
 * Items in RiftVault are defined by composition rather than inheritance.
 * A URiftItemFragment subclass adds a specific capability or data group to
 * a URiftItemDefinition. A sword might have URiftFragment_Equippable,
 * URiftFragment_Display, URiftFragment_Value, and URiftFragment_Condition.
 * A potion might only have URiftFragment_Stack and URiftFragment_Display.
 *
 * Fragments live in RiftVaultInventory (not Core) because their methods
 * take URiftItemInstance* parameters — placing them in Core would create
 * a circular dependency. This is a closed architectural decision.
 *
 * Fragment lifecycle per item:
 *   1. InitializeState  — creates per-instance runtime state (FRiftFragmentState subclass).
 *   2. ActivateForItem  — applies effects once the item is live in the inventory.
 *   3. DeactivateForItem — removes effects when the item is removed.
 *
 * Event routing:
 *   - WatchedEventTags declares which gameplay events this fragment cares about.
 *   - URiftInventoryComponent::BroadcastItemEvent routes matching events to HandleItemEvent.
 *
 * To create a new fragment:
 *   1. Subclass URiftItemFragment.
 *   2. Add designer-authored properties (type-level data).
 *   3. If per-instance runtime data is needed, create a FRiftFragmentState
 *      subclass and override InitializeState() to set it up.
 *   4. Override ActivateForItem() / DeactivateForItem() if the fragment
 *      needs to do work when an item becomes active or is removed.
 */
UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew)
class RIFTVAULTINVENTORY_API URiftItemFragment : public UObject
{
    GENERATED_BODY()

public:

    URiftItemFragment();

    /**
     * Returns true if the given item instance already has a state entry for this fragment.
     * Useful for quick checks before accessing state data to avoid unnecessary allocations.
     *
     * @param Item  The item to inspect.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments")
    bool ItemHasState(const URiftItemInstance* Item) const;

    /**
     * Returns true if the given item has authority (is server-side or standalone).
     * All state mutations must be gated on this check — clients must never write state.
     *
     * @param Item  The item to check authority for.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments")
    static bool ItemHasAuthority(const URiftItemInstance* Item);

    /**
     * Returns true if the given event tag is in this fragment's WatchedEventTags.
     * Uses HasTagExact — no parent-tag matching — so tags must be precise.
     * Used by the inventory component to route events to relevant fragments only.
     *
     * @param EventTag  The gameplay tag to test.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments")
    bool IsRelevantEvent(FGameplayTag EventTag) const;

    /**
     * Called during item initialization to create and save per-instance state.
     * Subclasses override this to call Item->SaveStateForFragment with their
     * FRiftFragmentState subclass.
     *
     * Only called server-side. Do not assume client execution.
     * Default implementation is a no-op — fragments without per-instance state
     * do not need to override this.
     *
     * @param Item  The item instance being initialized.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Fragments")
    void InitializeState(URiftItemInstance* Item);
    virtual void InitializeState_Implementation(URiftItemInstance* Item);

    /**
     * Called after initialization when the item is fully active in the inventory.
     * Use this to apply GAS effects, grant abilities, or register external listeners.
     *
     * Only called server-side.
     * Default implementation is a no-op.
     *
     * @param Item  The item instance that is now active.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Fragments")
    void ActivateForItem(URiftItemInstance* Item);
    virtual void ActivateForItem_Implementation(URiftItemInstance* Item);

    /**
     * Called when the item is being removed from the inventory.
     * Use this to remove GAS effects, revoke abilities, or unregister listeners.
     * Guaranteed to be called before the item instance is destroyed.
     *
     * Only called server-side.
     * Default implementation is a no-op.
     *
     * @param Item  The item instance being deactivated.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Fragments")
    void DeactivateForItem(URiftItemInstance* Item);
    virtual void DeactivateForItem_Implementation(URiftItemInstance* Item);

    /**
     * Called when a relevant gameplay event is broadcast for this item.
     * Only fires if the event tag exactly matches one in WatchedEventTags.
     * Routed by URiftInventoryComponent::BroadcastItemEvent.
     *
     * Default implementation is a no-op.
     *
     * @param Item      The item receiving the event.
     * @param EventTag  The gameplay tag identifying the event.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Fragments")
    void HandleItemEvent(URiftItemInstance* Item, FGameplayTag EventTag);
    virtual void HandleItemEvent_Implementation(URiftItemInstance* Item, FGameplayTag EventTag);

    /**
     * Appends this fragment's gameplay tags to the provided container.
     * Called by URiftItemInstance::GetOwnedGameplayTags (and by URiftItemDefinition
     * during RefreshDynamicTags with Item = nullptr for type-level tag collection).
     *
     * Base implementation appends FragmentTags. Subclasses (e.g. URiftFragment_Equippable)
     * override this to also contribute slot-specific or derived tags.
     *
     * @param Item          The item requesting its tags. May be null for definition-level queries.
     * @param TagContainer  The container to append tags into.
     */
    virtual void AppendFragmentTags(const URiftItemInstance* Item, FGameplayTagContainer& TagContainer) const;

public:

    /**
     * Name of the ViewModel on the slot widget's MVVM View to wire when this
     * fragment is present on an item. Must match the ViewModel name set in the
     * slot widget Blueprint's MVVM settings exactly.
     *
     * Leave empty (None) if this fragment has no associated ViewModel.
     * The slot widget will call SetItemInstance on the matching ViewModel
     * automatically when the item is set, and ClearItemInstance when cleared.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments")
    FORCEINLINE FName GetViewModelName() const { return ViewModelName; }

protected:

    /**
     * See GetViewModelName. Set this in the fragment's constructor or in
     * Blueprint defaults to declare which ViewModel this fragment drives.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ViewModel")
    FName ViewModelName;

    /**
     * Tags this fragment contributes to the owning item instance.
     * For example URiftFragment_Equippable contributes Tag_Rift_Item_Trait_Equippable,
     * and URiftFragment_Stack contributes Tag_Rift_Item_Trait_Stackable.
     * These tags are used by container compatibility queries and UI checks.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fragment")
    FGameplayTagContainer FragmentTags;

    /**
     * Gameplay event tags this fragment wants to respond to via HandleItemEvent.
     * The inventory component's BroadcastItemEvent checks IsRelevantEvent (which uses
     * HasTagExact) to decide which fragments receive each event.
     * Filtered to Rift.Event namespace via the Categories meta tag in the editor.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fragment", meta = (Categories = "Rift.Event"))
    FGameplayTagContainer WatchedEventTags;
};
