#include "ViewModels/URiftViewModelResolver.h"

#include "Blueprint/UserWidget.h"
#include "Subsystems/URiftInventoryUISubsystem.h"
#include "ViewModels/URiftViewModel.h"

UObject* URiftViewModelResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
    URiftInventoryUISubsystem* Subsystem = UserWidget->GetOwningLocalPlayer()->GetSubsystem<URiftInventoryUISubsystem>();
    check(Subsystem);
    return Subsystem->GetViewModel(ExpectedType, UserWidget, View);
}

void URiftViewModelResolver::DestroyInstance(const UObject* ViewModel, const UMVVMView* View) const
{
    const URiftViewModel* RiftViewModel = Cast<URiftViewModel>(ViewModel);
    if (IsValid(RiftViewModel))
    {
        URiftViewModel* MutableViewModel = const_cast<URiftViewModel*>(RiftViewModel);
        MutableViewModel->ShutdownViewModel();
    }
}

#if WITH_EDITOR
bool URiftViewModelResolver::DoesSupportViewModelClass(const UClass* Class) const
{
    if (Class->IsChildOf(URiftViewModel::StaticClass()))
    {
        return true;
    }

    return Super::DoesSupportViewModelClass(Class);
}
#endif
