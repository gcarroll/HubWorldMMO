#include "Actors/ARiftPickup.h"
#include "Components/URiftPickupComponent.h"
#include "Components/URiftInventoryComponent.h"
#include "Data/URiftGroundContainerDefinition.h"
#include "Data/URiftItemDefinition.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

ARiftPickup::ARiftPickup(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = false;

    OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
    OverlapSphere->SetSphereRadius(64.f);
    OverlapSphere->SetCollisionProfileName(TEXT("Trigger"));
    OverlapSphere->SetGenerateOverlapEvents(true);
    SetRootComponent(OverlapSphere);

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComponent->SetupAttachment(OverlapSphere);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PickupComponent = CreateDefaultSubobject<URiftPickupComponent>(TEXT("Pickup"));

    DropInventory = CreateDefaultSubobject<URiftInventoryComponent>(TEXT("DropInventory"));
    DropInventory->SetIsReplicated(true);
}

void ARiftPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARiftPickup, ReplicatedDropMesh);
}

void ARiftPickup::BeginPlay()
{
    Super::BeginPlay();

    // Force replication regardless of Blueprint class defaults.
    SetReplicates(true);

    if (HasAuthority())
    {
        // Create the ground container so items dropped here have a home.
        URiftGroundContainerDefinition* GroundDef = NewObject<URiftGroundContainerDefinition>(this);
        DropInventory->AddContainer(GroundDef);
    }
}

void ARiftPickup::AddDroppedItem(URiftItemDefinition* Definition, int32 Quantity)
{
    if (!Definition || Quantity <= 0 || !HasAuthority())
    {
        return;
    }

    DropInventory->AddItem(Definition, Quantity);
}

void ARiftPickup::OnRep_DropMesh()
{
    if (MeshComponent)
    {
        MeshComponent->SetStaticMesh(ReplicatedDropMesh);
    }
}

void ARiftPickup::SetDropMesh(UStaticMesh* Mesh)
{
    ReplicatedDropMesh = Mesh;
    if (MeshComponent)
    {
        MeshComponent->SetStaticMesh(Mesh);
    }
}
