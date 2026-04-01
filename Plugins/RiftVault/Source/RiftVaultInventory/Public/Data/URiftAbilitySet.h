#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "URiftAbilitySet.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UAttributeSet;
class UAbilitySystemComponent;

/**
 * Tracks every grant made by a single URiftAbilitySet::GrantToASC call.
 *
 * Pass one of these to GrantToASC; the struct is populated with handles that can
 * later be passed to RemoveFromASC to undo the grants atomically.
 *
 * Accumulation: if you call GrantToASC on multiple ability sets, you can either
 * use one handle struct per set (and call RemoveFromASC on each individually) or
 * pass the same struct to each call so all handles are collected together.
 * URiftEquipmentComponent uses one struct per equipment slot to revoke everything
 * for that slot in a single RemoveFromASC call.
 */
USTRUCT(BlueprintType)
struct RIFTVAULTINVENTORY_API FRiftAbilitySetGrantedHandles
{
    GENERATED_BODY()

    /**
     * Removes all abilities, effects, and attribute sets that were granted by the
     * corresponding GrantToASC call. Safe to call on an already-cleared struct (no-op).
     *
     * @param ASC  The ability system component that received the original grants.
     */
    void RemoveFromASC(UAbilitySystemComponent* ASC);

    /** Returns true if this struct holds at least one active grant. */
    bool HasAnyGrants() const;

    /** Handles returned by ASC->GiveAbility — one per granted ability. */
    UPROPERTY()
    TArray<FGameplayAbilitySpecHandle> AbilityHandles;

    /** Handles returned by ASC->ApplyGameplayEffectToSelf — one per granted effect. */
    UPROPERTY()
    TArray<FActiveGameplayEffectHandle> EffectHandles;

    /**
     * Attribute set instances created and registered with the ASC.
     * Stored as weak pointers because the ASC owns the lifetime of these objects.
     */
    UPROPERTY()
    TArray<TWeakObjectPtr<UAttributeSet>> AttributeSetInstances;
};

/**
 * Data asset that bundles one or more gameplay abilities, gameplay effects, and
 * attribute sets into a single grant operation.
 *
 * Usage — equipping an item:
 *   FRiftAbilitySetGrantedHandles Handles;
 *   AbilitySet->GrantToASC(ASC, ItemInstance, Handles);
 *   // store Handles in GrantedAbilitySetHandles[SlotTag]
 *
 * Usage — unequipping:
 *   FRiftAbilitySetGrantedHandles& Handles = GrantedAbilitySetHandles[SlotTag];
 *   Handles.RemoveFromASC(ASC);
 *
 * All three grant types are optional — an ability set that only grants abilities
 * and no effects / attribute sets is perfectly valid.
 */
UCLASS(BlueprintType, Const)
class RIFTVAULTINVENTORY_API URiftAbilitySet : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    /**
     * Grants all abilities, effects, and attribute sets in this data asset to the
     * given ability system component, appending the resulting handles to OutHandles.
     *
     * Appends rather than overwrites so callers can accumulate grants from multiple
     * ability sets into a single handle struct for atomic revocation later.
     *
     * @param ASC          The target ability system component.
     * @param SourceObject The UObject to associate with each ability spec as its source
     *                     (typically the URiftItemInstance). Ability code can retrieve
     *                     this via GetCurrentSourceObject() during execution.
     * @param OutHandles   Receives all handles produced by this call.
     */
    void GrantToASC(UAbilitySystemComponent* ASC, UObject* SourceObject, FRiftAbilitySetGrantedHandles& OutHandles) const;

protected:

    /**
     * Gameplay ability classes to grant when this set is applied.
     * Each ability is granted at level 1 with SourceObject set to the equipped item.
     * Empty means no abilities are granted (e.g. a pure stat-boost set).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

    /**
     * Gameplay effect classes to apply when this set is granted.
     * Applied as infinite effects (or with their own duration policy) at level 1.
     * Active effect handles are stored in OutHandles for removal on unequip.
     * Empty means no effects are applied.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
    TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;

    /**
     * Attribute set classes to instantiate and add to the ASC when this set is granted.
     * Each class is instantiated once per grant call; instances are stored in OutHandles.
     * Empty means no attribute sets are added.
     *
     * Note: attribute sets added at runtime via this mechanism are typically used for
     * item-provided temporary stats. Ensure your GAS setup handles their removal cleanly.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AttributeSets")
    TArray<TSubclassOf<UAttributeSet>> GrantedAttributeSets;
};
