#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "URiftAbility_Equip.generated.h"

/**
 * GAS ability that equips an item into an equipment slot.
 *
 * Activate via gameplay event Tag_Rift_Ability_Equip:
 *   EventData.OptionalObject  = URiftItemInstance* to equip
 *   EventData.TargetTags      = FGameplayTagContainer containing the target slot tag
 *
 * Example C++:
 *   FGameplayEventData Payload;
 *   Payload.OptionalObject = ItemInstance;
 *   Payload.TargetTags.AddTag(Tag_Rift_Slot_Weapon_Primary);
 *   UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, Tag_Rift_Ability_Equip, Payload);
 *
 * Example Blueprint: use "Send Gameplay Event To Actor" node with the same setup.
 *
 * The ability runs server-only (NetExecutionPolicy = ServerOnly).
 * Grant this ability to the owning ASC at game start (e.g. in PlayerState::BeginPlay).
 */
UCLASS(Blueprintable)
class RIFTVAULTEQUIPMENT_API URiftAbility_Equip : public UGameplayAbility
{
    GENERATED_BODY()

public:

    URiftAbility_Equip(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
