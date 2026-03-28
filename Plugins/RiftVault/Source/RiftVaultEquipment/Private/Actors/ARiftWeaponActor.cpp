#include "Actors/ARiftWeaponActor.h"
#include "Components/URiftMutableWeaponComponent.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "MuCO/CustomizableSkeletalComponent.h"
#include "Net/UnrealNetwork.h"

ARiftWeaponActor::ARiftWeaponActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bReplicates = true;
    bNetLoadOnClient = false; // Spawned at runtime by URiftEquipmentComponent, never level-placed.
    PrimaryActorTick.bCanEverTick = false;

    // Plain scene component as root — this is the socket attachment point.
    // In a Blueprint subclass, position WeaponMeshComponent relative to this
    // to visually align the weapon without needing manual offset values.
    WeaponRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
    SetRootComponent(WeaponRootComponent);

    // Actual mesh renderer — child of root so its relative transform acts as the visual offset.
    WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    WeaponMeshComponent->SetupAttachment(WeaponRootComponent);

    // Mutable driver — child of the mesh component.
    WeaponMutableComponent = CreateDefaultSubobject<UCustomizableSkeletalComponent>(TEXT("WeaponMutable"));
    WeaponMutableComponent->SetupAttachment(WeaponMeshComponent);

    MutableWeaponComponent = CreateDefaultSubobject<URiftMutableWeaponComponent>(TEXT("MutableWeapon"));
}

void ARiftWeaponActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARiftWeaponActor, ItemInstance);
}

void ARiftWeaponActor::OnRep_ItemInstance()
{
    if (MutableWeaponComponent && ItemInstance)
    {
        MutableWeaponComponent->ApplyForItem(ItemInstance);
    }
}

void ARiftWeaponActor::SetupForItem(URiftItemInstance* InItem)
{
    ItemInstance = InItem;

    if (MutableWeaponComponent && InItem)
    {
        MutableWeaponComponent->ApplyForItem(InItem);
    }
}
