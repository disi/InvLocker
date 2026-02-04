#include <Global.h>

// Global logger pointer
std::shared_ptr<spdlog::logger> gLog;

// --- Explicit F4SE_API Definition ---
// This macro is essential for exporting functions from the DLL.
// If the F4SE headers aren't providing it correctly for your setup,
// we define it directly.
#define F4SE_API __declspec(dllexport)

// This is used by commonLibF4
namespace Version
{
    inline constexpr std::size_t MAJOR = 0;
    inline constexpr std::size_t MINOR = 1;
    inline constexpr std::size_t PATCH = 0;
    inline constexpr auto NAME = "0.1.0"sv;
    inline constexpr auto AUTHORNAME = "disi"sv;
    inline constexpr auto PROJECT = "InvLockerCL"sv;
} // namespace Version


// Global Module Name
std::string g_moduleName = "InvLockerCL";
// Declare the F4SEMessagingInterface and F4SEScaleformInterface
const F4SE::MessagingInterface *g_messaging = nullptr;
// Papyrus interface
const F4SE::PapyrusInterface *g_papyrus = nullptr;
// Task interface for menus and threads
const F4SE::TaskInterface *g_taskInterface = nullptr;
// Scaleform interface
const F4SE::ScaleformInterface *g_scaleformInterface = nullptr;
// Plugin handle
F4SE::PluginHandle g_pluginHandle = 0;
// Datahandler
RE::TESDataHandler *g_dataHandle = 0;

// Variables

// Global debug flag
bool DEBUGGING = false;
// Lock equipped inventory items
bool LOCK_EQUIPPED = true;
// Lock favorite inventory items
bool LOCK_FAVORITES = true;
// Lock scrapping of equipped and/or favorite inventory items
bool LOCK_SCRAP = true;
// Bi-directional locking
bool LOCK_BIDIRECTIONAL = true;
// Lock when using Take All Items
bool LOCK_TAKEALL = true;

// Helper function to extract value from a line
inline std::string GetValueFromLine(const std::string &line)
{
    size_t eqPos = line.find('=');
    if (eqPos == std::string::npos)
        return "";
    std::string value = line.substr(eqPos + 1);
    value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
    return value;
}
// Helper to get the directory of the plugin DLL
std::string GetPluginDirectory(HMODULE hModule)
{
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    return (pos != std::string::npos) ? fullPath.substr(0, pos + 1) : "";
}
// Helper to parse hex string to uint32_t
uint32_t ParseHexFormID(const std::string &hexStr)
{
    return static_cast<uint32_t>(std::stoul(hexStr, nullptr, 16));
}
void LoadConfig(HMODULE hModule)
{
    std::string configPath = GetPluginDirectory(hModule) + "InvLocker.ini";
    REX::INFO("LoadConfig: Loading config from: {}", configPath);
    // First try to open the stream directly
    std::ifstream file(configPath);
    // Check if the file opened successfully
    if (!file.is_open()) {
        REX::WARN("LoadConfig: Could not open INI file: {}. Creating default.", configPath);
        // Create the file with defaultIni contents
        std::ofstream out(configPath);
        if (out.is_open()) {
            out << defaultIni;
            out.close();
            REX::INFO("LoadConfig: Default INI created at: {}", configPath);
        } else {
            REX::WARN("LoadConfig: Failed to create default INI at: {}", configPath);
            return;
        }
        // Try to open again for reading
        file.open(configPath);
        if (!file.is_open()) {
            REX::WARN("LoadConfig: Still could not open INI file after creating default: {}", configPath);
            return;
        }
    }
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        // Skip comments and empty lines
        if (line.empty() || line[0] == ';')
            continue;
        // Lower case for case-insensitive comparison
        std::string lowerLine = ToLower(line);
        // --- Debugging flag ---
        if (lowerLine.find("debugging") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "false" || value == "0") {
                DEBUGGING = false;
            } else {
                DEBUGGING = true;
            }
            continue;
        }
        // --- Lock equipped inventory items flag ---
        if (lowerLine.find("lock_equipped") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "false" || value == "0") {
                LOCK_EQUIPPED = false;
            } else {
                LOCK_EQUIPPED = true;
            }
            continue;
        }
        // --- Lock favorite inventory items flag ---
        if (lowerLine.find("lock_favorites") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "false" || value == "0") {
                LOCK_FAVORITES = false;
            } else {
                LOCK_FAVORITES = true;
            }
            continue;
        }
        // --- Lock scrapping flag ---
        if (lowerLine.find("lock_scrap") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "false" || value == "0") {
                LOCK_SCRAP = false;
            } else {
                LOCK_SCRAP = true;
            }
            continue;
        }
        // --- Lock bi-directional flag ---
        if (lowerLine.find("lock_bidirectional") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "false" || value == "0") {
                LOCK_BIDIRECTIONAL = false;
            } else {
                LOCK_BIDIRECTIONAL = true;
            }
            continue;
        }
        // --- Lock Take All Items flag ---
        if (lowerLine.find("lock_takeall") == 0) {
            std::string value = GetValueFromLine(line);
            if (ToLower(value) == "false" || value == "0") {
                LOCK_TAKEALL = false;
            } else {
                LOCK_TAKEALL = true;
            }
            continue;
        }
    }
    file.close();
    REX::INFO("LoadConfig: Completed loading config.");
    REX::INFO(" - Debugging: {}", DEBUGGING);
    REX::INFO(" - Lock Equipped Inventory Items: {}", LOCK_EQUIPPED);
    REX::INFO(" - Lock Favorite Inventory Items: {}", LOCK_FAVORITES);
    REX::INFO(" - Lock Scrapping of Equipped/Favorite Items: {}", LOCK_SCRAP);
    REX::INFO(" - Lock Bi-Directional: {}", LOCK_BIDIRECTIONAL);
    REX::INFO(" - Lock Take All Items: {}", LOCK_TAKEALL);
}

// Message handler definition
void F4SEMessageHandler(F4SE::MessagingInterface::Message *a_message) {
    switch (a_message->type) {
        case F4SE::MessagingInterface::kPostLoad:
            REX::INFO("Received kMessage_PostLoad. Game data is now loaded!");
            break;
        case F4SE::MessagingInterface::kPostPostLoad:
            REX::INFO("Received kMessage_PostPostLoad. Game data finished loading.");
            break;
        case F4SE::MessagingInterface::kGameDataReady:
            REX::INFO("Received kMessage_GameDataReady. Game data is ready.");
            // Get the global data handle and interfaces
            g_dataHandle = RE::TESDataHandler::GetSingleton();
            if (g_dataHandle) {
                REX::INFO("TESDataHandler singleton acquired successfully.");
            } else {
                REX::WARN("Failed to acquire TESDataHandler singleton.");
            }
            break;
        case F4SE::MessagingInterface::kPostLoadGame:
            REX::INFO("Received kMessage_PostLoadGame. A save game has been loaded.");
            break;
        case F4SE::MessagingInterface::kNewGame:
            REX::INFO("Received kMessage_NewGame. A new game has been started.");
            break;
    }
}

// --- F4SE Entry Points - MUST have C linkage for F4SE to find them ---
extern "C"
{ // This block ensures C-style (unmangled) names for the linker

    F4SE_API bool F4SEPlugin_Query(const F4SE::QueryInterface *f4se, F4SE::PluginInfo *info)
    {
        // Set the plugin information
        // This is crucial to load the plugin
        info->infoVersion = F4SE::PluginInfo::kVersion;
        info->name = Version::PROJECT.data();
        info->version = Version::MAJOR;
        // Set up the logger
        // F4SE::log::log_directory().value(); == Documents/My Games/F4SE/
        std::filesystem::path logPath = F4SE::log::log_directory().value();
        logPath = logPath.parent_path() / "Fallout4" / "F4SE" / std::format("{}.log", Version::PROJECT);
        // Create the file
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
        auto aLog = std::make_shared<spdlog::logger>("aLog"s, sink);
        // Configure the logger
        aLog->set_level(spdlog::level::info);
        aLog->flush_on(spdlog::level::info);
        // Set pattern
        aLog->set_pattern("[%T] [%^%l%$] %v"s);
        // Register to make it global accessable
        spdlog::register_logger(aLog);
        // Assign to global pointer
        gLog = spdlog::get("aLog");
        // First log
        REX::INFO("{}: Plugin Query started.", Version::PROJECT);
        // Minimum version 1.10.163
        const auto ver = f4se->RuntimeVersion();
        if (ver < F4SE::RUNTIME_1_10_162) {
            gLog->critical("unsupported runtime v{}", ver.string());
            return false;
        }
        return true;
    }

    // This function is called after F4SE has loaded all plugins and the game is about to start.
    F4SE_API bool F4SEPlugin_Load(const F4SE::LoadInterface *f4se) {
        // Initialize the plugin with logger false to prevent F4SE to use its own logger
        F4SE::Init(f4se, false);
        // Log information
        REX::INFO("{}: Plugin loaded!", Version::PROJECT);
        REX::INFO("F4SE version: {}", F4SE::GetF4SEVersion().string());
        REX::INFO("Game runtime version: {}", f4se->RuntimeVersion().string());
        // Get the global plugin handle and interfaces
        g_pluginHandle = f4se->GetPluginHandle();
        g_messaging = F4SE::GetMessagingInterface();
        g_papyrus = F4SE::GetPapyrusInterface();
        // Get the DLL handle for this plugin
        HMODULE hModule = GetModuleHandleA("InvLockerCL.dll");
        // Load config
        LoadConfig(hModule);
        // Register Papyrus functions
        if (g_papyrus) {
            g_papyrus->Register(RegisterPapyrusFunctions);
            REX::INFO("Papyrus functions registration callback successfully registered.");
        } else {
            REX::WARN("Failed to register Papyrus functions. This is critical for native functions.");
        }
        // Inject our Container Menu Hooks
        if (InstallContainerMenuHooks()) {
            REX::INFO("Successfully installed ContainerMenu hooks.");
        } else {
            REX::WARN("Failed to install ContainerMenu hooks.");
        }
        // Set the messagehandler to listen to events
        if (g_messaging && g_messaging->RegisterListener(F4SEMessageHandler, "F4SE")) {
            REX::INFO("Registered F4SE message handler.");
        } else {
            REX::WARN("Failed to register F4SE message handler.");
            return false;
        }
        // Get the task interface
        g_taskInterface = F4SE::GetTaskInterface();
        // Get the scaleform interface
        g_scaleformInterface = F4SE::GetScaleformInterface();
        return true;
    }

    F4SE_API void F4SEPlugin_Release() {
        // This is a new function for cleanup. It is called when the plugin is unloaded.
        REX::INFO("%s: Plugin released.", Version::PROJECT);
        gLog->flush();
        spdlog::drop_all();
    }
}