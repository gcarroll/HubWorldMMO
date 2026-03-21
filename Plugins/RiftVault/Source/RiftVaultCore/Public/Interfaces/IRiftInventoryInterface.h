#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/Interface.h"
#include "IRiftInventoryInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class URiftInventoryInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Allows any actor to advertise that it owns a URiftInventoryComponent.
 *
 * Returns UActorComponent* rather than URiftInventoryComponent* because
 * URiftInventoryComponent lives in RiftVaultInventory — referencing it here
 * in Core would create a cross-module UHT violation. Callers cast the result
 * to URiftInventoryComponent* at the call site.
 */
class RIFTVAULTCORE_API IRiftInventoryInterface
{
    GENERATED_BODY()

public:

    /**
     * Returns the inventory component owned by this actor.
     * Cast the result to URiftInventoryComponent* at the call site.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Interfaces|Inventory")
    UActorComponent* GetInventoryComponent() const;
    virtual UActorComponent* GetInventoryComponent_Implementation() const;
};
