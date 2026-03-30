#include "Components/URiftPickupComponent.h"
#include "Actors/ARiftPickup.h"
#include "Components/PrimitiveComponent.h"
#include "Components/URiftInventoryComponent.h"
#include "Data/URiftItemDefinition.h"
#include "GameFramework/Fragments/URiftFragment_Stack.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/IRiftInventoryInterface.h"

URiftPickupComponent::URiftPickupComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URiftPickupComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    // Bind to the first overlap-generating PrimitiveComponent on the owner.
    TArray<UPrimitiveComponent*> PrimComps;
    GetOwner()->GetComponents<UPrimitiveComponent>(PrimComps);

    for (UPrimitiveComponent* PrimComp : PrimComps)
    {
        if (PrimComp->GetGenerateOverlapEvents())
        {
            PrimComp->OnComponentBeginOverlap.AddDynamic(this, &URiftPickupComponent::OnOverlapBegin);
            break;
        }
    }
}

void URiftPickupComponent::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !GetOwner()->HasAuthority())
    {
        return;
    }

    TryCollect(OtherActor);
}

bool URiftPickupComponent::TryCollect_Implementation(AActor* Collector)
{
    // Find the collector's inventory.
    URiftInventoryComponent* CollectorInventory = nullptr;

    auto TryGetInventory = [&](AActor* Actor) -> URiftInventoryComponent*
    {
        if (IsValid(Actor) && Actor->Implements<URiftInventoryInterface>())
        {
            return Cast<URiftInventoryComponent>(
                IRiftInventoryInterface::Execute_GetInventoryComponent(Actor));
        }
        return nullptr;
    };

    CollectorInventory = TryGetInventory(Collector);

    if (!CollectorInventory)
    {
        CollectorInventory = Collector->FindComponentByClass<URiftInventoryComponent>();
    }

    if (!CollectorInventory)
    {
        if (const APawn* Pawn = Cast<APawn>(Collector))
        {
            CollectorInventory = TryGetInventory(Pawn->GetPlayerState());

            if (!CollectorInventory)
            {
                if (APlayerState* PS = Pawn->GetPlayerState())
                {
                    CollectorInventory = PS->FindComponentByClass<URiftInventoryComponent>();
                }
            }
        }
    }

    if (!CollectorInventory)
    {
        return false;
    }

    // --- Multi-item path: transfer everything from the owner's DropInventory ---
    if (ARiftPickup* Pickup = Cast<ARiftPickup>(GetOwner()))
    {
        if (URiftInventoryComponent* DropInventory = Pickup->GetDropInventory())
        {
            TArray<URiftItemInstance*> DroppedItems = DropInventory->GetAllItems();
            if (DroppedItems.Num() > 0)
            {
                bool bAnyAdded = false;
                for (URiftItemInstance* Item : DroppedItems)
                {
                    if (!IsValid(Item)) continue;

                    URiftItemDefinition* Def = Item->GetDefinition();
                    if (!Def) continue;

                    int32 Qty = 1;
                    if (URiftFragment_Stack* StackFrag = Item->FindFragment<URiftFragment_Stack>())
                    {
                        Qty = StackFrag->GetCurrentQuantity(Item);
                    }

                    if (CollectorInventory->AddItem(Def, Qty).IsValid())
                    {
                        bAnyAdded = true;
                    }
                }

                if (!bAnyAdded)
                {
                    return false;
                }

                OnPickupCollected.Broadcast(Collector);

                if (bDestroyOnCollect)
                {
                    GetOwner()->Destroy();
                }

                return true;
            }
        }
    }

    // --- Single-item path: legacy ItemDefinition + Quantity ---
    if (!ItemDefinition || Quantity <= 0)
    {
        return false;
    }

    CollectorInventory->AddItem(ItemDefinition, Quantity);
    OnPickupCollected.Broadcast(Collector);

    if (bDestroyOnCollect)
    {
        GetOwner()->Destroy();
    }

    return true;
}
