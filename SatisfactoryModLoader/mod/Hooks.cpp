#include <stdafx.h>
#include "Hooks.h"
#include <detours.h>
#include <stdio.h> 
#include <sstream>
#include <vector>
#include <functional>
#include <SatisfactoryModLoader.h>
#include <game/Global.h>
#include <util/Utility.h>
#include <mod/ModFunctions.h>
#include "ModFunctions.h"
#include <assets/AssetLoader.h>
#include <mod/Coremods.h>

using namespace std::placeholders;

namespace SML {
	namespace Mod {
		PVOID Hooks::chatFunc;
		PVOID Hooks::worldFunc;
		PVOID Hooks::playerAddedFunc;
		PVOID Hooks::playerControllerAddedFunc;
		PVOID Hooks::engineInitFunc;
		PVOID Hooks::levelDestroyFunc;
		PVOID Hooks::sigCheckFunc;

		void Hooks::hookFunctions() {
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			// find the function by name
			chatFunc = DetourFindFunction("FactoryGame-Win64-Shipping.exe", "AFGPlayerController::EnterChatMessage");
			DetourAttach(&(PVOID&)chatFunc, playerSentMessage);

			worldFunc = DetourFindFunction("FactoryGame-Win64-Shipping.exe", "ULevel::PostLoad");
			DetourAttach(&(PVOID&)worldFunc, getWorld);

			playerAddedFunc = DetourFindFunction("FactoryGame-Win64-Shipping.exe", "AFGGameState::NotifyPlayerAdded");
			DetourAttach(&(PVOID&)playerAddedFunc, playerAdded);

			engineInitFunc = DetourFindFunction("FactoryGame-Win64-Shipping.exe", "FEngineLoop::Init");
			DetourAttach(&(PVOID&)engineInitFunc, engineInit);

			playerControllerAddedFunc = DetourFindFunction("FactoryGame-Win64-Shipping.exe", "AFGPlayerController::PostLoad");
			DetourAttach(&(PVOID&)playerControllerAddedFunc, playerControllerAdded);

			levelDestroyFunc = DetourFindFunction("FactoryGame-Win64-Shipping.exe", "ULevel::~ULevel");
			DetourAttach(&(PVOID&)levelDestroyFunc, levelDestructor);

			sigCheckFunc = DetourFindFunction("FactoryGame-Win64-Shipping.exe", "FPakPrecacher::DoSignatureCheck");
			DetourAttach(&(PVOID&)sigCheckFunc, sigCheck);

			Utility::info("Installed hooks!");

			DetourTransactionCommit();
		}

		void Hooks::sigCheck(void* self, bool b, void* v) {

		}

		void Hooks::playerControllerAdded(SDK::AFGPlayerController* controller) {
			if (Assets::SinglePlayerController == nullptr) {
				Assets::SinglePlayerController = controller;
			}

			auto pointer = (void(WINAPI*)(void*))playerControllerAddedFunc;
			pointer(controller);
		}

		void Hooks::levelDestructor(SDK::ULevel* level) {

			Assets::SinglePlayerController = nullptr;

			auto pointer = (void(WINAPI*)(void*))levelDestroyFunc;
			pointer(level);
		}

		void Hooks::engineInit(void* engine) {
			//caching of assets
			modHandler.currentStage = GameStage::RUN;
			for (std::pair< const wchar_t*, SDK::UObject*> asset : modHandler.assetCache) {
				modHandler.assetCache[asset.first] = Assets::AssetLoader::loadObjectSimple(SDK::UClass::StaticClass(), asset.first);
			}

			for (const wchar_t* dll : delayedCoremods) {
				LoadLibraryW(dll);
			}

			auto pointer = (void(WINAPI*)(void*))engineInitFunc;
			pointer(engine);
		}

		void Hooks::playerAdded(SDK::AFGGameState* gameState, SDK::AFGCharacterPlayer* player) {
			auto pointer = (void(WINAPI*)(void*, void*))playerAddedFunc;
			//Utility::info("Player Added: ", player->GetName(), " - Controlled: ", player->IsControlled(), " - IsLocallyControlled: ", player->IsLocallyControlled());
			if (player->IsControlled() && player->IsLocallyControlled()) {
				Assets::SinglePlayerCharacter = player;
			}
			pointer(gameState, player);
		}

		void Hooks::getWorld(void* self) {
			auto pointer = (void(WINAPI*)(void*))worldFunc;
			pointer(self);
			/*
			PVOID getWorldRaw = DetourFindFunction("FactoryGame-Win64-Shipping.exe", "ULevel::GetWorld");
			auto getWorld = (void*(WINAPI*)(void*))getWorldRaw;
			Assets::CurrentWorld = getWorld(self);
			*/
			Assets::CurrentWorld = SDK::UWorld::GWorld;
		}

		// parse commands when the player sends a message
		void Hooks::playerSentMessage(void* player, SDK::FString* message) {

			auto pointer = (void(WINAPI*)(void*, void*))chatFunc;

			std::string str = message->ToString();
			Utility::info(str);
			std::vector<std::string> arguments;
			std::stringstream ss(str);
			std::string temp;
			if (str.find(' ') == std::string::npos) {
				arguments.push_back(str);
			}
			else {
				while (getline(ss, temp, ' ')) {
					arguments.push_back(temp);
				}
			}

			for (std::string s : arguments) {
				Utility::info(s);
			}

			bool found = false;
			SML::Mod::Functions::CommandData data = {
						arguments.size(),
						arguments
			};
			found = smlCommands(data); //run SML's commands
			for (Registry r : modHandler.commandRegistry) {
				if (arguments[0] == "/" + r.name) {
					auto commandFunc = (void(WINAPI*)(SML::Mod::Functions::CommandData))r.func;
					commandFunc(data);
					found = true;
				}
			}
			if (!found) {
				pointer(player, message);
			}

		}

		bool Hooks::smlCommands(Functions::CommandData data) {
			if (chatCommands) {
				if (data.argv[0] == "/sml") {
					if (data.argc == 1) {
						Utility::info("Please enter a subcommand with /sml: help, modlist, die, commands, apis, events");
					}
					else if (data.argc >= 2) {
						if (data.argv[1] == "help") {
							Utility::info("/sml help     -> Displays this help message");
							Utility::info("/sml modlist  -> Lists all the information about the loaded mods");
							Utility::info("/sml die      -> Performs a hard shutdown");
							Utility::info("/sml obj      -> Displays the memory address of various game objects");
							Utility::info("/sml commands -> Lists all the commands in the registry");
							Utility::info("/sml apis     -> Lists all the API functions in the registry");
							Utility::info("/sml events   -> Lists all the custom events in the registry");
						}
						else if (data.argv[1] == "modlist") {
							for (auto&& m : modHandler.mods) {
								Utility::info(m->info.name);
								Utility::info(m->info.version);
								Utility::info(m->info.loaderVersion);
								Utility::info(m->info.description);
								Utility::info(m->info.authors);
								Utility::info("Dependencies: ");
								for (std::string s : m->info.dependencies) {
									Utility::info(s);
								}
								Utility::info("=======================");
							}
						}
						else if (data.argv[1] == "die") {
							Utility::info("Hard shutdown requested!");
							SML::cleanup();
							abort();
						}
						else if (data.argv[1] == "commands") {
							for (Registry r : modHandler.commandRegistry) {
								Utility::info(r.name, " -> ", r.func);
							}

						}
						else if (data.argv[1] == "apis") {
							for (Registry r : modHandler.APIRegistry) {
								Utility::info(r.name, " -> ", r.func);
							}
						}
						else if (data.argv[1] == "events") {
							for (auto r : modHandler.eventRegistry) {
								Utility::info(r.first, ": ", r.second.size(), " function(s):");
								for (PVOID p : r.second) {
									Utility::info(p);
								}
								Utility::info("=======================");
							}
						}
						else if (data.argv[1] == "obj") {
							Utility::info("World:", Functions::getWorld());
							Utility::info("Local Character:", Functions::getPlayerCharacter());
							Utility::info("Local Controller: ", Functions::getPlayerController());
						}
						else {
							Utility::info("Subcommand not recognized!");
						}
					}
					return true;
				}
			}
			else {

			}
			return false;
		}

		//void get_signing_keys(ModReturns* modReturns, void* outKey) {
		//	info("Test");
		//	modReturns->use_original_function = false;
		//}
	}
}