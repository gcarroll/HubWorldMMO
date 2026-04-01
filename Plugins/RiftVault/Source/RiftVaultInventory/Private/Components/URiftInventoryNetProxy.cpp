#include "Components/URiftInventoryNetProxy.h"

#include "Components/URiftInventoryComponent.h"
#include "Net/UnrealNetwork.h"

ARiftInventoryNetProxy::ARiftInventoryNetProxy()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bOnlyRelevantToOwner = true;
    NetDormancy = DORM_DormantAll;
}

void ARiftInventoryNetProxy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ThisClass, OwningInventory);
}

void ARiftInventoryNetProxy::InitializeForInventory(URiftInventoryComponent* InventoryComponent)
{
    if (HasAuthority() && !IsValid(OwningInventory) && IsValid(InventoryComponent))
    {
        OwningInventory = InventoryComponent;
    }
}
