#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * RiftVaultEquipment module — handles equipping items to pawn slots.
 *
 * URiftEquipmentComponent lives on APawn (never ACharacter — Mover-compatible).
 * The inventory stays on APlayerState; the equipment component bridges the two.
 *
 * Provides:
 *   URiftEquipmentComponent        — slot state, ability granting, weapon actor spawning
 *   URiftMutableEquipmentComponent — Mutable visual parameters for body equipment
 *   URiftMutableWeaponComponent    — Mutable visual parameters for weapon actors
 *   ARiftWeaponActor               — replicated weapon actor attached to the pawn
 *   URiftAbility_Equip             — GAS ability wrapper for EquipItem
 *   URiftAbility_Unequip           — GAS ability wrapper for UnequipItem
 */
class FRiftVaultEquipmentModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
