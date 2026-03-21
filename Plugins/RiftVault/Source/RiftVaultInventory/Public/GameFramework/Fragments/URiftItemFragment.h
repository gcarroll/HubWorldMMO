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
     * Returns true if the given item instance contains this fragment's state.
     * Useful for quick checks before accessing state data.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments")
    bool ItemHasState(const URiftItemInstance* Item) const;

    /**
     * Returns true if the given item has authority (is server-side or standalone).
     * All state mutations must be gated on this check.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments")
    static bool ItemHasAuthority(const URiftItemInstance* Item);

    /**
     * Returns true if the given event tag is in this fragment's WatchedEventTags.
     * Used by the inventory component to route events to relevant fragments only.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments")
    bool IsRelevantEvent(FGameplayTag EventTag) const;

    /**
     * Called during item initialization. Subclasses create and save their
     * FRiftFragmentState here if they need per-instance runtime data.
     *
     * Only called server-side. Do not assume client execution.
     *
     * @param Item  The item instance being initialized.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Fragments")
    void InitializeState(URiftItemInstance* Item);
    virtual void InitializeState_Implementation(URiftItemInstance* Item);

    /**
     * Called after initialization when the item is fully active in the inventory.
     * Use this to apply effects, grant abilities, or register listeners.
     *
     * Only called server-side.
     *
     * @param Item  The item instance that is now active.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Fragments")
    void ActivateForItem(URiftItemInstance* Item);
    virtual void ActivateForItem_Implementation(URiftItemInstance* Item);

    /**
     * Called when the item is being removed from the inventory.
     * Use this to remove effects, revoke abilities, or unregister listeners.
     *
     * Only called server-side.
     *
     * @param Item  The item instance being deactivated.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Fragments")
    void DeactivateForItem(URiftItemInstance* Item);
    virtual void DeactivateForItem_Implementation(URiftItemInstance* Item);

    /**
     * Called when a relevant gameplay event is broadcast for this item.
     * Only fires if the event tag matches one in WatchedEventTags.
     *
     * @param Item          The item receiving the event.
     * @param EventTag      The gameplay tag identifying the event.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Fragments")
    void HandleItemEvent(URiftItemInstance* Item, FGameplayTag EventTag);
    virtual void HandleItemEvent_Implementation(URiftItemInstance* Item, FGameplayTag EventTag);

    /**
     * Appends this fragment's gameplay tags to the provided container.
     * Called by URiftItemInstance::GetOwnedGameplayTags.
     *
     * @param Item          The item requesting its tags. May be null.
     * @param TagContainer  The container to append tags into.
     */
    virtual void AppendFragmentTags(const URiftItemInstance* Item, FGameplayTagContainer& TagContainer) const;

protected:

    /**
     * Tags this fragment contributes to the owning item instance.
     * For example URiftFragment_Equippable would contribute Tag_Rift_Item_Trait_Equippable.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fragment")
    FGameplayTagContainer FragmentTags;

    /**
     * Gameplay event tags this fragment wants to respond to.
     * Events matching these tags will be routed to HandleItemEvent.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fragment", meta = (Categories = "Rift.Event"))
    FGameplayTagContainer WatchedEventTags;
};
