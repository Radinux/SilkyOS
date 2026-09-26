#include <esp_system.h>
#include "Diag.h"

const char *diagRaisonReset() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return "Mise sous tension";
    case ESP_RST_SW:       return "Redemarrage logiciel";
    case ESP_RST_PANIC:    return "PANIC (crash)";
    case ESP_RST_INT_WDT:  return "Watchdog interruption";
    case ESP_RST_TASK_WDT: return "Watchdog tache";
    case ESP_RST_WDT:      return "Autre watchdog";
    case ESP_RST_BROWNOUT: return "BROWNOUT (tension)";
    default:               return "Inconnue";
  }
}