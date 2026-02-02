#include <Global.h>
#include <PCH.h>

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

// Install the menu hooks
using ContDoItemTransfer_t = void(RE::ContainerMenu *, std::uint32_t, std::uint32_t, bool);
ContDoItemTransfer_t *_originalContDoItemTransfer = nullptr;
void MyContDoItemTransfer(RE::ContainerMenu* menu, std::uint32_t a_itemIndex, std::uint32_t a_count, bool a_fromContainer) {
    if (DEBUGGING)
        REX::INFO("MyContDoItemTransfer: Attempting to transfer item at index {} (count: {}) from container: {}", a_itemIndex, a_count, a_fromContainer);
    // Early exit if both locks are disabled
    if ((!LOCK_EQUIPPED && !LOCK_FAVORITES) || a_fromContainer) {
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
    // Start checking items
    bool bIsEquipped = false;
    bool bIsFavorite = false;
    // Inspect the InventoryUserUIInterfaceEntry used by the menu
    const auto* entryPassed = menu->GetInventoryItemByListIndex(a_fromContainer, a_itemIndex);
    const auto* entryFlipped= menu->GetInventoryItemByListIndex(!a_fromContainer, a_itemIndex);
    // Check only one valid entry
    const auto* entry = entryPassed ? entryPassed : entryFlipped;
    if (entry && entry->invHandle.id != 0xFFFFFFFFu) {
        auto* invItem = invInterface->RequestInventoryItem(entry->invHandle.id);
        if (invItem) {
            // Get stackId from entry
            std::uint32_t stackId = 0; bool haveStackId = false;
            if (!entry->stackIndex.empty()) { stackId = static_cast<std::uint32_t>(entry->stackIndex[0]); haveStackId = true; }

            if (haveStackId) {
                if (IsItemEquipped(invItem, stackId)) 
                    bIsEquipped = true;
                if (IsItemFavorite(invItem, stackId))
                    bIsFavorite = true;
            } else {
                // Do not treat the whole invItem as favorite (avoids blocking other stacks)
                if (DEBUGGING)
                    REX::INFO("No stackIndex for entry (handle {}), skipping instance-favorite check to avoid false positives", entry->invHandle.id);
            }
        }
    }
    // If the item is a favorite and LOCK_FAVORITES is true, prevent transfer
    if ((LOCK_EQUIPPED && bIsEquipped) || (LOCK_FAVORITES && bIsFavorite)) {
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
    // Early exit if both locks are disabled
    if ((!LOCK_EQUIPPED && !LOCK_FAVORITES) || a_fromContainer) {
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
    // Start checking items
    bool bIsEquipped = false;
    bool bIsFavorite = false;
    // Inspect the InventoryUserUIInterfaceEntry used by the menu
    const auto* entryPassed = menu->GetInventoryItemByListIndex(a_fromContainer, a_itemIndex);
    const auto* entryFlipped= menu->GetInventoryItemByListIndex(!a_fromContainer, a_itemIndex);
    // Check only one valid entry
    const auto* entry = entryPassed ? entryPassed : entryFlipped;
    if (entry && entry->invHandle.id != 0xFFFFFFFFu) {
        auto* invItem = invInterface->RequestInventoryItem(entry->invHandle.id);
        if (invItem) {
            // Get stackId from entry
            std::uint32_t stackId = 0; bool haveStackId = false;
            if (!entry->stackIndex.empty()) { stackId = static_cast<std::uint32_t>(entry->stackIndex[0]); haveStackId = true; }

            if (haveStackId) {
                if (IsItemEquipped(invItem, stackId)) 
                    bIsEquipped = true;
                if (IsItemFavorite(invItem, stackId))
                    bIsFavorite = true;
            } else {
                // Do not treat the whole invItem as favorite (avoids blocking other stacks)
                if (DEBUGGING)
                    REX::INFO("MyBartDoItemTransfer: No stackIndex for entry (handle {}), skipping instance-favorite check to avoid false positives", entry->invHandle.id);
            }
        }
    }
    // If the item is equipped and LOCK_EQUIPPED is true, prevent transfer
    if ((LOCK_EQUIPPED && bIsEquipped) || (LOCK_FAVORITES && bIsFavorite)) {
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
    // Start checking items
    bool bIsEquipped = false;
    bool bIsFavorite = false;
    // Get the InventoryItem for the scrapped item (make sure to use a pointer or equiped check will fail)
    const auto& entry = menu->invInterface.stackedEntries[index];
    if (entry.invHandle.id != 0xFFFFFFFFu) {
        auto* invItem = invInterface->RequestInventoryItem(entry.invHandle.id);
        if (invItem) {
            // Get stackId from entry
            std::uint32_t stackId = 0; bool haveStackId = false;
            if (!entry.stackIndex.empty()) { stackId = static_cast<std::uint32_t>(entry.stackIndex[0]); haveStackId = true; }
            if (haveStackId) {
                if (IsItemEquipped(invItem, stackId)) 
                    bIsEquipped = true;
                if (IsItemFavorite(invItem, stackId))
                    bIsFavorite = true;
            } else {
                // Do not treat the whole invItem as favorite (avoids blocking other stacks)
                if (DEBUGGING)
                    REX::INFO("MyScrapOnAccept: No stackIndex for entry (handle {}), skipping instance-favorite check to avoid false positives", entry.invHandle.id);
            }
        }
    }
    if ((LOCK_EQUIPPED && bIsEquipped) || (LOCK_FAVORITES && bIsFavorite)) {
        if (DEBUGGING)
            REX::INFO("MyScrapOnAccept: Scrap blocked for protected item at index {}", index);
        return; // Prevent scrap
    }
    // Otherwise forward
    _originalScrapOnAccept(self);
}

// General hook installation function
bool InstallContainerMenuHooks() {
    // Get the vtable for ExamineMenu
    auto vtbl0 = REL::Relocation<std::uintptr_t>(RE::VTABLE::ContainerMenu[0]);
    // Overwrite vfunc at index 0x15 (21 decimal)
    _originalContDoItemTransfer = reinterpret_cast<ContDoItemTransfer_t *>(vtbl0.write_vfunc(0x15, &MyContDoItemTransfer));
    auto vtbl1 = REL::Relocation<std::uintptr_t>(RE::VTABLE::BarterMenu[0]);
    // Overwrite vfunc at index 0x15 (22 decimal)
    _originalBartDoItemTransfer = reinterpret_cast<BartDoItemTransfer_t *>(vtbl1.write_vfunc(0x15, &MyBartDoItemTransfer));
    auto vtbl2 = REL::Relocation<std::uintptr_t>(RE::VTABLE::__ScrapItemCallback[0]);
    // Overwrite vfunc at index 0x01 (1 decimal)
    _originalScrapOnAccept = reinterpret_cast<ScrapOnAccept_t*>(vtbl2.write_vfunc(0x01, &MyScrapOnAccept));
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