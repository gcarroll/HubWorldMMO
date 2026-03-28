#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Fragments/URiftFragment_Equippable_GAS.h"
#include "URiftFragment_Equippable_Weapon.generated.h"

/**
 * Extends URiftFragment_Equippable_GAS with weapon-specific data.
 * Use this fragment on weapon items (swords, rifles, pistols, shields).
 * Armor items that need GAS abilities use URiftFragment_Equippable_GAS directly;
 * armor items without GAS abilities use the base URiftFragment_Equippable.
 *
 * Weapon actor attachment:
 *   - PrimarySocketName:   the main-hand socket on the character's body mesh.
 *                          Optional — leave empty if no primary weapon actor is needed.
 *   - OffHandSocketName:   socket for a second actor (shield, or two-handed grip).
 *                          Optional — leave empty if unused.
 *   Both socket names are designer-defined and must match sockets on the
 *   Reference Skeletal Mesh that feeds into the character's CustomizableObject.
 *
 * Weapon state:
 *   - WeaponStateTag:  pushed to the owning pawn's ASC (as a loose tag) when this
 *                      weapon is equipped, and removed when it is unequipped.
 *                      Mover movement modes and Animation Blueprints read this tag
 *                      to switch locomotion stance and animation layers.
 *                      Use tags under Rift.Weapon.State.* (e.g. Rift.Weapon.State.Rifle).
 */
UCLASS(DisplayName = "Weapon Fragment")
class RIFTVAULTINVENTORY_API URiftFragment_Equippable_Weapon : public URiftFragment_Equippable_GAS
{
    GENERATED_BODY()

public:

    /**
     * Socket name on the character's body mesh for the primary hand weapon actor.
     * Must match a socket defined on the CO's Reference Skeletal Mesh.
     * Leave empty if no primary hand weapon actor is needed for this item.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Weapon")
    FORCEINLINE FName GetPrimarySocketName() const { return PrimarySocketName; }

    /**
     * Socket name on the character's body mesh for the off-hand weapon actor.
     * Used for shields or two-handed weapon grips.
     * Must match a socket defined on the CO's Reference Skeletal Mesh.
     * Leave empty if no off-hand actor is needed.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Weapon")
    FORCEINLINE FName GetOffHandSocketName() const { return OffHandSocketName; }

    /**
     * Gameplay tag applied as a loose tag to the pawn's ASC when this weapon is equipped.
     * Removed automatically on unequip.
     * Mover and Animation Blueprints read this to switch movement mode and animation layer.
     * Should be a child of Rift.Weapon.State (e.g. Rift.Weapon.State.Rifle).
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Weapon")
    FORCEINLINE FGameplayTag GetWeaponStateTag() const { return WeaponStateTag; }

    /**
     * The ARiftWeaponActor subclass to spawn for this weapon.
     * Set this to a Blueprint subclass that has the weapon's CustomizableObject assigned
     * to its WeaponMesh component. If left empty, URiftEquipmentComponent falls back to
     * its own WeaponActorClass, then to the base ARiftWeaponActor.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Weapon")
    FORCEINLINE TSubclassOf<AActor> GetWeaponActorClass() const { return WeaponActorClass; }


protected:

    /** Primary hand socket name. Designer types the socket name from their Reference Skeletal Mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    FName PrimarySocketName;

    /** Off-hand socket name. For shields or two-handed grips. Leave empty if unused. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    FName OffHandSocketName;

    /**
     * Weapon stance tag pushed to the pawn's ASC on equip.
     * Restricts the tag picker to the Rift.Weapon.State namespace.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon",
        meta = (Categories = "Rift.Weapon.State"))
    FGameplayTag WeaponStateTag;

    /**
     * The ARiftWeaponActor subclass to spawn for this weapon.
     * Create a Blueprint subclass of ARiftWeaponActor, assign the weapon's
     * CustomizableObject to its WeaponMesh component, then set it here.
     * Falls back to URiftEquipmentComponent::WeaponActorClass if left empty.
     * Typed as AActor to avoid a circular module dependency with RiftVaultEquipment;
     * URiftEquipmentComponent validates and casts it to ARiftWeaponActor at spawn time.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    TSubclassOf<AActor> WeaponActorClass;

};
