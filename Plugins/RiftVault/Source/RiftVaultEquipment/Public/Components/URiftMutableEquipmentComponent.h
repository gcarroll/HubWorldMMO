#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Types/FRiftMutableParameter.h"
#include "URiftMutableEquipmentComponent.generated.h"

class UCustomizableSkeletalComponent;
class UCustomizableObjectInstance;
class URiftItemInstance;
class URiftItemDefinition;

/**
 * Applies Mutable visual parameters to the pawn's body mesh when items are equipped or unequipped.
 *
 * Add alongside URiftEquipmentComponent on the same pawn. Binds to OnItemEquipped
 * and OnItemUnequipped in BeginPlay and reacts to visual changes only.
 *
 * To identify the body mesh: add the string "Rift.Component.BodyMesh" to the
 * UCustomizableSkeletalComponent's ComponentTags array in the pawn Blueprint.
 * This component finds it at startup using Tag_Rift_Component_BodyMesh.GetTagName().
 *
 * Body armor / helmet equipment flows through here.
 * Weapon equipment flows through URiftMutableWeaponComponent on ARiftWeaponActor.
 *
 * NOTE: ApplyMutableParameters and ResetMutableParameters are server-only.
 * Mutable's built-in instance replication handles client-side visual sync.
 * Verify that UCustomizableObjectInstance replication is enabled in your Mutable project.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(RiftVault), meta=(BlueprintSpawnableComponent))
class RIFTVAULTEQUIPMENT_API URiftMutableEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    URiftMutableEquipmentComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;

    /** Returns the body mesh component found by the Rift.Component.BodyMesh tag. May be null. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment|Mutable")
    UCustomizableSkeletalComponent* GetBodyMeshComponent() const { return BodyMeshComponent.Get(); }

    /**
     * Applies a set of Mutable parameters to the pawn's body mesh instance and requests an update.
     * Server-only — Mutable instance replication syncs the result to clients.
     *
     * NOTE: Verify Mutable API method names for your UE5 version before shipping.
     * See ApplyMutableParameters implementation for details.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Equipment|Mutable")
    void ApplyMutableParameters(const TArray<FRiftMutableParameter>& Parameters);

    /**
     * Resets a set of Mutable parameters to default values (0 / empty / white) and requests an update.
     * Server-only. Called on unequip to restore the body mesh to its pre-equip state.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Equipment|Mutable")
    void ResetMutableParameters(const TArray<FRiftMutableParameter>& Parameters);

private:

    UPROPERTY()
    TWeakObjectPtr<UCustomizableSkeletalComponent> BodyMeshComponent;

    UFUNCTION()
    void OnItemEquipped(URiftItemInstance* Item, FGameplayTag SlotTag, URiftItemDefinition* ItemDefinition);

    UFUNCTION()
    void OnItemUnequipped(URiftItemInstance* Item, FGameplayTag SlotTag, URiftItemDefinition* ItemDefinition);

    UCustomizableSkeletalComponent* FindBodyMeshComp() const;
    UCustomizableObjectInstance*    GetObjectInstance() const;

    // Deferred single-flush: coalesces multiple parameter changes in one tick into
    // one UpdateSkeletalMeshAsync call so swap (reset + apply) doesn't generate two
    // competing mesh requests where the reset request shadows the apply request.
    void ScheduleMutableUpdate();
    void FlushMutableUpdate();

    FTimerHandle MutableUpdateTimer;
};
