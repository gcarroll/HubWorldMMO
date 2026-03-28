#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARiftPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UStaticMesh;
class URiftPickupComponent;
class URiftInventoryComponent;
class URiftItemDefinition;

/**
 * Ready-made world pickup actor: overlap sphere + static mesh + pickup logic.
 *
 * Walk a pawn into it on the server → URiftPickupComponent::TryCollect fires,
 * the item is added to the player's inventory, and the actor is destroyed.
 *
 * Single-item usage (level-placed or loot table spawn):
 *   1. Place ARiftPickup in the level (or spawn dynamically).
 *   2. Set ItemDefinition and Quantity on PickupComponent in the Details panel.
 *   3. Optionally assign a Static Mesh to MeshComponent.
 *
 * Multi-item drop usage (Server_DropItem on URiftInventoryComponent):
 *   - Spawns this actor near the player's feet with no ItemDefinition set.
 *   - Calls AddDroppedItem to populate DropInventory with one or more items.
 *   - TryCollect transfers all items from DropInventory to the collector's inventory.
 *   - Multiple nearby drops consolidate into the same actor's DropInventory.
 *
 * Subclass in Blueprint to override mesh, sphere radius, VFX, or spawn logic.
 */
UCLASS(Blueprintable)
class RIFTVAULTINVENTORY_API ARiftPickup : public AActor
{
    GENERATED_BODY()

public:

    ARiftPickup(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory")
    URiftPickupComponent* GetPickupComponent() const { return PickupComponent; }

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory")
    URiftInventoryComponent* GetDropInventory() const { return DropInventory; }

    /**
     * Adds an item (by definition + quantity) to this pickup's DropInventory.
     * Call this after spawning to populate the pickup.
     * Server only.
     *
     * @param Definition  The item type to add.
     * @param Quantity    How many units to add.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Inventory")
    void AddDroppedItem(URiftItemDefinition* Definition, int32 Quantity);

    /**
     * Sets the visual mesh on MeshComponent. Called after spawn to reflect
     * the dropped item's world appearance.
     *
     * @param Mesh  The static mesh to display. Pass null to hide the mesh.
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|Inventory")
    void SetDropMesh(UStaticMesh* Mesh);

protected:

    /** Replicated mesh set when an item is dropped. OnRep applies it to MeshComponent on clients. */
    UPROPERTY(ReplicatedUsing=OnRep_DropMesh)
    TObjectPtr<UStaticMesh> ReplicatedDropMesh;

    UFUNCTION()
    void OnRep_DropMesh();

    /** Overlap trigger. Set radius in Blueprint to match item size. Default: 64 cm. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> OverlapSphere;

    /** Optional visual mesh. Collision disabled — OverlapSphere handles detection. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    /** Pickup logic: item definition, quantity, destroy-on-collect. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<URiftPickupComponent> PickupComponent;

    /**
     * Inventory that holds all items in this ground-drop pickup.
     * Populated by URiftInventoryComponent::Server_DropItem when items are dropped.
     * When a collector overlaps, TryCollect transfers all items here to the collector's inventory.
     * Null until BeginPlay initializes it with a URiftGroundContainerDefinition.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<URiftInventoryComponent> DropInventory;
};
