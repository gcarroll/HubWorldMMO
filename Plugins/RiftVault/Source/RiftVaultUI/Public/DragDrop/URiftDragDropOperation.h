#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "URiftDragDropOperation.generated.h"

class URiftItemInstance;
class URiftContainer;

/**
 * Drag and Drop operation for RiftVault inventory items.
 *
 * Carries the item being dragged, its source container, and its source slot index.
 * The drop target uses this information to perform a move or swap via
 * URiftInventoryComponent.
 *
 * Blueprintable so games can extend or replace the visual and logic behavior.
 * The default class can be overridden in Project Settings via URiftInventorySettings.
 */
UCLASS(Blueprintable, BlueprintType, DisplayName = "Rift Drag Drop Operation")
class RIFTVAULTUI_API URiftDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:

    /**
     * Creates a drag drop operation for the given item.
     * Uses the provided class or falls back to URiftDragDropOperation if null.
     *
     * @param InItemInstance        The item being dragged.
     * @param InSourceContainer     The container the item came from.
     * @param InSourceSlotIndex     The slot index the item occupied.
     * @param InQuantityToDrag      How many units to drag. -1 means drag the entire stack.
     * @param OperationClass        Optional override class. Defaults to URiftDragDropOperation.
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|UI|Drag and Drop")
    static URiftDragDropOperation* CreateRiftDragDropOperation(
        URiftItemInstance* InItemInstance,
        URiftContainer* InSourceContainer,
        int32 InSourceSlotIndex,
        int32 InQuantityToDrag = -1,
        TSubclassOf<URiftDragDropOperation> OperationClass = nullptr);

    /** The item instance being dragged. */
    UPROPERTY(BlueprintReadOnly, Category = "RiftVault|UI|Drag and Drop")
    TObjectPtr<URiftItemInstance> ItemInstance;

    /** The container the item was dragged from. */
    UPROPERTY(BlueprintReadOnly, Category = "RiftVault|UI|Drag and Drop")
    TObjectPtr<URiftContainer> SourceContainer;

    /** The slot index the item was dragged from. */
    UPROPERTY(BlueprintReadOnly, Category = "RiftVault|UI|Drag and Drop")
    int32 SourceSlotIndex = INDEX_NONE;

    /**
     * How many units of this stack are being dragged.
     * -1 means the entire stack is being moved.
     * A positive value indicates a partial stack drag (split).
     * The drop target is responsible for reading this and calling the appropriate
     * stack operations on URiftInventoryComponent.
     */
    UPROPERTY(BlueprintReadOnly, Category = "RiftVault|UI|Drag and Drop")
    int32 QuantityToDrag = -1;
};
