#pragma once
#include <Global.h>

// --- Hooks ---

// -- Structs ---

// --- Functions ---

bool CheckEquippedOrFavorite(RE::BGSInventoryInterface* invInterface, const RE::InventoryUserUIInterfaceEntry* a_entry);
bool IsContainerDeadActor(RE::ContainerMenu* a_menu);
bool IsItemEquipped(const RE::BGSInventoryItem* a_item, std::uint32_t a_stackId);
bool IsItemFavorite(const RE::BGSInventoryItem* a_item, std::uint32_t a_stackId);

bool InstallContainerMenuHooks();
bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine *vm);