#include "Widgets/URiftInventoryGridWidget.h"

#include "Components/PanelWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/URiftInventoryComponent.h"
#include "Data/URiftContainerDefinition.h"
#include "GameFramework/Containers/URiftContainer.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "GameFramework/PlayerState.h"
#include "Widgets/URiftItemSlotWidget.h"

URiftInventoryGridWidget::URiftInventoryGridWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URiftInventoryGridWidget::NativeConstruct()
{
    Super::NativeConstruct();

    InventoryComponent = FindInventoryComponent();
    if (InventoryComponent.IsValid())
    {
        BindToInventory();
        InventoryComponent->WaitForInitialized(FOnRiftInventoryInitializedDelegate::CreateUObject(this, &URiftInventoryGridWidget::OnInventoryInitialized));
    }
}

void URiftInventoryGridWidget::NativeDestruct()
{
    UnbindFromInventory();
    SlotWidgets.Empty();
    Super::NativeDestruct();
}

void URiftInventoryGridWidget::OnInventoryInitialized(bool bSuccess)
{
    UE_LOG(LogTemp, Log, TEXT("URiftInventoryGridWidget::OnInventoryInitialized — bSuccess: %s"), bSuccess ? TEXT("true") : TEXT("false"));

    if (!bSuccess || !InventoryComponent.IsValid())
    {
        return;
    }

    BuildSlots();

    UE_LOG(LogTemp, Log, TEXT("URiftInventoryGridWidget::OnInventoryInitialized — SlotWidgets built: %d"), SlotWidgets.Num());

    if (TrackedContainer.IsValid())
    {
        TArray<URiftItemInstance*> Items = TrackedContainer->GetAllItems();
        UE_LOG(LogTemp, Log, TEXT("URiftInventoryGridWidget::OnInventoryInitialized — Items in container: %d"), Items.Num());

        for (URiftItemInstance* Item : Items)
        {
            const int32 SlotIndex = FindSlotIndexOfItem(Item);
            if (SlotWidgets.IsValidIndex(SlotIndex) && IsValid(SlotWidgets[SlotIndex]))
            {
                SlotWidgets[SlotIndex]->SetItem(Item, TrackedContainer.Get(), SlotIndex);
            }
        }
    }
}

void URiftInventoryGridWidget::OnItemAdded(URiftItemInstance* Item, URiftContainer* Container)
{
    UE_LOG(LogTemp, Log, TEXT("URiftInventoryGridWidget::OnItemAdded — Item: %s, Container: %s, TrackedContainer valid: %s"),
        IsValid(Item) ? *Item->GetName() : TEXT("null"),
        IsValid(Container) ? *Container->GetContainerTag().ToString() : TEXT("null"),
        TrackedContainer.IsValid() ? TEXT("true") : TEXT("false"));

    if (!IsValid(Item) || !IsValid(Container) || !IsValid(ContainerDefinition) || Container->GetContainerTag() != ContainerDefinition->GetContainerTag())
    {
        return;
    }

    AddSlotForItem(Item);
}

void URiftInventoryGridWidget::AddSlotForItem(URiftItemInstance* Item)
{
    if (!IsValid(Item) || !IsValid(ItemSlotWidgetClass) || !IsValid(ItemGrid))
    {
        return;
    }

    URiftItemSlotWidget* SlotWidget = CreateWidget<URiftItemSlotWidget>(GetOwningPlayer(), ItemSlotWidgetClass);
    if (!IsValid(SlotWidget))
    {
        return;
    }

    const int32 SlotIndex = SlotWidgets.Add(SlotWidget);
    const int32 Column = SlotIndex % ColumnsPerRow;
    const int32 Row = SlotIndex / ColumnsPerRow;

    UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(ItemGrid->AddChild(SlotWidget));
    if (IsValid(GridSlot))
    {
        GridSlot->SetColumn(Column);
        GridSlot->SetRow(Row);
    }

    ItemGrid->InvalidateLayoutAndVolatility();
    SlotWidget->SetItem(Item, TrackedContainer.Get(), SlotIndex);
}

void URiftInventoryGridWidget::OnItemRemoved(URiftItemInstance* Item, URiftContainer* Container)
{
    if (!IsValid(Item) || !IsValid(Container) || !IsValid(ContainerDefinition) || Container->GetContainerTag() != ContainerDefinition->GetContainerTag())
    {
        return;
    }

    // Find the slot widget showing this item and remove it
    for (int32 i = SlotWidgets.Num() - 1; i >= 0; --i)
    {
        if (IsValid(SlotWidgets[i]) && SlotWidgets[i]->GetItemInstance() == Item)
        {
            ItemGrid->RemoveChild(SlotWidgets[i]);
            SlotWidgets.RemoveAt(i);
            break;
        }
    }
}

void URiftInventoryGridWidget::BuildSlots()
{
    UE_LOG(LogTemp, Log, TEXT("URiftInventoryGridWidget::BuildSlots — ItemGrid valid: %s, ItemSlotWidgetClass valid: %s, InventoryComponent valid: %s"),
        IsValid(ItemGrid) ? TEXT("true") : TEXT("false"),
        IsValid(ItemSlotWidgetClass) ? TEXT("true") : TEXT("false"),
        InventoryComponent.IsValid() ? TEXT("true") : TEXT("false"));

    if (!IsValid(ItemGrid) || !IsValid(ItemSlotWidgetClass) || !InventoryComponent.IsValid() || !IsValid(ContainerDefinition))
    {
        return;
    }

    ItemGrid->ClearChildren();
    SlotWidgets.Empty();

    TrackedContainer = InventoryComponent->GetContainerByTag(ContainerDefinition->GetContainerTag());
    if (!TrackedContainer.IsValid())
    {
        return;
    }

    TArray<URiftItemInstance*> Items = TrackedContainer->GetAllItems();
    for (URiftItemInstance* Item : Items)
    {
        AddSlotForItem(Item);
    }
}

void URiftInventoryGridWidget::BindToInventory()
{
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    InventoryComponent->OnItemAdded.AddDynamic(this, &URiftInventoryGridWidget::OnItemAdded);
    InventoryComponent->OnItemRemoved.AddDynamic(this, &URiftInventoryGridWidget::OnItemRemoved);
}

void URiftInventoryGridWidget::UnbindFromInventory()
{
    if (!InventoryComponent.IsValid())
    {
        return;
    }

    InventoryComponent->OnItemAdded.RemoveDynamic(this, &URiftInventoryGridWidget::OnItemAdded);
    InventoryComponent->OnItemRemoved.RemoveDynamic(this, &URiftInventoryGridWidget::OnItemRemoved);
    InventoryComponent->OnInventoryInitialized.RemoveDynamic(this, &URiftInventoryGridWidget::OnInventoryInitialized);
}

URiftInventoryComponent* URiftInventoryGridWidget::FindInventoryComponent() const
{
    APlayerController* PC = GetOwningPlayer();
    if (!IsValid(PC))
    {
        return nullptr;
    }

    APlayerState* PS = PC->GetPlayerState<APlayerState>();
    if (!IsValid(PS))
    {
        return nullptr;
    }

    return PS->FindComponentByClass<URiftInventoryComponent>();
}

int32 URiftInventoryGridWidget::FindSlotIndexOfItem(URiftItemInstance* Item) const
{
    if (!TrackedContainer.IsValid() || !IsValid(Item))
    {
        return INDEX_NONE;
    }

    return TrackedContainer->GetSlotIndexOfItem(Item);
}
