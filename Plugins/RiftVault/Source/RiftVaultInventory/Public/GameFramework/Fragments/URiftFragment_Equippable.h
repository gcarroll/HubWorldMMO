#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Fragments/URiftItemFragment.h"
#include "Types/FRiftMutableParameter.h"
#include "URiftFragment_Equippable.generated.h"

/**
 * Marks an item as equippable and defines what happens when it is equipped.
 *
 * No per-instance state is stored on the item — equipment state (currently worn,
 * holstered, etc.) is tracked by URiftEquipmentComponent on the pawn, not on the
 * item instance itself. This keeps the inventory system decoupled from the pawn.
 *
 * This fragment does two things:
 *   1. Declares the item as equippable via Tag_Rift_Item_Trait_Equippable (in FragmentTags).
 *   2. Emits the EquipmentSlotTag via AppendFragmentTags so per-slot containers can
 *      filter by slot (e.g. only a HeadSlot container accepts items with Rift.Slot.Armor.Head).
 *
 * Mutable parameters:
 *   - ActiveMutableParameters: applied when the item is equipped and drawn.
 *   - HolsteredMutableParameters: applied when the item is holstered (visible on back/hip).
 *   These are read by URiftMutableEquipmentComponent and URiftMutableWeaponComponent.
 */
UCLASS(DisplayName = "Equippable Fragment")
class RIFTVAULTINVENTORY_API URiftFragment_Equippable : public URiftItemFragment
{
    GENERATED_BODY()

public:

    URiftFragment_Equippable();

    /**
     * Returns the gameplay tag identifying the equipment slot this item occupies when equipped.
     * Should match one of the Tag_Rift_Slot_* tags (e.g. Rift.Slot.Weapon.Primary).
     * This tag is also emitted by AppendFragmentTags so slot containers can match it.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Equippable")
    FORCEINLINE FGameplayTag GetEquipmentSlotTag() const { return EquipmentSlotTag; }

    /**
     * Appends the equippable trait tag AND the slot tag to the item's tag container.
     * The base class AppendFragmentTags adds FragmentTags (which includes
     * Tag_Rift_Item_Trait_Equippable). This override also emits the EquipmentSlotTag
     * so per-slot container queries (e.g. ANY(Rift.Slot.Armor.Head)) can accept or
     * reject this item based on which slot it belongs to.
     *
     * @param Item          The item requesting its tags. May be null for definition-level queries.
     * @param TagContainer  The container to append tags into.
     */
    virtual void AppendFragmentTags(const URiftItemInstance* Item, FGameplayTagContainer& TagContainer) const override;

    /**
     * Mutable parameters applied to the pawn's body mesh (or weapon mesh) when this item is active.
     * Body equipment: URiftMutableEquipmentComponent reads these to update the pawn's CustomizableObject.
     * Weapons: URiftMutableWeaponComponent reads these to configure ARiftWeaponActor's mesh.
     *
     * Not a UFUNCTION — UHT does not support const TArray<T>& return types in UFUNCTION declarations.
     * All callers (URiftMutableEquipmentComponent, URiftMutableWeaponComponent) are C++.
     */
    FORCEINLINE const TArray<FRiftMutableParameter>& GetActiveMutableParameters() const { return ActiveMutableParameters; }

    /**
     * Mutable parameters applied when this item is holstered (visible on back/hip).
     * Empty means the item is invisible/absent when holstered.
     *
     * Not a UFUNCTION — same reason as GetActiveMutableParameters.
     */
    FORCEINLINE const TArray<FRiftMutableParameter>& GetHolsteredMutableParameters() const { return HolsteredMutableParameters; }

protected:

    /**
     * The gameplay tag identifying the equipment slot this item occupies.
     * Must match one of the Tag_Rift_Slot_* tags defined in RiftVaultTags.h.
     * The Categories meta restricts the tag picker in the editor to the Rift.Slot namespace.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable", meta = (Categories = "Rift.Slot"))
    FGameplayTag EquipmentSlotTag;

    /**
     * Mutable parameters applied when this item is active (equipped and drawn).
     * Body armor/helmets: applied to the pawn's body mesh via URiftMutableEquipmentComponent.
     * Weapons: applied to ARiftWeaponActor's mesh via URiftMutableWeaponComponent.
     * Empty for items that have no visual representation when active.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable|Mutable")
    TArray<FRiftMutableParameter> ActiveMutableParameters;

    /**
     * Mutable parameters applied when this item is holstered (visible on back/hip).
     * Leave empty if the item should not be visible when holstered (e.g. stored in a bag).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable|Mutable")
    TArray<FRiftMutableParameter> HolsteredMutableParameters;
};
