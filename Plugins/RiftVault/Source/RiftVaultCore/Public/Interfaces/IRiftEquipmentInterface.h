#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/Interface.h"
#include "IRiftEquipmentInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class URiftEquipmentInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Allows any actor to advertise that it owns a URiftEquipmentComponent.
 *
 * Returns UActorComponent* rather than URiftEquipmentComponent* because
 * URiftEquipmentComponent lives in RiftVaultEquipment — referencing it here
 * in Core would create a cross-module UHT violation. Callers cast the result
 * to URiftEquipmentComponent* after retrieving it.
 *
 * NOTE: All equipment operates at the APawn level for Mover compatibility.
 * Do not add ACharacter or USkeletalMeshComponent assumptions here.
 */
class RIFTVAULTCORE_API IRiftEquipmentInterface
{
    GENERATED_BODY()

public:

    /**
     * Returns the equipment component owned by this pawn.
     * Cast the result to URiftEquipmentComponent* at the call site.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Interfaces|Equipment")
    UActorComponent* GetEquipmentComponent() const;
    virtual UActorComponent* GetEquipmentComponent_Implementation() const;
};
