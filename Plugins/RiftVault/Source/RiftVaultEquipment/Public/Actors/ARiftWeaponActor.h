#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARiftWeaponActor.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UCustomizableSkeletalComponent;
class URiftMutableWeaponComponent;
class URiftItemInstance;

/**
 * A replicated actor representing an equipped or holstered weapon in the world.
 *
 * Spawned by URiftEquipmentComponent when an item with a non-empty WeaponSocketName is equipped.
 * Attached to the pawn's body mesh (UCustomizableSkeletalComponent tagged Rift.Component.BodyMesh)
 * at the socket named in URiftFragment_Equippable::WeaponSocketName.
 *
 * Contains a UCustomizableSkeletalComponent for the weapon's own Mutable mesh.
 * URiftMutableWeaponComponent configures Mutable parameters after SetupForItem is called.
 *
 * Subclass in Blueprint to set a specific UCustomizableObject asset on the WeaponMeshComponent
 * and to add any additional gameplay components (collision, audio, VFX, etc.).
 */
UCLASS(Blueprintable)
class RIFTVAULTEQUIPMENT_API ARiftWeaponActor : public AActor
{
    GENERATED_BODY()

public:

    ARiftWeaponActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /**
     * Called by URiftEquipmentComponent after spawning and attaching.
     * Passes the item instance to URiftMutableWeaponComponent to configure visuals.
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|Equipment|Weapon")
    void SetupForItem(URiftItemInstance* InItem);

    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment|Weapon")
    URiftItemInstance* GetItemInstance() const { return ItemInstance.Get(); }

    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment|Weapon")
    USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMeshComponent; }

    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment|Weapon")
    UCustomizableSkeletalComponent* GetWeaponMutableComponent() const { return WeaponMutableComponent; }

protected:

    /**
     * Plain scene component used as the root.
     * The actor attaches to the character socket at this point.
     * Position WeaponMeshComponent relative to this in your Blueprint subclass
     * to visually align the weapon to the socket without manual offset values.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> WeaponRootComponent;

    /** The actual rendered skeletal mesh. Child of WeaponRootComponent. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

    /** Drives WeaponMeshComponent via Mutable. Set the CustomizableObject on this in your BP subclass. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCustomizableSkeletalComponent> WeaponMutableComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<URiftMutableWeaponComponent> MutableWeaponComponent;

private:

    UPROPERTY(ReplicatedUsing = OnRep_ItemInstance)
    TObjectPtr<URiftItemInstance> ItemInstance;

    UFUNCTION()
    void OnRep_ItemInstance();
};
