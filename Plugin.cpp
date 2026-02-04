#include <Global.h>
#include <PCH.h>

// Helper to check the entry
bool CheckEquippedOrFavorite(RE::BGSInventoryInterface* invInterface, const RE::InventoryUserUIInterfaceEntry* a_entry) {
    // Start checking items
    bool bIsEquipped = false;
    bool bIsFavorite = false;
    if (a_entry && a_entry->invHandle.id != 0xFFFFFFFFu) {
        auto* invItem = invInterface->RequestInventoryItem(a_entry->invHandle.id);
        if (invItem) {
            // Get stackId from entry
            std::uint32_t stackId = 0; bool haveStackId = false;
            if (!a_entry->stackIndex.empty()) { stackId = static_cast<std::uint32_t>(a_entry->stackIndex[0]); haveStackId = true; }
            if (haveStackId) {
                if (IsItemEquipped(invItem, stackId)) {
                    if (DEBUGGING)
                        REX::INFO("CheckEquippedOrFavorite: Item (handle {}) is equipped (stackId {})", a_entry->invHandle.id, stackId);
                    bIsEquipped = true;
                }
                if (IsItemFavorite(invItem, stackId)) {
                    if (DEBUGGING)
                        REX::INFO("CheckEquippedOrFavorite: Item (handle {}) is favorite (stackId {})", a_entry->invHandle.id, stackId);
                    bIsFavorite = true;
                }
            } else {
                // Do not treat the whole invItem as favorite (avoids blocking other stacks)
                if (DEBUGGING)
                    REX::INFO("CheckEquippedOrFavorite: No stackIndex for entry (handle {}), skipping instance-favorite check to avoid false positives", a_entry->invHandle.id);
            }
        }
    }
    return ( (LOCK_EQUIPPED && bIsEquipped) || (LOCK_FAVORITES && bIsFavorite) );
}

// Helper to check if the item is equipped
bool IsItemEquipped(const RE::BGSInventoryItem* a_item, std::uint32_t a_stackId) {
    if (!a_item) return false;
    auto* stack = a_item->GetStackByID(a_stackId);
    return stack && stack->IsEquipped();
}

// Helper to check if the item is a favorite
bool IsItemFavorite(const RE::BGSInventoryItem* a_item, std::uint32_t a_stackId) {
    if (!a_item) return false;
    auto* stack = a_item->GetStackByID(a_stackId);
    return stack && stack->extra && stack->extra->IsFavorite();
}

// Helper to check if the container is a dead actor's corpse
bool IsContainerDeadActor(RE::ContainerMenu* a_menu) {
    if (!a_menu) return false;
    auto* containerRef = a_menu->containerRef.get().get();
    if (!containerRef) return false;
    auto* actor = containerRef->As<RE::Actor>();
    return actor && actor->IsDead(true);
}

// Install the menu hooks
using ContDoItemTransfer_t = void(RE::ContainerMenu *, std::uint32_t, std::uint32_t, bool);
ContDoItemTransfer_t *_originalContDoItemTransfer = nullptr;
void MyContDoItemTransfer(RE::ContainerMenu* menu, std::uint32_t a_itemIndex, std::uint32_t a_count, bool a_fromContainer) {
    if (DEBUGGING)
        REX::INFO("MyContDoItemTransfer: Attempting to transfer item at index {} (count: {}) from container: {}", a_itemIndex, a_count, a_fromContainer);
    // Early exit if conditions to lock are not met
    if ((!LOCK_EQUIPPED && !LOCK_FAVORITES) || (a_fromContainer && !LOCK_BIDIRECTIONAL)) {
        _originalContDoItemTransfer(menu, a_itemIndex, a_count, a_fromContainer);
        return;
    }
    // Check if the container is a dead actor's corpse
    if (IsContainerDeadActor(menu)) {
        if (DEBUGGING)
            REX::INFO("MyContDoItemTransfer: Container is a dead actor's corpse, skipping transfer restrictions");
        _originalContDoItemTransfer(menu, a_itemIndex, a_count, a_fromContainer);
        return;
    }
    // Access the BGSInventoryInterface singleton
    auto* invInterface = RE::BGSInventoryInterface::GetSingleton();
    if (!invInterface) {
        if (DEBUGGING)
            REX::INFO("MyContDoItemTransfer: BGSInventoryInterface singleton not found");
        _originalContDoItemTransfer(menu, a_itemIndex, a_count, a_fromContainer);
        return;
    }
    // Inspect the InventoryUserUIInterfaceEntry used by the menu
    const auto* entryPassed = menu->GetInventoryItemByListIndex(a_fromContainer, a_itemIndex);
    const auto* entryFlipped= menu->GetInventoryItemByListIndex(!a_fromContainer, a_itemIndex);
    // Check only one valid entry
    const auto* entry = entryPassed ? entryPassed : entryFlipped;
    // Check if the item is equipped or favorite
    bool blocked = CheckEquippedOrFavorite(invInterface, entry);
    // If the item is blocked, prevent transfer
    if (blocked) {
        if (DEBUGGING)
            REX::INFO("MyContDoItemTransfer: Transfer blocked for protected item at index {}", a_itemIndex);
        return; // Block the transfer
    }
    // Custom behavior can be added here
    _originalContDoItemTransfer(menu, a_itemIndex, a_count, a_fromContainer);
}

using BartDoItemTransfer_t = void(RE::BarterMenu*, std::uint32_t, std::uint32_t, bool);
BartDoItemTransfer_t *_originalBartDoItemTransfer = nullptr;
void MyBartDoItemTransfer(RE::BarterMenu* menu, std::uint32_t a_itemIndex, std::uint32_t a_count, bool a_fromContainer) {
    if (DEBUGGING)
        REX::INFO("MyBartDoItemTransfer: function called for item index {}", a_itemIndex);
    // Early exit if conditions to lock are not met
    if ((!LOCK_EQUIPPED && !LOCK_FAVORITES) || (a_fromContainer && !LOCK_BIDIRECTIONAL)) {
        _originalBartDoItemTransfer(menu, a_itemIndex, a_count, a_fromContainer);
        return;
    }
    // Access the BGSInventoryInterface singleton
    auto* invInterface = RE::BGSInventoryInterface::GetSingleton();
    if (!invInterface) {
        if (DEBUGGING)
            REX::INFO("MyBartDoItemTransfer: BGSInventoryInterface singleton not found");
        _originalBartDoItemTransfer(menu, a_itemIndex, a_count, a_fromContainer);
        return;
    }
    // Inspect the InventoryUserUIInterfaceEntry used by the menu
    const auto* entryPassed = menu->GetInventoryItemByListIndex(a_fromContainer, a_itemIndex);
    const auto* entryFlipped= menu->GetInventoryItemByListIndex(!a_fromContainer, a_itemIndex);
    // Check only one valid entry
    const auto* entry = entryPassed ? entryPassed : entryFlipped;
    // Check if the item is equipped or favorite
    bool blocked = CheckEquippedOrFavorite(invInterface, entry);
    // If the item is blocked, prevent transfer
    if (blocked) {
        if (DEBUGGING)
            REX::INFO("MyBartDoItemTransfer: Transfer blocked for protected item at index {}", a_itemIndex);
        return; // Block the transfer
    }
    _originalBartDoItemTransfer(menu, a_itemIndex, a_count, a_fromContainer);
}

using ScrapOnAccept_t = void(RE::ScrapItemCallback*);
ScrapOnAccept_t* _originalScrapOnAccept = nullptr;
void MyScrapOnAccept(RE::ScrapItemCallback* self) {
    if (DEBUGGING)
        REX::INFO("MyScrapOnAccept: function called");
    // Early exit if scrapping lock is disabled
    if (!LOCK_SCRAP || (!LOCK_EQUIPPED && !LOCK_FAVORITES)) {
        _originalScrapOnAccept(self);
        return;
    }
    if (!self || !self->thisMenu) {
        if (DEBUGGING)
            REX::INFO("MyScrapOnAccept: Invalid ScrapItemCallback or thisMenu is null");
        _originalScrapOnAccept(self);
        return;
    }
    // Get the ExamineMenu and index
    auto* menu = self->thisMenu;
    auto index = menu->GetSelectedIndex();
    //auto index = self->itemIndex;
    // Access the BGSInventoryInterface singleton
    auto* invInterface = RE::BGSInventoryInterface::GetSingleton();
    if (!invInterface) {
        if (DEBUGGING)
            REX::INFO("MyScrapOnAccept: BGSInventoryInterface singleton not found");
        _originalScrapOnAccept(self);
        return;
    }
    // Get the InventoryItem for the scrapped item (make sure to use a pointer or equiped check will fail)
    const auto& entry = menu->invInterface.stackedEntries[index];
    // Check if the item is equipped or favorite
    bool blocked = CheckEquippedOrFavorite(invInterface, &entry);
    // If the item is blocked, prevent scrapping
    if (blocked) {
        if (DEBUGGING)
            REX::INFO("MyScrapOnAccept: Scrap blocked for protected item at index {}", index);
        return; // Prevent scrap
    }
    // Otherwise forward
    _originalScrapOnAccept(self);
}

// Replace ContainerMenu::TakeAllItems to handle locking
using TakeAllItems_t = void(RE::ContainerMenu*);
TakeAllItems_t* _originalTakeAllItems = nullptr;
void MyTakeAllItems(RE::ContainerMenu* menu) {
    REX::INFO("MyTakeAllItems: function called");
    std::int32_t counter = 0;
    auto* invInterface = RE::BGSInventoryInterface::GetSingleton();
    if (!invInterface) {
        if (DEBUGGING) REX::WARN("MyTakeAllItems: BGSInventoryInterface unavailable, forwarding to original TakeAll");
        // Do not call the original function, it crashes the game
        //_originalTakeAllItems(menu);
        return;
    }
    // Go over the inventory backwards to avoid index shifting indices issues
    for (int i = static_cast<int>(menu->containerInv.stackedEntries.size()) - 1; i >= 0; --i) {
        const auto& entry = menu->containerInv.stackedEntries[i];
        if (entry.invHandle.id == 0xFFFFFFFFu || entry.stackIndex.empty())
            continue;
        if (entry.stackIndex.empty())
            continue;
        // Check if we get a valis InventoryItem at this index to get the count
        auto* invItem = invInterface->RequestInventoryItem(entry.invHandle.id);
        if (!invItem)
            continue;
        auto transferCount = invItem->GetCount();
        if (transferCount <= 0)
            continue;
        // Transfer the item with the original function to check for locks
        menu->DoItemTransfer(static_cast<std::uint32_t>(i), transferCount, true);
        // Update the menu to reflect changes or only one item may be transferred at a time
        menu->UpdateList(true);
        counter += static_cast<std::int32_t>(transferCount);
    }
    // Finally, update encumbrance and caps
    menu->UpdateEncumbranceAndCaps(0, true);
    REX::INFO("MyTakeAllItems: function funinished, total items attempted to transfer: {}", counter);
}

// General hook installation function
bool InstallContainerMenuHooks() {
    // Get the vtable for ExamineMenu
    auto vtbl0 = REL::Relocation<std::uintptr_t>(RE::VTABLE::ContainerMenu[0]);
    // Overwrite vfunc at index 0x15 (21 decimal)
    _originalContDoItemTransfer = reinterpret_cast<ContDoItemTransfer_t *>(vtbl0.write_vfunc(0x15, &MyContDoItemTransfer));
    REX::INFO("InstallContainerMenuHooks: Hooked ContainerMenu::DoItemTransfer");
    // Get the vtable for BarterMenu
    auto vtbl1 = REL::Relocation<std::uintptr_t>(RE::VTABLE::BarterMenu[0]);
    // Overwrite vfunc at index 0x15 (22 decimal)
    _originalBartDoItemTransfer = reinterpret_cast<BartDoItemTransfer_t *>(vtbl1.write_vfunc(0x15, &MyBartDoItemTransfer));
    REX::INFO("InstallContainerMenuHooks: Hooked BarterMenu::DoItemTransfer");
    // Get the vtable for ScrapItemCallback
    auto vtbl2 = REL::Relocation<std::uintptr_t>(RE::VTABLE::__ScrapItemCallback[0]);
    // Overwrite vfunc at index 0x01 (1 decimal)
    _originalScrapOnAccept = reinterpret_cast<ScrapOnAccept_t*>(vtbl2.write_vfunc(0x01, &MyScrapOnAccept));
    REX::INFO("InstallContainerMenuHooks: Hooked ScrapItemCallback::OnAccept");
    if (!LOCK_TAKEALL) {
        REX::INFO("InstallContainerMenuHooks: All menu hooks installed.");
        return true;
    }
    // Try to find the relocation ID for RE::ContainerMenu::TakeAllItems
    REX::INFO("InstallContainerMenuHooks: Installing ContainerMenu::TakeAllItems hook...");
    // Map per-version
    REL::ID takeAllId = REL::ID(1323703); // fallback / older (1.10.163)
    if (g_moduleName == "InvLockerCLX") {
        takeAllId = REL::ID(2248619);
    } else if (g_moduleName == "InvLockerCLXAE") {
        takeAllId = REL::ID(2248619);
    }
    // Create trampoline for branch/call hooks (do this BEFORE write_branch calls)
    bool skipTakeAllHook = false;
    try {
        constexpr std::size_t TRAMPOLINE_SIZE = 64 * 1024;
        F4SE::GetTrampoline().create(TRAMPOLINE_SIZE); // let create() choose module
        REX::INFO("Trampoline created ({} bytes)", TRAMPOLINE_SIZE);
    } catch (const std::exception& e) {
        REX::WARN("Failed to create trampoline: {}", e.what());
        skipTakeAllHook = true; // guard installing branch hooks later
    }
    if (skipTakeAllHook) {
        REX::WARN("InstallContainerMenuHooks: Skipping ContainerMenu::TakeAllItems hook due to trampoline creation failure.");
        return true; // continue without this hook
    }
    using func_t = decltype(&RE::ContainerMenu::TakeAllItems);
    static REL::Relocation<func_t> rel{ takeAllId };
    _originalTakeAllItems = reinterpret_cast<TakeAllItems_t*>(F4SE::GetTrampoline().write_branch<6>(rel.address(), MyTakeAllItems));
    REX::INFO("InstallContainerMenuHooks: hooked ContainerMenu::TakeAllItems using REL id {}", takeAllId.id());
    // Log completion
    REX::INFO("InstallContainerMenuHooks: All menu hooks installed.");
    return true;
}

// Register Papyrus functions
bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine *vm) {
    if (DEBUGGING)
        REX::INFO("RegisterPapyrusFunctions: Attempting to register Papyrus functions. VM pointer: {}", static_cast<const void *>(vm));
    // vm->BindNativeMethod("<Name of the script binding the function>", "<Name of the function in Papyrus>", <Name of
    // the function in F4SE>, <can run parallel to Papyrus>);
    //vm->BindNativeMethod("NL_API", "GetDebugging", GetDebugging_Native, true);
    if (DEBUGGING)
        REX::INFO("RegisterPapyrusFunctions: All Papyrus functions registration attempts completed.");
    return true;
}