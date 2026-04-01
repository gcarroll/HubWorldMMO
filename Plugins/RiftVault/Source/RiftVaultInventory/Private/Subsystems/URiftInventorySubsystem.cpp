#include "Subsystems/URiftInventorySubsystem.h"

#include "Components/URiftInventoryComponent.h"

void URiftInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URiftInventorySubsystem::Deinitialize()
{
    KnownInventories.Empty();
    Super::Deinitialize();
}

void URiftInventorySubsystem::RegisterInventory(URiftInventoryComponent* InventoryComponent)
{
    if (!IsValid(InventoryComponent))
    {
        return;
    }

    const AActor* Owner = InventoryComponent->GetOwner();
    if (IsValid(Owner))
    {
        KnownInventories.Add(Owner, InventoryComponent);
    }
}

void URiftInventorySubsystem::UnregisterInventory(URiftInventoryComponent* InventoryComponent)
{
    if (!IsValid(InventoryComponent))
    {
        return;
    }

    const AActor* Owner = InventoryComponent->GetOwner();
    if (IsValid(Owner))
    {
        KnownInventories.Remove(Owner);
    }
}

URiftInventoryComponent* URiftInventorySubsystem::GetInventoryForOwner(const AActor* Owner) const
{
    if (!IsValid(Owner))
    {
        return nullptr;
    }

    const TObjectPtr<URiftInventoryComponent>* Found = KnownInventories.Find(Owner);
    return Found ? Found->Get() : nullptr;
}

TArray<URiftInventoryComponent*> URiftInventorySubsystem::GetAllInventories() const
{
    TArray<URiftInventoryComponent*> Result;
    for (const auto& Pair : KnownInventories)
    {
        if (IsValid(Pair.Value))
        {
            Result.Add(Pair.Value.Get());
        }
    }
    return Result;
}
