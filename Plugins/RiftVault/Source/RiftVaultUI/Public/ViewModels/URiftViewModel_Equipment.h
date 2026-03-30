#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ViewModels/URiftViewModel.h"
#include "URiftViewModel_Equipment.generated.h"

class URiftEquipmentComponent;
class URiftItemInstance;
class URiftItemDefinition;

/**
 * Lightweight view struct representing one equipment slot.
 *
 * This is a pure data carrier — it exists only in the ViewModel layer and is
 * never written back to gameplay code. Widgets bind to the array of these entries
 * via GetEquipmentSlots() to render all slots in one pass (e.g. with a ListView).
 *
 * Item is nullptr when the slot is empty, which widgets should treat as an empty
 * slot visual (greyed-out placeholder, no icon, etc.).
 */
USTRUCT(BlueprintType)
struct RIFTVAULTUI_API FRiftEquipmentSlotViewEntry
{
    GENERATED_BODY()

    /** The gameplay tag identifying which equipment slot this entry represents (e.g. "Equipment.Slot.Head"). */
    UPROPERTY(BlueprintReadOnly, Category = "RiftVault|UI|Equipment")
    FGameplayTag SlotTag;

    /** The item currently occupying this slot. Nullptr when the slot is empty. */
    UPROPERTY(BlueprintReadOnly, Category = "RiftVault|UI|Equipment")
    TObjectPtr<URiftItemInstance> Item = nullptr;
};

/**
 * ViewModel for equipment slot UI.
 *
 * Exposes a flat array of FRiftEquipmentSlotViewEntry — one entry per supported slot,
 * ordered to match URiftEquipmentComponent::SupportedSlots. Empty slots have Item = nullptr.
 *
 * Bind via URiftViewModelResolver (singleton). Call SetEquipmentComponent() once the pawn
 * is available (e.g. from the owning HUD or PlayerController).
 *
 * MVVM binding pattern:
 *   - Bind a ListView's ItemsSource to GetEquipmentSlots()
 *   - Each list entry widget reads SlotTag and Item from its FRiftEquipmentSlotViewEntry
 *   - When an item is equipped or unequipped, only the affected entry is mutated and the
 *     GetEquipmentSlots FieldNotify fires, triggering a ListView refresh
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "Equipment ViewModel")
class RIFTVAULTUI_API URiftViewModel_Equipment : public URiftViewModel
{
    GENERATED_BODY()

public:

    /** Sets bIsSingleton = true so the subsystem caches and reuses this instance. */
    virtual void InitializeViewModel_Implementation(const UUserWidget* UserWidget) override;

    /** Unbinds from the equipment component and empties the cached EquipmentSlots array. */
    virtual void ShutdownViewModel_Implementation() override;

    /**
     * Connects this ViewModel to a URiftEquipmentComponent.
     *
     * Unbinds from any previous component, stores a weak reference to the new one,
     * binds to its delegates, and takes an immediate snapshot via RefreshSlots().
     * Safe to call again on pawn respawn.
     *
     * @param NewEquipmentComponent  The equipment component to observe. May be null.
     */
    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModels|Equipment")
    void SetEquipmentComponent(URiftEquipmentComponent* NewEquipmentComponent);

    /**
     * Returns the full list of equipment slot view entries.
     *
     * FieldNotify: Bound widgets re-evaluate whenever UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED
     * (GetEquipmentSlots) is called — on equip, unequip, and full refresh.
     *
     * Returns a copy of EquipmentSlots (not a reference) so the MVVM system can diff
     * old and new values to avoid spurious widget updates.
     */
    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Equipment")
    TArray<FRiftEquipmentSlotViewEntry> GetEquipmentSlots() const;

    /**
     * Returns the number of supported equipment slots.
     *
     * FieldNotify: Useful for widgets that display a slot count label or drive a dynamic
     * slot grid that needs to know how many entries to render.
     */
    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Equipment")
    int32 GetSlotCount() const;

    /**
     * Returns the item currently equipped in the given slot, or nullptr if empty.
     * Convenience accessor — does not trigger FieldNotify (read directly from
     * the EquipmentSlots array if you need change notification instead).
     *
     * @param SlotTag  The gameplay tag of the slot to query.
     */
    UFUNCTION(BlueprintPure, BlueprintCosmetic, Category = "RiftVault|UI|ViewModels|Equipment")
    URiftItemInstance* GetItemInSlot(FGameplayTag SlotTag) const;

    /**
     * Delegate handler bound to URiftEquipmentComponent::OnItemEquipped.
     * Finds the matching slot entry and sets its Item pointer, then broadcasts
     * GetEquipmentSlots so the UI reflects the newly equipped item.
     *
     * @param Item     The item that was equipped.
     * @param SlotTag  The slot it was equipped into.
     */
    UFUNCTION()
    void OnItemEquipped(URiftItemInstance* Item, FGameplayTag SlotTag, URiftItemDefinition* ItemDefinition);

    /**
     * Delegate handler bound to URiftEquipmentComponent::OnItemUnequipped.
     * Finds the matching slot entry and clears its Item pointer, then broadcasts
     * GetEquipmentSlots so the UI shows the slot as empty.
     *
     * @param Item     The item that was unequipped.
     * @param SlotTag  The slot it was removed from.
     */
    UFUNCTION()
    void OnItemUnequipped(URiftItemInstance* Item, FGameplayTag SlotTag, URiftItemDefinition* ItemDefinition);

protected:

    /**
     * The authoritative cached list of slot view entries exposed to the UI.
     *
     * FieldNotify: MVVM-bound widgets re-evaluate when UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED
     * is called. Array length equals the number of supported slots on the equipment component.
     */
    UPROPERTY(FieldNotify, BlueprintReadOnly)
    TArray<FRiftEquipmentSlotViewEntry> EquipmentSlots;

private:

    /**
     * Weak reference to the equipment component being observed.
     * Weak so this ViewModel does not prevent the pawn from being garbage collected.
     */
    TWeakObjectPtr<URiftEquipmentComponent> EquipmentComponent;

    /** Returns a pointer to the slot entry matching the given tag, or nullptr if not found. */
    FRiftEquipmentSlotViewEntry* FindSlotByTag(FGameplayTag SlotTag);
    const FRiftEquipmentSlotViewEntry* FindSlotByTag(FGameplayTag SlotTag) const;

    /** Subscribes to OnItemEquipped and OnItemUnequipped delegates. No-op if component is invalid. */
    void BindToEquipmentComponent();

    /**
     * Unsubscribes from OnItemEquipped and OnItemUnequipped delegates.
     * No-op if component is invalid (handles pawn destruction before unbind).
     */
    void UnbindFromEquipmentComponent();

    /**
     * Clears and repopulates EquipmentSlots from the component's current state.
     * Creates one FRiftEquipmentSlotViewEntry per supported slot, querying the component
     * for the current item in each slot. Broadcasts both GetEquipmentSlots and GetSlotCount.
     * Also broadcasts when the component is invalid so widgets see an empty state.
     */
    void RefreshSlots();
};
