#include "GameModeBase.h"

#include "reboot.h"
#include "FortPlayerControllerAthena.h"
#include "FortGameModeAthena.h"
#include "FortLootPackage.h"
#include "FortAthenaMutator_GiveItemsAtGamePhaseStep.h"
#include "DataTableFunctionLibrary.h"
#include "FortAthenaMutator_GG.h"
#include "FortAthenaMutator_InventoryOverride.h"

void AGameModeBase::RestartPlayerAtTransform(AController* NewPlayer, FTransform SpawnTransform)
{
	static auto RestartPlayerAtTransformFn = FindObject<UFunction>("/Script/Engine.GameModeBase.RestartPlayerAtTransform");

	struct
	{
		AController* NewPlayer;                                                // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		FTransform                                  SpawnTransform;                                           // (ConstParm, Parm, OutParm, ReferenceParm, IsPlainOldData, NoDestructor, NativeAccessSpecifierPublic)
	} AGameModeBase_RestartPlayerAtTransform_Params{ NewPlayer, SpawnTransform };

	this->ProcessEvent(RestartPlayerAtTransformFn, &AGameModeBase_RestartPlayerAtTransform_Params);
}

void AGameModeBase::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	static auto RestartPlayerAtPlayerStartFn = FindObject<UFunction>("/Script/Engine.GameModeBase.RestartPlayerAtPlayerStart");

	struct
	{
		AController* NewPlayer;                                                // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		AActor* StartSpot;                                                // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	} AGameModeBase_RestartPlayerAtPlayerStart_Params{ NewPlayer, StartSpot };

	this->ProcessEvent(RestartPlayerAtPlayerStartFn, &AGameModeBase_RestartPlayerAtPlayerStart_Params);
}

void AGameModeBase::RestartPlayer(AController* NewPlayer)
{
	static auto RestartPlayerFn = FindObject<UFunction>("/Script/Engine.GameModeBase.RestartPlayer");
	this->ProcessEvent(RestartPlayerFn, &NewPlayer);
}

UClass* AGameModeBase::GetDefaultPawnClassForController(AController* InController)
{
	static auto GetDefaultPawnClassForControllerFn = FindObject<UFunction>("/Script/Engine.GameModeBase.GetDefaultPawnClassForController");
	struct
	{
		AController* InController;                                             // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		UClass* ReturnValue;                                              // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	} AGameModeBase_GetDefaultPawnClassForController_Params{InController};

	this->ProcessEvent(GetDefaultPawnClassForControllerFn, &AGameModeBase_GetDefaultPawnClassForController_Params);

	return AGameModeBase_GetDefaultPawnClassForController_Params.ReturnValue;
}

void AGameModeBase::ChangeName(AController* Controller, const FString& NewName, bool bNameChange)
{
	static auto ChangeNameFn = FindObject<UFunction>("/Script/Engine.GameModeBase.ChangeName");

	struct
	{
		AController* Controller;                                               // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		struct FString                                     NewName;                                                  // (Parm, ZeroConstructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		bool                                               bNameChange;                                              // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	} AGameModeBase_ChangeName_Params{ Controller, NewName, bNameChange };

	this->ProcessEvent(ChangeNameFn, &AGameModeBase_ChangeName_Params);
}

AActor* AGameModeBase::K2_FindPlayerStart(AController* Player, FString IncomingName)
{
	static auto K2_FindPlayerStartFn = FindObject<UFunction>("/Script/Engine.GameModeBase.K2_FindPlayerStart");

	struct
	{
		AController* Player;                                                   // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		FString                                     IncomingName;                                             // (Parm, ZeroConstructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		AActor* ReturnValue;                                              // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	} AGameModeBase_K2_FindPlayerStart_Params{ Player, IncomingName };

	this->ProcessEvent(K2_FindPlayerStartFn, &AGameModeBase_K2_FindPlayerStart_Params);
	
	return AGameModeBase_K2_FindPlayerStart_Params.ReturnValue;
}

APawn* AGameModeBase::SpawnDefaultPawnForHook(AGameModeBase* GameMode, AController* NewPlayer, AActor* StartSpot)
{
	LOG_INFO(LogDev, "SpawnDefaultPawnForHook!");

	auto NewPlayerAsAthena = Cast<AFortPlayerControllerAthena>(NewPlayer);

	if (!NewPlayerAsAthena)
		return nullptr; // return original?

	auto PlayerStateAthena = NewPlayerAsAthena->GetPlayerStateAthena();

	if (!PlayerStateAthena)
		return nullptr; // return original?

	static auto PawnClass = FindObject<UClass>("/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C");
	static auto DefaultPawnClassOffset = GameMode->GetOffset("DefaultPawnClass");
	GameMode->Get<UClass*>(DefaultPawnClassOffset) = PawnClass;

	constexpr bool bUseSpawnActor = false;

	static auto SpawnDefaultPawnAtTransformFn = FindObject<UFunction>(L"/Script/Engine.GameModeBase.SpawnDefaultPawnAtTransform");

	FTransform SpawnTransform = StartSpot->GetTransform();
	APawn* NewPawn = nullptr;

	if constexpr (bUseSpawnActor)
	{
		NewPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, CreateSpawnParameters(ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn));
	}
	else
	{
		struct { AController* NewPlayer; FTransform SpawnTransform; APawn* ReturnValue; }
		AGameModeBase_SpawnDefaultPawnAtTransform_Params{ NewPlayer, SpawnTransform };

		GameMode->ProcessEvent(SpawnDefaultPawnAtTransformFn, &AGameModeBase_SpawnDefaultPawnAtTransform_Params);

		NewPawn = AGameModeBase_SpawnDefaultPawnAtTransform_Params.ReturnValue;
	}

	if (!NewPawn)
		return nullptr;

	bool bIsRespawning = false;

	/*
	static auto RespawnDataOffset = PlayerStateAthena->GetOffset("RespawnData", false);

	if (RespawnDataOffset != -1)
	{
		auto RespawnDataPtr = PlayerStateAthena->GetRespawnData();

		if (RespawnDataPtr->IsServerReady() && RespawnDataPtr->IsClientReady()) // && GameState->IsRespawningAllowed(PlayerState);
		{
			bIsRespawning = true;
		}
	} */

	/* auto DeathInfo = (void*)(__int64(PlayerStateAthena) + MemberOffsets::FortPlayerStateAthena::DeathInfo);
	FVector DeathLocation = MemberOffsets::DeathInfo::DeathLocation != -1 ? *(FVector*)(__int64(DeathInfo) + MemberOffsets::DeathInfo::DeathLocation) : FVector(0, 0, 0);

	bIsRespawning = !(DeathLocation.CompareVectors(FVector(0, 0, 0))); // bro kms */

	auto ASC = PlayerStateAthena->GetAbilitySystemComponent();
	auto GameState = ((AFortGameModeAthena*)GameMode)->GetGameStateAthena();

	GET_PLAYLIST(GameState);

	if (CurrentPlaylist) // Apply gameplay effects from playlist // We need to move this maybe?
	{
		CurrentPlaylist->ApplyModifiersToActor(PlayerStateAthena);
	}

	auto PlayerAbilitySet = GetPlayerAbilitySet(); // Apply default gameplay effects // We need to move maybe?

	if (PlayerAbilitySet && ASC)
	{
		PlayerAbilitySet->ApplyGrantedGameplayAffectsToAbilitySystem(ASC);
	}

	if (!bIsRespawning)
	{
		auto WorldInventory = NewPlayerAsAthena->GetWorldInventory();

		if (!WorldInventory)
			return NewPawn;

		if (!WorldInventory->GetPickaxeInstance())
		{
			// TODO Check Playlist->bRequirePickaxeInStartingInventory

			auto& StartingItems = ((AFortGameModeAthena*)GameMode)->GetStartingItems();

			NewPlayerAsAthena->AddPickaxeToInventory();

			for (int i = 0; i < StartingItems.Num(); i++)
			{
				auto& StartingItem = StartingItems.at(i);

				WorldInventory->AddItem(StartingItem.GetItem(), nullptr, StartingItem.GetCount());
			}

			/* if (Globals::bLateGame)
			{
				auto SpawnIslandTierGroup = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaFloorLoot_Warmup");

				for (int i = 0; i < 5; i++)
				{
					auto LootDrops = PickLootDrops(SpawnIslandTierGroup);

					for (auto& LootDrop : LootDrops)
					{
						WorldInventory->AddItem(LootDrop.ItemDefinition, nullptr, LootDrop.Count, LootDrop.LoadedAmmo);
					}
				}
			} */

			auto AddInventoryOverrideTeamLoadouts = [&](AFortAthenaMutator* Mutator)
			{
				if (auto InventoryOverride = Cast<AFortAthenaMutator_InventoryOverride>(Mutator))
				{
					auto TeamIndex = PlayerStateAthena->GetTeamIndex();
					auto LoadoutTeam = InventoryOverride->GetLoadoutTeamForTeamIndex(TeamIndex);

					if (LoadoutTeam.UpdateOverrideType == EAthenaInventorySpawnOverride::Always)
					{
						auto LoadoutContainer = InventoryOverride->GetLoadoutContainerForTeamIndex(TeamIndex);

						for (int i = 0; i < LoadoutContainer.Loadout.Num(); i++)
						{
							auto& ItemAndCount = LoadoutContainer.Loadout.at(i);
							WorldInventory->AddItem(ItemAndCount.GetItem(), nullptr, ItemAndCount.GetCount());
						}
					}
				}
			};

			LoopMutators(AddInventoryOverrideTeamLoadouts);

			WorldInventory->Update();
		}
	}
	else
	{
		// TODO I DONT KNOW WHEN TO DO THIS

		/*

		static auto DeathInfoStruct = FindObject<UStruct>(L"/Script/FortniteGame.DeathInfo");
		static auto DeathInfoStructSize = DeathInfoStruct->GetPropertiesSize();
		RtlSecureZeroMemory(DeathInfo, DeathInfoStructSize); // TODO FREE THE DEATHTAGS

		static auto OnRep_DeathInfoFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerStateAthena.OnRep_DeathInfo");

		if (OnRep_DeathInfoFn)
		{
			PlayerStateAthena->ProcessEvent(OnRep_DeathInfoFn);
		}

		*/

		// NewPlayerAsAthena->ClientClearDeathNotification();
		// NewPlayerAsAthena->RespawnPlayerAfterDeath(true);
	}

	{
		auto itr = Globals::OutfitMap.find(PlayerStateAthena->GetPlayerName().ToString());
		auto playerName = PlayerStateAthena->GetPlayerName().ToString();
		if (itr != Globals::OutfitMap.end()) {
			auto cidDef = FindObject(itr->second, nullptr, ANY_PACKAGE);
			if (cidDef) {
				LOG_INFO(LogGame, "Setting {0}'s outfit to {1}", playerName, itr->second);
				MessageBoxA(0, cidDef->GetFullName().c_str(), "Outfit", 0);
				ApplyCID((AFortPlayerPawn*)NewPawn, cidDef, true);
				LOG_INFO(LogGame, "Set {0}'s outfit to {1}", playerName, itr->second);
			} else {
				LOG_INFO(LogGame, "Set {0}'s outfit to {1} but couldn't find it", playerName, itr->second);
			}
		} else {
			LOG_INFO(LogGame, "Couldn't find {0}'s outfit", playerName);
		}
	}

	return NewPawn;
}