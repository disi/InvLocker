#include <PCH.h>
#include <Global.h>

const char* defaultIni = R"(
; Enable/disable debugging messages
DEBUGGING=false
; Lock equipped inventory items
LOCK_EQUIPPED=true
; Lock favorite inventory items
LOCK_FAVORITES=true
; Lock equipped and/or favorite inventory items from scrapping
LOCK_SCRAP=true
; Bi-directional locking
LOCK_BIDIRECTIONAL=true
; Lock items when Take All Items is used
LOCK_TAKEALL=true
)";