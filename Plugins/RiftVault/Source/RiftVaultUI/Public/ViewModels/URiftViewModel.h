#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "URiftViewModel.generated.h"

/**
 * Base class for all RiftVault ViewModels.
 *
 * Derives from UMVVMViewModelBase, which provides the FieldNotify notification
 * infrastructure used by UE's MVVM plugin. All data bindings in Blueprint widget
 * graphs talk to properties and functions declared on this class hierarchy.
 *
 * Three rules all ViewModels must follow:
 * 1. ViewModels are owned by URiftInventoryUISubsystem, not by widgets.
 *    Widgets request their ViewModel from the subsystem through URiftViewModelResolver.
 * 2. All updates are event-driven — no polling.
 *    ViewModels bind to inventory/equipment component delegates in InitializeViewModel
 *    and unbind in ShutdownViewModel.
 * 3. Cross-object references use TWeakObjectPtr.
 *    This prevents hard GC roots from ViewModel → gameplay objects, keeping the
 *    UI layer safely decoupled from the gameplay layer.
 */
UCLASS(Abstract, Blueprintable)
class RIFTVAULTUI_API URiftViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:

    URiftViewModel();

    /**
     * Returns true if this ViewModel should only ever have one instance per local player.
     *
     * Singleton ViewModels are cached in URiftInventoryUISubsystem::SingletonMap and
     * reused whenever multiple widgets request the same ViewModel class. Non-singleton
     * ViewModels get a fresh instance for each request.
     *
     * Subclasses set bIsSingleton = true inside InitializeViewModel_Implementation
     * (or in the constructor) to opt into singleton behavior.
     */
    bool IsSingleton() const { return bIsSingleton; }

    /**
     * Called by URiftInventoryUISubsystem::GetViewModel immediately after this ViewModel
     * is created. Subclasses should:
     *   - Set bIsSingleton if appropriate
     *   - Locate the relevant gameplay component (inventory, equipment, etc.)
     *   - Bind to that component's delegates so the ViewModel stays in sync
     *
     * @param UserWidget  The widget that triggered the ViewModel creation. May be null
     *                    when called from GetViewModelByClass (non-MVVM path). Use it
     *                    to walk up to PlayerController/PlayerState to find components.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModel")
    void InitializeViewModel(const UUserWidget* UserWidget);
    virtual void InitializeViewModel_Implementation(const UUserWidget* UserWidget);

    /**
     * Called by URiftViewModelResolver::DestroyInstance before this ViewModel is destroyed.
     * Subclasses should:
     *   - Unbind all delegates they registered in InitializeViewModel
     *   - Reset or empty any cached data arrays
     *   - Release TWeakObjectPtr references (they will expire naturally but explicit
     *     resets make the intent clear and aid debugging)
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModel")
    void ShutdownViewModel();
    virtual void ShutdownViewModel_Implementation();

protected:

    /**
     * Controls whether this ViewModel instance is shared across all widgets of the same type.
     *
     * Default is false (set in constructor). Subclasses that represent per-player shared
     * state (e.g. the player's inventory, the player's equipment) should set this to true
     * inside InitializeViewModel_Implementation so URiftInventoryUISubsystem caches them.
     *
     * Per-item ViewModels (e.g. URiftViewModel_ItemDisplay, one per slot widget) must
     * leave this false so each slot widget gets its own independent ViewModel instance.
     */
    bool bIsSingleton;
};
