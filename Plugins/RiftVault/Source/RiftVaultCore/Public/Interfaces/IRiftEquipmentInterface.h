#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/Interface.h"
#include "IRiftEquipmentInterface.generated.h"

// UInterface boilerplate required by UHT — not used directly.
UINTERFACE(MinimalAPI, Blueprintable)
class URiftEquipmentInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Implemented by any pawn that owns a URiftEquipmentComponent.
 *
 * WHY UActorComponent* INSTEAD OF URiftEquipmentComponent*?
 * ---------------------------------------------------------
 * URiftEquipmentComponent lives in the RiftVaultEquipment module.
 * IRiftEquipmentInterface lives in RiftVaultCore. Including RiftVaultEquipment
 * headers here would create a circular module dependency since RiftVaultEquipment
 * depends on RiftVaultCore. Returning UActorComponent* breaks the cycle.
 *
 * Callers cast the result at their own call site:
 *
 *   UActorComponent* Comp = Pawn->Execute_GetEquipmentComponent(Pawn);
 *   URiftEquipmentComponent* Equipment = Cast<URiftEquipmentComponent>(Comp);
 *
 * MOVER COMPATIBILITY NOTE
 * -------------------------
 * All equipment logic operates at the APawn level — not ACharacter — to remain
 * compatible with Unreal's Mover movement system. Do not introduce ACharacter or
 * USkeletalMeshComponent assumptions into this interface or its implementations.
 *
 * Blueprintable is set so that Blueprint pawns can implement the interface
 * and override GetEquipmentComponent in a Blueprint graph without a C++ base class.
 */
class RIFTVAULTCORE_API IRiftEquipmentInterface
{
    GENERATED_BODY()

public:

    /**
     * Returns the equipment component owned by this pawn.
     *
     * The default C++ implementation returns nullptr. Override in the concrete
     * pawn class to return the actual URiftEquipmentComponent subobject.
     *
     * Callers should cast the result to URiftEquipmentComponent* before use:
     *   URiftEquipmentComponent* Eq = Cast<URiftEquipmentComponent>(GetEquipmentComponent());
     *
     * @return The pawn's URiftEquipmentComponent, or nullptr if not implemented.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Interfaces|Equipment")
    UActorComponent* GetEquipmentComponent() const;
    virtual UActorComponent* GetEquipmentComponent_Implementation() const;
};
