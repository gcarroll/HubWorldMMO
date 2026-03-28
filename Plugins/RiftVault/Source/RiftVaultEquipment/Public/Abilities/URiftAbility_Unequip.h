#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "URiftAbility_Unequip.generated.h"

/**
 * GAS ability that unequips an item from an equipment slot.
 *
 * Activate via gameplay event Tag_Rift_Ability_Unequip:
 *   EventData.TargetTags = FGameplayTagContainer containing the slot tag to unequip
 *
 * Example C++:
 *   FGameplayEventData Payload;
 *   Payload.TargetTags.AddTag(Tag_Rift_Slot_Weapon_Primary);
 *   UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, Tag_Rift_Ability_Unequip, Payload);
 *
 * The ability runs server-only (NetExecutionPolicy = ServerOnly).
 * Grant this ability to the owning ASC at game start alongside URiftAbility_Equip.
 */
UCLASS(Blueprintable)
class RIFTVAULTEQUIPMENT_API URiftAbility_Unequip : public UGameplayAbility
{
    GENERATED_BODY()

public:

    URiftAbility_Unequip(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
