#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "URiftViewModelResolver.generated.h"

/**
 * Context resolver for all RiftVault ViewModels.
 *
 * Add this resolver to any widget's MVVM settings to have ViewModels
 * automatically created and managed by URiftInventoryUISubsystem.
 * No Blueprint code needed to wire up ViewModels — just set the
 * resolver and the ViewModel class in the widget's MVVM bindings.
 */
UCLASS(DisplayName = "RiftVault Resolver")
class RIFTVAULTUI_API URiftViewModelResolver : public UMVVMViewModelContextResolver
{
    GENERATED_BODY()

public:

    // -- Begin UMVVMViewModelContextResolver interface
    virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
    virtual void DestroyInstance(const UObject* ViewModel, const UMVVMView* View) const override;
    // -- End UMVVMViewModelContextResolver interface

#if WITH_EDITOR
    virtual bool DoesSupportViewModelClass(const UClass* Class) const override;
#endif
};
