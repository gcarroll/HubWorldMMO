#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/Interface.h"
#include "IRiftInventoryInterface.generated.h"

// UInterface boilerplate required by UHT — not used directly.
UINTERFACE(MinimalAPI, Blueprintable)
class URiftInventoryInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Implemented by any actor that owns a URiftInventoryComponent.
 *
 * WHY UActorComponent* INSTEAD OF URiftInventoryComponent*?
 * ---------------------------------------------------------
 * URiftInventoryComponent is declared in the RiftVaultInventory module.
 * IRiftInventoryInterface lives in RiftVaultCore. If Core were to include
 * the RiftVaultInventory public headers, it would create a circular module
 * dependency (RiftVaultInventory already depends on RiftVaultCore).
 *
 * Returning UActorComponent* breaks the cycle: callers that actually need to
 * operate on the inventory component cast the result at their own call site:
 *
 *   UActorComponent* Comp = Actor->Execute_GetInventoryComponent(Actor);
 *   URiftInventoryComponent* Inventory = Cast<URiftInventoryComponent>(Comp);
 *
 * Blueprintable is set so that Blueprint actors can implement the interface
 * and override GetInventoryComponent in a Blueprint graph when they do not
 * use a C++ base class that already implements it.
 */
class RIFTVAULTCORE_API IRiftInventoryInterface
{
    GENERATED_BODY()

public:

    /**
     * Returns the inventory component owned by this actor.
     *
     * The default C++ implementation returns nullptr. Override in the concrete
     * actor class to return the actual URiftInventoryComponent subobject.
     *
     * Callers should cast the result to URiftInventoryComponent* before use:
     *   URiftInventoryComponent* Inv = Cast<URiftInventoryComponent>(GetInventoryComponent());
     *
     * @return The actor's URiftInventoryComponent, or nullptr if not implemented.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Interfaces|Inventory")
    UActorComponent* GetInventoryComponent() const;
    virtual UActorComponent* GetInventoryComponent_Implementation() const;
};
