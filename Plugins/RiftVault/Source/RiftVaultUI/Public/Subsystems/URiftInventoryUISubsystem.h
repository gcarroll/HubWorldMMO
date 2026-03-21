#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "URiftInventoryUISubsystem.generated.h"

class URiftViewModel;
class UUserWidget;
class UMVVMView;

UCLASS()
class RIFTVAULTUI_API URiftInventoryUISubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:

    virtual void Deinitialize() override;

    /**
     * Gets or creates a ViewModel of the given type.
     * Called by URiftViewModelResolver automatically when using MVVM bindings.
     */
    URiftViewModel* GetViewModel(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View);

    /**
     * Blueprint callable version — gets or creates a ViewModel by class.
     * Use this to manually retrieve a ViewModel outside of widget MVVM bindings.
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|UI|Subsystem", meta = (DeterminesOutputType = "ViewModelClass"))
    URiftViewModel* GetViewModelByClass(TSubclassOf<URiftViewModel> ViewModelClass);

private:

    UPROPERTY(Transient)
    TMap<TObjectPtr<const UClass>, TObjectPtr<URiftViewModel>> SingletonMap;
};
