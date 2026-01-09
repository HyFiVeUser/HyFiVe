#ifndef SDCARD_SERVICE_H
#define SDCARD_SERVICE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <functional>
#include "ble_uuids.h"

// ===== Commands =====
#define CMD_TOTAL_COUNT   0x00
#define CMD_NEW_COUNT   0x01
#define CMD_ALL_RANGES  0x02
#define CMD_UPLOAD_ALL_DATA  0x03

// ===== Transfer-Paket =====
struct Transfer_Packet_t {
  struct {
    uint16_t total_num_packets = 1;
    uint16_t whole_packet_checksum = 0xDEAD;
  } __attribute__((packed)) metadata;

  struct {
    uint16_t index = 0;
    uint16_t checksum = 0xDEAD;
    uint16_t payload_size = 0;
    uint8_t  payload[502] = {};
  } __attribute__((packed)) chunk;
} __attribute__((packed));

// ===== Ack status =====
enum class AckStatus { IDLE, PENDING, ACKNOWLEDGED, TIMEOUT };

// ===== Callback types =====
typedef std::function<int()> GetCountCallback;
typedef std::function<String(int start, int end, bool isNew)> GetDataCallback;

class SDCardService {
public:
  SDCardService();
  ~SDCardService();

  bool begin(NimBLEServer* server);

  // App-Callbacks
  void setGetAllCountCallback(GetCountCallback cb);
  void setGetNewCountCallback(GetCountCallback cb);
  void setGetDataCallback(GetDataCallback cb);

  bool isInitialized() const { return _isInitialized; }
  static const char* getServiceUUID() { return SD_CARD_SERVICE_UUID; }

  // Ack/Transfer
  AckStatus getAckStatus() const { return _ackStatus; }
  void      resetAckStatus() { _ackStatus = AckStatus::IDLE; }
  bool      checkAckTimeout();

  // Call from loop()/task
  void tick();

private:
  NimBLEService*        _service;
  NimBLECharacteristic* _characteristic;
  bool                  _isInitialized;

  // Callbacks
  GetCountCallback _getAllCountCallback;
  GetCountCallback _getNewCountCallback;
  GetDataCallback  _getDataCallback;

  // Ack/Transfer
  volatile AckStatus     _ackStatus;
  volatile unsigned long _lastIndicationTime;
  volatile uint16_t      _pendingConnHandle;
  static constexpr unsigned long ACK_TIMEOUT_MS = 5000;

  volatile bool     _txActive;
  
volatile bool _txAdvanceRequested = false;
  volatile uint16_t _txNextIndex;
  volatile uint16_t _txTotalChunks;
  volatile uint16_t _txMaxChunkSize;
  volatile uint16_t _txConnHandle;
  String            _txPayload;

  // Pending command (moved out of onWrite)
  volatile bool     _hasPendingCommand;
  volatile uint8_t  _pendingCommand;
  volatile uint16_t _pendingCmdHandle;
  String            _pendingPayload;

  uint16_t calcChecksum(const uint8_t* buf, uint32_t len);
  bool     sendCurrentChunk();
  void     sendChunkedResponse(String response, uint16_t connHandle);
  bool     sendToApp(String response, uint16_t connHandle);
  void     processCommand(uint8_t command, uint16_t connHandle);

  class CommandHandler : public NimBLECharacteristicCallbacks {
  public:
    CommandHandler(SDCardService* service) : _service(service) {}
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
    void onRead (NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
    void onStatus(NimBLECharacteristic* pCharacteristic, int code) override;
    void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override;
  private:
    SDCardService* _service;
  };
  friend class CommandHandler;

  void enqueueCommand(uint8_t command, uint16_t connHandle);
};

#endif // SDCARD_SERVICE_H