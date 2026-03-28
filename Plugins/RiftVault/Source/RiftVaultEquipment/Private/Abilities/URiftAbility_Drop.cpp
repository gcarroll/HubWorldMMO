#include "Abilities/URiftAbility_Drop.h"
#include "Components/URiftInventoryComponent.h"
#include "GameFramework/Containers/URiftContainer.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "Tags/RiftVaultTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

URiftAbility_Drop::URiftAbility_Drop(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    // Register the gameplay event that activates this ability.
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = Tag_Rift_Ability_Drop;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    ActivationBlockedTags.AddTag(Tag_Rift_Status_Inventory_Busy);
}

void URiftAbility_Drop::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!TriggerEventData)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftAbility_Drop: Activated without TriggerEventData. Use SendGameplayEventToActor with Tag_Rift_Ability_Drop."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Item to drop: passed as OptionalObject
    URiftItemInstance* Item = const_cast<URiftItemInstance*>(Cast<URiftItemInstance>(TriggerEventData->OptionalObject));
    if (!Item)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftAbility_Drop: TriggerEventData.OptionalObject is not a URiftItemInstance."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* AvatarActor = ActorInfo->AvatarActor.Get();
    if (!AvatarActor)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Find the inventory on the avatar pawn or its PlayerState.
    URiftInventoryComponent* InventoryComp = nullptr;
    if (const APawn* Pawn = Cast<APawn>(AvatarActor))
    {
        if (APlayerState* PS = Pawn->GetPlayerState())
        {
            InventoryComp = PS->FindComponentByClass<URiftInventoryComponent>();
        }
    }
    if (!InventoryComp)
    {
        InventoryComp = AvatarActor->FindComponentByClass<URiftInventoryComponent>();
    }

    if (IsValid(InventoryComp))
    {
        // Ability is ServerOnly so this executes immediately on the server.
        // Resolve container tag + slot index from the live item pointer (valid on server).
        URiftContainer* ItemContainer = InventoryComp->GetContainerForItem(Item);
        const int32 SlotIdx = IsValid(ItemContainer) ? ItemContainer->GetSlotIndexOfItem(Item) : INDEX_NONE;
        if (IsValid(ItemContainer) && SlotIdx != INDEX_NONE)
        {
            InventoryComp->Server_DropItem(ItemContainer->GetContainerTag(), SlotIdx, 200.f, nullptr);
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
