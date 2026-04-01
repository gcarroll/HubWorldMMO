#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DragDrop/URiftDragDropOperation.h"
#include "URiftInventorySettings.generated.h"

/**
 * Project-wide settings for the RiftVault UI module.
 * Accessible via Project Settings → Plugins → RiftVault.
 *
 * Games can override the default drag drop operation class here
 * to customize or replace drag and drop behavior entirely.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "RiftVault"))
class RIFTVAULTUI_API URiftInventorySettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:

    URiftInventorySettings();

    /**
     * Default class used for inventory drag and drop operations.
     * Override this to customize drag and drop behavior globally.
     * Individual item slot widgets can further override this per-widget.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Drag and Drop")
    TSubclassOf<URiftDragDropOperation> DragDropOperationClass;
};
