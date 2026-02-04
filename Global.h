#pragma once
#include <PCH.h>
#include <Plugin.h>

// Global logger pointer
extern std::shared_ptr<spdlog::logger> gLog;

// Global data handler
extern RE::TESDataHandler* g_dataHandle;
// Declare the F4SEMessagingInterface and F4SEScaleformInterface
extern const F4SE::MessagingInterface* g_messaging;
extern const F4SE::ScaleformInterface* g_scaleform;
// Declare the F4SEPapyrusInterface
extern const F4SE::PapyrusInterface* g_papyrus;
// Declare the F4SETaskInterface
extern const F4SE::TaskInterface* g_taskInterface;

// Default ini file
extern const char* defaultIni;

// Global module name
extern std::string g_moduleName;
// Global debug flag
extern bool DEBUGGING;
// Lock equipped inventory items
extern bool LOCK_EQUIPPED;
// Lock favorite inventory items
extern bool LOCK_FAVORITES;
// Lock scrapping of equipped and/or favorite inventory items
extern bool LOCK_SCRAP;
// Bi-directional locking
extern bool LOCK_BIDIRECTIONAL;
// Lock when using Take All Items
extern bool LOCK_TAKEALL;

// Helper function to convert string to lowercase
inline std::string ToLower(const std::string& str) {
    std::string out = str;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

// REX Logging Compatibility
#undef ERROR
namespace REX
{
    template <class... Args> void INFO(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->info(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void WARN(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->warn(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void ERROR(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->error(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void CRITICAL(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->critical(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void DEBUG(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->debug(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void TRACE(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->trace(a_fmt, std::forward<Args>(a_args)...);
    }
} // namespace REX