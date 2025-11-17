#include "ble_server.h"
#include "SDCardService.h"
#include "sd_csv_backend.h"
#include "logging.h"

// Adapter to the existing API
bool bleSDCardTransferActive = false;

static SDCardService sdCardBleService;
static TaskHandle_t sdCardServiceTickTaskHandle = nullptr;

static void sdCardServiceTickTask(void* parameter) {
  (void)parameter;
  for (;;) {
    sdCardBleService.tick();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

bool initSDCardService() {
  if (!bleServerInstance || !bleServerInitialized) {
    LOG_E("[BLE SD] No BLE server");
    return false;
  }
  if (!sd_csv_begin()) {
    LOG_W("[BLE SD] SD CSV init failed");
    // do not abort: service can still start; app checks the count
  }
  if (!sdCardBleService.begin(bleServerInstance)) {
    LOG_E("[BLE SD] Service begin failed");
    return false;
  }

  // Register CSV callbacks
  sdCardBleService.setGetAllCountCallback(sd_csv_count_all);
  sdCardBleService.setGetNewCountCallback(sd_csv_count_new);
  sdCardBleService.setGetDataCallback(sd_csv_get_data);

  if (!sdCardServiceTickTaskHandle) {
    xTaskCreatePinnedToCore(sdCardServiceTickTask, "sd_tick", 4096, nullptr, 2, &sdCardServiceTickTaskHandle, 0); //! Core switched from 1 to 0
  }
  LOG_I("[BLE SD] SDCardService ready");
  return true;
}

bool startSDCardService() {
  // Service was started in begin(); nothing else needed here
  return true;
}

void stopSDCardService() {
  // Minimal option: stopping the task would be nice-to-have; omitted for simplicity
}

bool isSDCardTransferActive() {
  return (sdCardBleService.getAckStatus() == AckStatus::PENDING);
}

void printBLESDCardServiceStatus() {
  LOG_I("\n=== BLE SDCard Status ===");
  LOG_I("Initialized: %s", sdCardBleService.isInitialized() ? "yes" : "no");
  LOG_I("CSV count(all): %d", sd_csv_count_all());
  LOG_I("=========================\n");
}