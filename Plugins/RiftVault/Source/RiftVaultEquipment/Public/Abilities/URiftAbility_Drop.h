#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "URiftAbility_Drop.generated.h"

/**
 * GAS ability that drops an item from the inventory into the world as a pickup actor.
 *
 * Activate via gameplay event Tag_Rift_Ability_Drop:
 *   EventData.OptionalObject = URiftItemInstance* to drop
 *
 * Example C++:
 *   FGameplayEventData Payload;
 *   Payload.OptionalObject = ItemInstance;
 *   UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, Tag_Rift_Ability_Drop, Payload);
 *
 * Example Blueprint: use "Send Gameplay Event To Actor" node with the same setup.
 *
 * The ability runs server-only (NetExecutionPolicy = ServerOnly).
 * Auto-granted by URiftEquipmentComponent when bAutoGrantEquipAbilities is true.
 */
UCLASS(Blueprintable)
class RIFTVAULTEQUIPMENT_API URiftAbility_Drop : public UGameplayAbility
{
    GENERATED_BODY()

public:

    URiftAbility_Drop(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
