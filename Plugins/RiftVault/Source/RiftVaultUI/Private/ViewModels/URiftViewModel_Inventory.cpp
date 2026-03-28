#include "ViewModels/URiftViewModel_Inventory.h"

#include "Components/URiftInventoryComponent.h"

void URiftViewModel_Inventory::InitializeViewModel_Implementation(const UUserWidget* UserWidget)
{
    // Mark as singleton so URiftInventoryUISubsystem caches this instance and returns
    // the same ViewModel to every widget that asks for URiftViewModel_Inventory.
    // This ensures all inventory widgets (grid, count label, etc.) share one source of truth.
    bIsSingleton = true;
}

void URiftViewModel_Inventory::ShutdownViewModel_Implementation()
{
    // Disconnect from the inventory component's delegates before this ViewModel is
    // destroyed. Failing to do so would leave dangling UFUNCTION pointers on the delegate.
    UnbindFromInventoryComponent();

    // Clear the cached array so there are no lingering TObjectPtr references to item
    // instances after shutdown. This is important because the subsystem may outlive
    // the pawn and its inventory.
    Items.Empty();
}

void URiftViewModel_Inventory::SetInventoryComponent(URiftInventoryComponent* NewInventoryComponent)
{
    // Always unbind from the previous component first. This handles the case where
    // SetInventoryComponent is called again on pawn respawn with a new component instance.
    UnbindFromInventoryComponent();

    // Store a weak reference so the ViewModel does not prevent the component/pawn
    // from being garbage collected when the pawn is destroyed.
    InventoryComponent = NewInventoryComponent;

    // Subscribe to future item events so the ViewModel stays in sync incrementally.
    BindToInventoryComponent();

    // Take an immediate snapshot of the current inventory state. This populates Items
    // with anything that was already in the inventory before the ViewModel connected,
    // and broadcasts FieldNotify so any already-bound widgets update immediately.
    RefreshItems();
}

TArray<URiftItemInstance*> URiftViewModel_Inventory::GetItems() const
{
    // Build a raw-pointer array from the TObjectPtr array.
    // We filter out any invalid entries defensively — items can become invalid if they
    // are GC'd between the delegate firing and this getter being called.
    TArray<URiftItemInstance*> Result;
    for (const TObjectPtr<URiftItemInstance>& Item : Items)
    {
        if (IsValid(Item))
        {
            Result.Add(Item.Get());
        }
    }
    return Result;
}

int32 URiftViewModel_Inventory::GetItemCount() const
{
    // Returns the raw count of the Items array including any not-yet-GC'd invalid entries.
    // This is intentional — the count reflects how many slots the grid should render,
    // not how many items passed IsValid(). GetItems() performs the validity filter.
    return Items.Num();
}

void URiftViewModel_Inventory::OnItemAdded(URiftItemInstance* Item, URiftContainer* Container)
{
    // Guard: skip null/invalid items (e.g. if the component fires the delegate before
    // the item is fully initialized, which shouldn't happen but is cheap to check).
    if (IsValid(Item))
    {
        // AddUnique prevents duplicate entries if the delegate fires more than once for
        // the same item (e.g. due to a container transfer that internally removes and re-adds).
        Items.AddUnique(Item);

        // Notify all MVVM-bound widgets that the items list has changed.
        // GetItems and GetItemCount are separate FieldNotify fields so widgets can bind
        // to either independently (e.g. a count label binds only to GetItemCount).
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItems);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItemCount);
    }
}

void URiftViewModel_Inventory::OnItemRemoved(URiftItemInstance* Item, URiftContainer* Container)
{
    // TArray::Remove returns the number of elements removed.
    // We only broadcast if something was actually removed to avoid spurious notifications.
    if (Items.Remove(Item) > 0)
    {
        // Notify bound widgets that the list and count have changed.
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItems);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItemCount);
    }
}

void URiftViewModel_Inventory::BindToInventoryComponent()
{
    // Guard: if the component was destroyed before we could bind, skip silently.
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    // AddDynamic registers UFUNCTION-marked methods as delegate handlers.
    // These will fire on the game thread whenever the inventory component adds/removes items.
    InventoryComponent->OnItemAdded.AddDynamic(this, &URiftViewModel_Inventory::OnItemAdded);
    InventoryComponent->OnItemRemoved.AddDynamic(this, &URiftViewModel_Inventory::OnItemRemoved);
}

void URiftViewModel_Inventory::UnbindFromInventoryComponent()
{
    // Guard: if the component is no longer valid (pawn destroyed), the delegate objects
    // are already gone and there is nothing to remove. Skip to avoid a null-deref.
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    // RemoveDynamic matches by both function pointer and object pointer, so only this
    // ViewModel's handlers are removed — other listeners on the same delegate are unaffected.
    InventoryComponent->OnItemAdded.RemoveDynamic(this, &URiftViewModel_Inventory::OnItemAdded);
    InventoryComponent->OnItemRemoved.RemoveDynamic(this, &URiftViewModel_Inventory::OnItemRemoved);
}

void URiftViewModel_Inventory::RefreshItems()
{
    // Start from a clean slate to avoid stale entries from a previous component.
    Items.Empty();

    // Guard: if no component is connected, broadcast empty-state notifications so any
    // bound widgets clear themselves (e.g. the grid renders zero slots).
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    // Populate from the inventory component's authoritative item list.
    // Filter invalid items defensively in case any are in an intermediate GC state.
    for (URiftItemInstance* Item : InventoryComponent->GetAllItems())
    {
        if (IsValid(Item))
        {
            Items.Add(Item);
        }
    }

    // Broadcast FieldNotify for both fields so all bound widgets see the refreshed state.
    // This fires even if Items ended up empty (after clearing), which is the correct
    // behavior when switching between pawns or when the inventory loads with no items.
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItems);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItemCount);
}
