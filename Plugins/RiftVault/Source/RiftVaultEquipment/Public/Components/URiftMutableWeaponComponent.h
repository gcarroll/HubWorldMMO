#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URiftMutableWeaponComponent.generated.h"

class URiftItemInstance;
class UCustomizableSkeletalComponent;
class UCustomizableObjectInstance;

/**
 * Drives the Mutable visual parameters for an ARiftWeaponActor's mesh.
 *
 * Lives on ARiftWeaponActor. Called by ARiftWeaponActor::SetupForItem after spawn.
 * Reads URiftFragment_Equippable::ActiveMutableParameters and applies them to the
 * weapon actor's own UCustomizableSkeletalComponent.
 *
 * Weapon actors are spawned server-side and replicated to clients as normal actors.
 * The UCustomizableObjectInstance on the weapon is configured server-side here;
 * Mutable replication (if enabled) handles client sync.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(RiftVault), meta=(BlueprintSpawnableComponent))
class RIFTVAULTEQUIPMENT_API URiftMutableWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    URiftMutableWeaponComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /**
     * Reads ActiveMutableParameters from the item's URiftFragment_Equippable and applies
     * them to the owning ARiftWeaponActor's UCustomizableSkeletalComponent.
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|Equipment|Mutable")
    void ApplyForItem(URiftItemInstance* Item);

private:

    UCustomizableObjectInstance* GetWeaponInstance() const;
};
