#pragma once
#include <Global.h>

// --- Hooks ---

// -- Structs ---

// --- Functions ---

bool IsItemEquipped(const RE::BGSInventoryItem* a_item, std::uint32_t a_stackId);
bool IsItemFavorite(const RE::BGSInventoryItem* a_item, std::uint32_t a_stackId);

bool InstallContainerMenuHooks();
bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine *vm);