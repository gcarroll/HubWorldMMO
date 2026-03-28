#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Fragments/URiftFragment_Equippable.h"
#include "URiftFragment_Equippable_GAS.generated.h"

class URiftAbilitySet;

/**
 * GAS-aware extension of URiftFragment_Equippable.
 *
 * Adds a list of URiftAbilitySet data assets that URiftEquipmentComponent grants to
 * the pawn's UAbilitySystemComponent when this item is equipped. The grants are
 * revoked atomically when the item is unequipped.
 *
 * Use this fragment when the equipped item should grant abilities, gameplay effects,
 * or temporary attribute sets (e.g. a sword that grants a slash ability, a chest
 * piece that applies a passive armour effect, a ring that increases max health).
 *
 * If you do not use GAS in your project, use URiftFragment_Equippable instead and
 * implement your own equip/unequip logic via URiftEquipmentComponent::OnItemEquipped
 * / OnItemUnequipped delegates.
 */
UCLASS(DisplayName = "Equippable Fragment (GAS)")
class RIFTVAULTINVENTORY_API URiftFragment_Equippable_GAS : public URiftFragment_Equippable
{
    GENERATED_BODY()

public:

    /**
     * Returns the list of ability sets to grant when this item is equipped.
     * Not a UFUNCTION — UHT does not support const TArray<T*>& return in UFUNCTION.
     * All callers (URiftEquipmentComponent) are C++.
     */
    FORCEINLINE const TArray<TObjectPtr<URiftAbilitySet>>& GetAbilitySets() const { return AbilitySets; }

protected:

    /**
     * Ability sets granted to the pawn's ASC when this item is equipped.
     * Each set can contain abilities, gameplay effects, and attribute sets.
     * All grants from all sets in this array are revoked together when the item is unequipped.
     * Empty means no GAS grants are made (same behaviour as URiftFragment_Equippable).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
    TArray<TObjectPtr<URiftAbilitySet>> AbilitySets;
};
