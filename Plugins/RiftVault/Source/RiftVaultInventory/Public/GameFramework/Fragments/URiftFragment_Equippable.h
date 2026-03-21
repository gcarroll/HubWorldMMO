#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Fragments/URiftItemFragment.h"
#include "URiftFragment_Equippable.generated.h"

/**
 * Marks an item as equippable and defines what happens when it is equipped.
 *
 * No per-instance state — equipment state is tracked by URiftEquipmentComponent
 * on the pawn, not on the item instance itself.
 */
UCLASS(DisplayName = "Equippable Fragment")
class RIFTVAULTINVENTORY_API URiftFragment_Equippable : public URiftItemFragment
{
    GENERATED_BODY()

public:

    URiftFragment_Equippable();

    /** The slot this item occupies when equipped (e.g. Rift.Slot.Weapon.Primary). */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Equippable")
    FORCEINLINE FGameplayTag GetEquipmentSlotTag() const { return EquipmentSlotTag; }

    /** Abilities granted to the owner's ASC when this item is equipped. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Equippable")
    FORCEINLINE TArray<TSubclassOf<UGameplayAbility>> GetGrantedAbilities() const { return GrantedAbilities; }

    /** Socket name on the character mesh to attach a weapon actor to. Empty for body equipment. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Equippable")
    FORCEINLINE FName GetWeaponSocketName() const { return WeaponSocketName; }

protected:

    /**
     * The gameplay tag identifying the equipment slot this item occupies.
     * Must match one of the Tag_Rift_Slot_* tags.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable", meta = (Categories = "Rift.Slot"))
    FGameplayTag EquipmentSlotTag;

    /**
     * Abilities granted to the owning ASC when this item is equipped.
     * Removed when the item is unequipped.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable")
    TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

    /**
     * Socket name on the character's Mutable mesh to attach ARiftWeaponActor to.
     * Leave empty for body equipment (armor, helmets) that uses Mutable parameters directly.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable")
    FName WeaponSocketName;
};
