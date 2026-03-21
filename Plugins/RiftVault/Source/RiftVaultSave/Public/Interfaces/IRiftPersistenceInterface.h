#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IRiftPersistenceInterface.generated.h"

/**
 * Save data payload. Backend-agnostic — JSON, binary, or whatever the game backend expects.
 */
USTRUCT(BlueprintType)
struct RIFTVAULTSAVE_API FRiftInventorySaveData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "RiftVault|Persistence")
    FString PlayerId;

    UPROPERTY(BlueprintReadWrite, Category = "RiftVault|Persistence")
    TArray<uint8> InventoryData;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSaveComplete, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnLoadComplete, bool, bSuccess, const FRiftInventorySaveData&, SaveData);

UINTERFACE(MinimalAPI, Blueprintable)
class URiftPersistenceInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Boundary between RiftVault and the game's persistence backend.
 * Implement this in the game project against EOS, REST, local disk, etc.
 */
class RIFTVAULTSAVE_API IRiftPersistenceInterface
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Interfaces|Persistence")
    void SaveInventory(const FRiftInventorySaveData& Data, const FOnSaveComplete& OnComplete);
    virtual void SaveInventory_Implementation(const FRiftInventorySaveData& Data, const FOnSaveComplete& OnComplete);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RiftVault|Interfaces|Persistence")
    void LoadInventory(const FString& PlayerId, const FOnLoadComplete& OnComplete);
    virtual void LoadInventory_Implementation(const FString& PlayerId, const FOnLoadComplete& OnComplete);
};
