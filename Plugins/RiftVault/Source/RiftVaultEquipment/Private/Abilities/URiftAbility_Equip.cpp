#include "Abilities/URiftAbility_Equip.h"
#include "Components/URiftEquipmentComponent.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "Tags/RiftVaultTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"

URiftAbility_Equip::URiftAbility_Equip(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // Register the gameplay event that activates this ability.
    // Without this, SendGameplayEventToActor(Tag_Rift_Ability_Equip) finds no matching ability.
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = Tag_Rift_Ability_Equip;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    // Block while inventory is processing
    ActivationBlockedTags.AddTag(Tag_Rift_Status_Inventory_Busy);
}

void URiftAbility_Equip::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!TriggerEventData)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftAbility_Equip: Activated without TriggerEventData. Use SendGameplayEventToActor with Tag_Rift_Ability_Equip."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Item to equip: passed as OptionalObject
    URiftItemInstance* Item = const_cast<URiftItemInstance*>(Cast<URiftItemInstance>(TriggerEventData->OptionalObject));
    if (!Item)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftAbility_Equip: TriggerEventData.OptionalObject is not a URiftItemInstance."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Target slot: first tag in TargetTags
    TArray<FGameplayTag> TagArray;
    TriggerEventData->TargetTags.GetGameplayTagArray(TagArray);
    if (TagArray.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftAbility_Equip: TriggerEventData.TargetTags is empty — no slot tag provided."));
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
        UE_LOG(LogTemp, Warning, TEXT("URiftAbility_Equip: No URiftEquipmentComponent on avatar pawn."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const bool bSuccess = EquipComp->EquipItem(Item, SlotTag);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, !bSuccess);
}
