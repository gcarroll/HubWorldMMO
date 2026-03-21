#include "Components/URiftInventoryComponent.h"

#include "GameFramework/PlayerState.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "Interfaces/IRiftPersistenceInterface.h"
#include "Tags/RiftVaultTags.h"

URiftInventoryComponent::URiftInventoryComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsReplicatedByDefault(true);
    bWantsInitializeComponent = true;
}

void URiftInventoryComponent::InitializeComponent()
{
    Super::InitializeComponent();
}

void URiftInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        InitializeInventory();
    }
}

void URiftInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SaveDebounceHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void URiftInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool URiftInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
    bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

    for (URiftContainer* Container : Containers)
    {
        if (IsValid(Container))
        {
            bWroteSomething |= Channel->ReplicateSubobject(Container, *Bunch, *RepFlags);
        }
    }

    for (URiftItemInstance* Item : Items)
    {
        if (IsValid(Item))
        {
            bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
        }
    }

    return bWroteSomething;
}

bool URiftInventoryComponent::IsInitialized() const
{
    return bIsInitialized;
}

void URiftInventoryComponent::WaitForInitialized(FOnRiftInventoryInitializedDelegate&& Delegate)
{
    if (bIsInitialized)
    {
        UE_LOG(LogTemp, Log, TEXT("URiftInventoryComponent::WaitForInitialized — already initialized, calling immediately."));
        Delegate.Execute(true);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("URiftInventoryComponent::WaitForInitialized — not yet initialized, queuing delegate."));
    PendingInitializedDelegates.Add(MoveTemp(Delegate));
}

void URiftInventoryComponent::BroadcastInitialized(bool bSuccess)
{
    UE_LOG(LogTemp, Log, TEXT("URiftInventoryComponent::BroadcastInitialized — bSuccess: %s, pending delegates: %d"), bSuccess ? TEXT("true") : TEXT("false"), PendingInitializedDelegates.Num());
    OnInventoryInitialized.Broadcast(bSuccess);

    TArray<FOnRiftInventoryInitializedDelegate> LocalPending = MoveTemp(PendingInitializedDelegates);
    for (FOnRiftInventoryInitializedDelegate& Delegate : LocalPending)
    {
        if (Delegate.IsBound())
        {
            Delegate.Execute(bSuccess);
        }
    }
}

void URiftInventoryComponent::SetPersistenceObject(UObject* NewPersistenceObject)
{
    PersistenceObject = NewPersistenceObject;
}

void URiftInventoryComponent::InitializeInventory()
{
    UE_LOG(LogTemp, Log, TEXT("URiftInventoryComponent::InitializeInventory — creating %d default containers."), DefaultContainers.Num());

    for (URiftContainerDefinition* Definition : DefaultContainers)
    {
        if (IsValid(Definition))
        {
            URiftContainer* Container = CreateContainer(Definition);
            UE_LOG(LogTemp, Log, TEXT("URiftInventoryComponent — Created container with tag: %s, capacity: %d"),
                *Container->GetContainerTag().ToString(),
                Container->GetCapacity());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("URiftInventoryComponent — Null container definition in DefaultContainers array."));
        }
    }

    if (IsValid(PersistenceObject) && PersistenceObject->Implements<URiftPersistenceInterface>())
    {
        FString PlayerId;
        if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
        {
            PlayerId = PlayerState->GetPlayerName();
        }

        FOnLoadComplete LoadCallback;
        LoadCallback.BindDynamic(this, &URiftInventoryComponent::OnInventoryLoaded);
        IRiftPersistenceInterface::Execute_LoadInventory(PersistenceObject, PlayerId, LoadCallback);
    }
    else
    {
        bIsInitialized = true;
        BroadcastInitialized(true);
    }
}

void URiftInventoryComponent::OnInventoryLoaded(const bool bSuccess, const FRiftInventorySaveData& SaveData)
{
    if (!bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftInventoryComponent: Failed to load inventory."));
        bIsInitialized = true;
        BroadcastInitialized(false);
        return;
    }

    bIsInitialized = true;
    BroadcastInitialized(true);
}

void URiftInventoryComponent::SaveInventory()
{
    if (!IsValid(PersistenceObject) || !PersistenceObject->Implements<URiftPersistenceInterface>())
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SaveDebounceHandle);
        World->GetTimerManager().SetTimer(SaveDebounceHandle, [this]()
        {
            FRiftInventorySaveData SaveData;
            if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
            {
                SaveData.PlayerId = PlayerState->GetPlayerName();
            }

            FOnSaveComplete SaveCallback;
            IRiftPersistenceInterface::Execute_SaveInventory(PersistenceObject, SaveData, SaveCallback);

        }, 2.0f, false);
    }
}

FGuid URiftInventoryComponent::AddContainer(URiftContainerDefinition* ContainerDefinition)
{
    if (!IsValid(ContainerDefinition))
    {
        return FGuid();
    }

    URiftContainer* NewContainer = CreateContainer(ContainerDefinition);
    return IsValid(NewContainer) ? NewContainer->GetContainerId() : FGuid();
}

void URiftInventoryComponent::RemoveContainer(URiftContainer* Container)
{
    if (!IsValid(Container))
    {
        return;
    }

    for (URiftItemInstance* Item : Container->GetAllItems())
    {
        RemoveItem(Item);
    }

    Containers.Remove(Container);
    OnContainerRemoved.Broadcast(Container);
}

URiftContainer* URiftInventoryComponent::GetContainerById(const FGuid& ContainerId) const
{
    for (URiftContainer* Container : Containers)
    {
        if (IsValid(Container) && Container->GetContainerId() == ContainerId)
        {
            return Container;
        }
    }
    return nullptr;
}

URiftContainer* URiftInventoryComponent::GetContainerByTag(const FGameplayTag ContainerTag) const
{
    for (URiftContainer* Container : Containers)
    {
        if (IsValid(Container) && Container->GetContainerTag() == ContainerTag)
        {
            return Container;
        }
    }
    return nullptr;
}

TArray<URiftContainer*> URiftInventoryComponent::GetAllContainers() const
{
    TArray<URiftContainer*> Result;
    for (const TObjectPtr<URiftContainer>& Container : Containers)
    {
        if (IsValid(Container))
        {
            Result.Add(Container.Get());
        }
    }
    return Result;
}

int32 URiftInventoryComponent::CountContainers() const
{
    return Containers.Num();
}

FGuid URiftInventoryComponent::AddItem(URiftItemDefinition* Definition, const int32 Quantity)
{
    if (!IsValid(Definition))
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftInventoryComponent::AddItem — Invalid definition."));
        return FGuid();
    }

    URiftItemInstance* NewItem = CreateItemInstance(Definition);
    if (!IsValid(NewItem))
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftInventoryComponent::AddItem — Failed to create item instance for %s."), *Definition->GetInternalName().ToString());
        return FGuid();
    }

    // Log item tags to help debug container acceptance
    FGameplayTagContainer ItemTags;
    NewItem->GetOwnedGameplayTags(ItemTags);
    UE_LOG(LogTemp, Log, TEXT("URiftInventoryComponent::AddItem — Item %s has tags: %s"),
        *Definition->GetInternalName().ToString(),
        *ItemTags.ToString());

    // Log each container's acceptance result
    for (URiftContainer* Container : Containers)
    {
        if (IsValid(Container))
        {
            const bool bCanAccept = Container->CanAcceptItem(NewItem);
            UE_LOG(LogTemp, Log, TEXT("URiftInventoryComponent::AddItem — Container %s CanAccept: %s, IsFull: %s, Capacity: %d, Count: %d"),
                *Container->GetContainerTag().ToString(),
                bCanAccept ? TEXT("true") : TEXT("false"),
                Container->IsFull() ? TEXT("true") : TEXT("false"),
                Container->GetCapacity(),
                Container->CountItems());
        }
    }

    URiftContainer* TargetContainer = FindBestContainerForItem(NewItem);
    if (!IsValid(TargetContainer))
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftInventoryComponent::AddItem — No container found for item %s."), *Definition->GetInternalName().ToString());
        return FGuid();
    }

    const int32 SlotIndex = TargetContainer->AddItem(NewItem);
    if (SlotIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftInventoryComponent::AddItem — Container rejected item %s at slot add."), *Definition->GetInternalName().ToString());
        return FGuid();
    }

    Items.Add(NewItem);
    NewItem->InitializeFragmentStates();
    NewItem->ActivateFragments();

    OnItemAdded.Broadcast(NewItem, TargetContainer);
    SaveInventory();

    UE_LOG(LogTemp, Log, TEXT("URiftInventoryComponent::AddItem — Successfully added item %s to container %s at slot %d."),
        *Definition->GetInternalName().ToString(),
        *TargetContainer->GetContainerTag().ToString(),
        SlotIndex);

    return NewItem->GetItemId();
}

FGuid URiftInventoryComponent::AddItemToContainer(URiftItemDefinition* Definition, URiftContainer* TargetContainer, const int32 Quantity)
{
    if (!IsValid(Definition) || !IsValid(TargetContainer))
    {
        return FGuid();
    }

    URiftItemInstance* NewItem = CreateItemInstance(Definition);
    if (!IsValid(NewItem))
    {
        return FGuid();
    }

    if (!TargetContainer->CanAcceptItem(NewItem))
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftInventoryComponent::AddItemToContainer — Container rejected item %s."), *Definition->GetInternalName().ToString());
        return FGuid();
    }

    const int32 SlotIndex = TargetContainer->AddItem(NewItem);
    if (SlotIndex == INDEX_NONE)
    {
        return FGuid();
    }

    Items.Add(NewItem);
    NewItem->InitializeFragmentStates();
    NewItem->ActivateFragments();

    OnItemAdded.Broadcast(NewItem, TargetContainer);
    SaveInventory();

    return NewItem->GetItemId();
}

void URiftInventoryComponent::RemoveItem(URiftItemInstance* Item)
{
    if (!IsValid(Item))
    {
        return;
    }

    URiftContainer* Container = GetContainerForItem(Item);
    Item->DeactivateFragments();

    if (IsValid(Container))
    {
        Container->RemoveItem(Item);
        OnItemRemoved.Broadcast(Item, Container);
    }

    Items.Remove(Item);
    SaveInventory();
}

bool URiftInventoryComponent::MoveItemToSlot(URiftItemInstance* Item, const int32 TargetSlotIndex)
{
    if (!IsValid(Item))
    {
        return false;
    }

    URiftContainer* Container = GetContainerForItem(Item);
    if (!IsValid(Container))
    {
        return false;
    }

    const bool bMoved = Container->MoveItemToSlot(Item, TargetSlotIndex);
    if (bMoved)
    {
        OnItemMoved.Broadcast(Item, Container, Container);
    }

    return bMoved;
}

bool URiftInventoryComponent::MoveItemToContainer(URiftItemInstance* Item, URiftContainer* TargetContainer)
{
    if (!IsValid(Item) || !IsValid(TargetContainer))
    {
        return false;
    }

    URiftContainer* SourceContainer = GetContainerForItem(Item);
    if (!IsValid(SourceContainer))
    {
        return false;
    }

    if (!TargetContainer->CanAcceptItem(Item))
    {
        return false;
    }

    SourceContainer->RemoveItem(Item);
    const int32 SlotIndex = TargetContainer->AddItem(Item);

    if (SlotIndex == INDEX_NONE)
    {
        SourceContainer->AddItem(Item);
        return false;
    }

    OnItemMoved.Broadcast(Item, SourceContainer, TargetContainer);
    SaveInventory();

    return true;
}

URiftItemInstance* URiftInventoryComponent::GetItemById(const FGuid& ItemId) const
{
    for (URiftItemInstance* Item : Items)
    {
        if (IsValid(Item) && Item->GetItemId() == ItemId)
        {
            return Item;
        }
    }
    return nullptr;
}

URiftContainer* URiftInventoryComponent::GetContainerForItem(const URiftItemInstance* Item) const
{
    for (URiftContainer* Container : Containers)
    {
        if (IsValid(Container) && Container->HasItem(Item))
        {
            return Container;
        }
    }
    return nullptr;
}

bool URiftInventoryComponent::HasItem(const URiftItemDefinition* Definition) const
{
    return GetItemsByDefinition(Definition).Num() > 0;
}

TArray<URiftItemInstance*> URiftInventoryComponent::GetItemsByDefinition(const URiftItemDefinition* Definition) const
{
    TArray<URiftItemInstance*> Result;
    for (URiftItemInstance* Item : Items)
    {
        if (IsValid(Item) && Item->GetDefinition() == Definition)
        {
            Result.Add(Item);
        }
    }
    return Result;
}

TArray<URiftItemInstance*> URiftInventoryComponent::GetAllItems() const
{
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

int32 URiftInventoryComponent::CountItems() const
{
    return Items.Num();
}

URiftItemInstance* URiftInventoryComponent::CreateItemInstance(URiftItemDefinition* Definition)
{
    URiftItemInstance* NewItem = NewObject<URiftItemInstance>(this);
    NewItem->SetDefinition(Definition);
    NewItem->SetItemId(FGuid::NewGuid());
    return NewItem;
}

URiftContainer* URiftInventoryComponent::CreateContainer(URiftContainerDefinition* Definition)
{
    URiftContainer* NewContainer = NewObject<URiftContainer>(this);
    NewContainer->SetDefinition(Definition);
    NewContainer->SetContainerId(FGuid::NewGuid());
    Containers.Add(NewContainer);
    OnContainerAdded.Broadcast(NewContainer);
    return NewContainer;
}

URiftContainer* URiftInventoryComponent::FindBestContainerForItem(const URiftItemInstance* Item) const
{
    for (URiftContainer* Container : Containers)
    {
        if (IsValid(Container) && Container->CanAcceptItem(Item))
        {
            return Container;
        }
    }
    return nullptr;
}
