#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "URiftInventoryNetProxy.generated.h"

class URiftInventoryComponent;

/**
 * Actor that provides network authority for inventory operations.
 * Spawned and owned by the player controller. Allows clients to
 * execute server-authoritative inventory operations via Server RPCs.
 *
 * Kept separate from URiftInventoryComponent to isolate replication
 * concerns from inventory logic.
 */
UCLASS()
class RIFTVAULTINVENTORY_API ARiftInventoryNetProxy : public AActor
{
    GENERATED_BODY()

public:

    ARiftInventoryNetProxy();

    // -- Begin AActor interface
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    // -- End AActor interface

    /**
     * Binds this proxy to an inventory component.
     * Must be called server-side immediately after spawning.
     */
    void InitializeForInventory(URiftInventoryComponent* InventoryComponent);

protected:

    /** The inventory component this proxy serves. */
    UPROPERTY(Replicated)
    TObjectPtr<URiftInventoryComponent> OwningInventory;
};
