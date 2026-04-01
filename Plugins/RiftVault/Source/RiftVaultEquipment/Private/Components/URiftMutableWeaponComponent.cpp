#include "Components/URiftMutableWeaponComponent.h"
#include "Actors/ARiftWeaponActor.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "GameFramework/Fragments/URiftFragment_Equippable.h"
#include "Types/FRiftMutableParameter.h"
#include "MuCO/CustomizableSkeletalComponent.h"
#include "MuCO/CustomizableObjectInstance.h"

URiftMutableWeaponComponent::URiftMutableWeaponComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URiftMutableWeaponComponent::ApplyForItem(URiftItemInstance* Item)
{
    if (!Item)
    {
        return;
    }

    const URiftFragment_Equippable* Fragment = Item->FindFragment<URiftFragment_Equippable>();
    if (!Fragment || Fragment->GetActiveMutableParameters().IsEmpty())
    {
        return;
    }

    UCustomizableObjectInstance* Instance = GetWeaponInstance();
    if (!Instance)
    {
        return;
    }

    // NOTE: Verify exact Mutable API method names for your UE5 version.
    for (const FRiftMutableParameter& Param : Fragment->GetActiveMutableParameters())
    {
        switch (Param.ParameterType)
        {
        case ERiftMutableParameterType::Int32:
            Instance->SetIntParameterSelectedOption(Param.ParameterName.ToString(), Param.IntOptionName);
            break;
        case ERiftMutableParameterType::Float:
            Instance->SetFloatParameterSelectedOption(Param.ParameterName.ToString(), Param.FloatValue);
            break;
        case ERiftMutableParameterType::Color:
            Instance->SetColorParameterSelectedOption(Param.ParameterName.ToString(), Param.ColorValue);
            break;
        }
    }

    // NOTE: In older Mutable versions this may be UpdateSkeletalMesh() on the component instead.
    Instance->UpdateSkeletalMeshAsync();
}

UCustomizableObjectInstance* URiftMutableWeaponComponent::GetWeaponInstance() const
{
    if (ARiftWeaponActor* WeaponActor = GetOwner<ARiftWeaponActor>())
    {
        if (UCustomizableSkeletalComponent* MutableComp = WeaponActor->GetWeaponMutableComponent())
        {
            return MutableComp->GetCustomizableObjectInstance();
        }
    }
    return nullptr;
}
