// Copyright 2023 Sabre Dart Studios

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OWSGameMode.h"
#include "HWGameMode.generated.h"

USTRUCT(BlueprintType)
struct FNpcSpawnData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
		int32 Id = 0;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
		FString NpcName;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
		float X = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
		float Y = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
		float Z = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
		float Rx = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
		float Ry = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
		float Rz = 0.f;
};

/**
 *
 */
UCLASS()
class OWSHUBWORLDMMO_API AHWGameMode : public AOWSGameMode
{
	GENERATED_BODY()

public:

	AHWGameMode();

	UPROPERTY(EditDefaultsOnly, Category = "Combat States")
		TSubclassOf<UGameplayEffect> ApplyColdGameplayEffect;
	UPROPERTY(EditDefaultsOnly, Category = "Combat States")
		TSubclassOf<UGameplayEffect> ApplyBurningGameplayEffect;
	UPROPERTY(EditDefaultsOnly, Category = "Combat States")
		TSubclassOf<UGameplayEffect> ApplyWetGameplayEffect;
	UPROPERTY(EditDefaultsOnly, Category = "Combat States")
		TSubclassOf<UGameplayEffect> ApplyElectrifiedGameplayEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Combat States")
		TSubclassOf<UGameplayEffect> ApplyFrozenGameplayEffect;
	UPROPERTY(EditDefaultsOnly, Category = "Combat States")
		TSubclassOf<UGameplayEffect> ApplyChargedGameplayEffect;

	// Populated after zone content loads; iterate in NotifyZoneNpcsLoaded to spawn actors
	UPROPERTY(BlueprintReadOnly, Category = "Content")
		TArray<FNpcSpawnData> ZoneNpcs;

	// Override in Blueprint to spawn NPC actors using ZoneNpcs
	UFUNCTION(BlueprintImplementableEvent, Category = "Content")
		void NotifyZoneNpcsLoaded();

private:

	UFUNCTION()
		void OnZoneContentLoadedHandler(FString ContentJSON);
};
