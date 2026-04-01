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
 * concerns from inventory logic. The component lives on APlayerState
 * (which has limited RPC support); this actor provides a dedicated,
 * cleanly owned RPC surface.
 *
 * Network notes:
 *   - bOnlyRelevantToOwner = true  → Only the owning client receives this actor.
 *   - NetDormancy = DORM_DormantAll → No per-tick relevancy checks; wakes on demand.
 *   - OwningInventory is replicated so the client can resolve the component reference.
 */
UCLASS()
class RIFTVAULTINVENTORY_API ARiftInventoryNetProxy : public AActor
{
    GENERATED_BODY()

public:

    ARiftInventoryNetProxy();

    // -- Begin AActor interface
    /** Registers OwningInventory for replication to the owning client. */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    // -- End AActor interface

    /**
     * Binds this proxy to an inventory component.
     * Must be called server-side immediately after spawning.
     * Guards against double-binding by checking IsValid(OwningInventory).
     *
     * @param InventoryComponent  The inventory this proxy will serve.
     */
    void InitializeForInventory(URiftInventoryComponent* InventoryComponent);

protected:

    /**
     * The inventory component this proxy serves.
     * Replicated so the owning client can resolve back to the component.
     * Set once on the server via InitializeForInventory; never changes afterwards.
     */
    UPROPERTY(Replicated)
    TObjectPtr<URiftInventoryComponent> OwningInventory;
};
