#include "Abilities/URiftAbility_Unequip.h"
#include "Components/URiftEquipmentComponent.h"
#include "Tags/RiftVaultTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"

URiftAbility_Unequip::URiftAbility_Unequip(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // Register the gameplay event that activates this ability.
    // Without this, SendGameplayEventToActor(Tag_Rift_Ability_Unequip) finds no matching ability.
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = Tag_Rift_Ability_Unequip;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    ActivationBlockedTags.AddTag(Tag_Rift_Status_Inventory_Busy);
}

void URiftAbility_Unequip::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!TriggerEventData)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftAbility_Unequip: Activated without TriggerEventData. Use SendGameplayEventToActor with Tag_Rift_Ability_Unequip."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Slot to unequip: first tag in TargetTags
    TArray<FGameplayTag> TagArray;
    TriggerEventData->TargetTags.GetGameplayTagArray(TagArray);
    if (TagArray.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftAbility_Unequip: TriggerEventData.TargetTags is empty — no slot tag provided."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    const FGameplayTag SlotTag = TagArray[0];

    APawn* OwningPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
    URiftEquipmentComponent* EquipComp = OwningPawn
        ? OwningPawn->FindComponentByClass<URiftEquipmentComponent>()
        : nullptr;

    if (!EquipComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftAbility_Unequip: No URiftEquipmentComponent on avatar pawn."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const bool bSuccess = EquipComp->UnequipItem(SlotTag);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, !bSuccess);
}
