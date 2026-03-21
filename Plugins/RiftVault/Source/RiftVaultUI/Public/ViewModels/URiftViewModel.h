#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "URiftViewModel.generated.h"

/**
 * Base class for all RiftVault ViewModels.
 *
 * Three rules all ViewModels must follow:
 * 1. ViewModels are owned by URiftInventoryUISubsystem, not by widgets.
 * 2. All updates are event-driven — no polling.
 * 3. Cross-object references use TWeakObjectPtr.
 */
UCLASS(Abstract, Blueprintable)
class RIFTVAULTUI_API URiftViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:

    URiftViewModel();

    /**
     * Returns true if this ViewModel should only ever have one instance.
     * Singleton ViewModels are stored and reused by the subsystem.
     */
    bool IsSingleton() const { return bIsSingleton; }

    /**
     * Called by the subsystem when this ViewModel is first created.
     * Bind to inventory component delegates here.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModel")
    void InitializeViewModel(const UUserWidget* UserWidget);
    virtual void InitializeViewModel_Implementation(const UUserWidget* UserWidget);

    /**
     * Called by the subsystem before this ViewModel is destroyed.
     * Unbind delegates and release weak references here.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModel")
    void ShutdownViewModel();
    virtual void ShutdownViewModel_Implementation();

protected:

    /**
     * Set to true in subclass constructors for ViewModels that should be
     * shared across all widgets of the same type (e.g. a single player inventory ViewModel).
     */
    bool bIsSingleton;
};
