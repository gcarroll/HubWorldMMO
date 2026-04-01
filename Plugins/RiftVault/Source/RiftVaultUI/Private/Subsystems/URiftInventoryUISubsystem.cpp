#include "Subsystems/URiftInventoryUISubsystem.h"

#include "ViewModels/URiftViewModel.h"

void URiftInventoryUISubsystem::Deinitialize()
{
    for (auto& Pair : SingletonMap)
    {
        if (IsValid(Pair.Value))
        {
            Pair.Value->ShutdownViewModel();
        }
    }

    SingletonMap.Empty();
    Super::Deinitialize();
}

URiftViewModel* URiftInventoryUISubsystem::GetViewModel(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View)
{
    if (!IsValid(ExpectedType))
    {
        return nullptr;
    }

    if (SingletonMap.Contains(ExpectedType))
    {
        return SingletonMap[ExpectedType];
    }

    URiftViewModel* ViewModel = NewObject<URiftViewModel>(GetOuter(), ExpectedType);
    if (!IsValid(ViewModel))
    {
        return nullptr;
    }

    ViewModel->InitializeViewModel(UserWidget);

    if (ViewModel->IsSingleton())
    {
        SingletonMap.Add(ExpectedType, ViewModel);
    }

    return ViewModel;
}

URiftViewModel* URiftInventoryUISubsystem::GetViewModelByClass(TSubclassOf<URiftViewModel> ViewModelClass)
{
    return GetViewModel(ViewModelClass, nullptr, nullptr);
}
