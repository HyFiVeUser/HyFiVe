#include "SDCardService.h"
#include "logging.h"
#include "power.h"
#include "measurement_storage.h"

// Globale Busy-Flags (Definition in ble_sd_card.cpp)
extern bool bleSDCardTransferActive;

SDCardService::SDCardService()
    : _service(nullptr),
      _characteristic(nullptr),
      _isInitialized(false),
      _ackStatus(AckStatus::IDLE),
      _lastIndicationTime(0),
      _pendingConnHandle(BLE_HS_CONN_HANDLE_NONE),
      _txActive(false),
      _txNextIndex(0),
      _txTotalChunks(0),
      _txMaxChunkSize(160), //! 160
      _txConnHandle(BLE_HS_CONN_HANDLE_NONE),
      _hasPendingCommand(false),
      _pendingCommand(0),
      _pendingCmdHandle(BLE_HS_CONN_HANDLE_NONE)
{
}

SDCardService::~SDCardService() {}

bool SDCardService::begin(NimBLEServer *server)
{
  if (!server)
  {
    LOG_E("[SD BLE] No server provided");
    return false;
  }
  LOG_I("[SD BLE] Initializing service...");
  _service = server->createService(SD_CARD_SERVICE_UUID);
  if (!_service)
    return false;

  _characteristic = _service->createCharacteristic(
      SD_CARD_CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::INDICATE);
  if (!_characteristic)
    return false;

  _characteristic->setCallbacks(new CommandHandler(this));
  _service->start();
  _isInitialized = true;
  LOG_I("[SD BLE] Ready");
  return true;
}

void SDCardService::setGetAllCountCallback(GetCountCallback cb) { _getAllCountCallback = cb; }
void SDCardService::setGetNewCountCallback(GetCountCallback cb) { _getNewCountCallback = cb; }
void SDCardService::setGetDataCallback(GetDataCallback cb) { _getDataCallback = cb; }

uint16_t SDCardService::calcChecksum(const uint8_t *buf, uint32_t len)
{
  (void)buf;
  (void)len;
  return 0xDEAD;
}

bool SDCardService::checkAckTimeout()
{
  if (_ackStatus == AckStatus::PENDING)
  {
    unsigned long elapsed = millis() - _lastIndicationTime;
    if (elapsed > ACK_TIMEOUT_MS)
    {
      LOG_W("[SD BLE] ACK Timeout");
      _ackStatus = AckStatus::TIMEOUT;
      _txActive = false;
      _pendingConnHandle = BLE_HS_CONN_HANDLE_NONE;
      bleSDCardTransferActive = false;
      LOG_W("[BLE SD] Transfer flag reset (ACK timeout)");
      return true;
    }
  }
  return false;
}

bool SDCardService::sendCurrentChunk()
{
  if (!_txActive || !_characteristic)
    return false;
  if (_txNextIndex >= _txTotalChunks)
    return true;

  const uint32_t offset = _txNextIndex * _txMaxChunkSize;
  const uint16_t remaining = (_txPayload.length() > offset) ? (_txPayload.length() - offset) : 0;
  const uint16_t chunkSize = (remaining >= _txMaxChunkSize) ? _txMaxChunkSize : remaining;

  Transfer_Packet_t packet;
  memset(&packet, 0, sizeof(packet));
  packet.metadata.total_num_packets = _txTotalChunks;
  packet.metadata.whole_packet_checksum = calcChecksum((const uint8_t *)_txPayload.c_str(), _txPayload.length());
  packet.chunk.index = _txNextIndex;
  packet.chunk.payload_size = chunkSize;
  memcpy(packet.chunk.payload, _txPayload.c_str() + offset, chunkSize);
  packet.chunk.checksum = calcChecksum(packet.chunk.payload, chunkSize);

  const uint16_t dynamic_len =
      sizeof(packet.metadata) +
      sizeof(packet.chunk.index) +
      sizeof(packet.chunk.checksum) +
      sizeof(packet.chunk.payload_size) +
      packet.chunk.payload_size;

  _characteristic->setValue((uint8_t *)&packet, dynamic_len);

  int retries = 15;
  while (retries-- > 0)
  {
    bool success = (_txConnHandle != BLE_HS_CONN_HANDLE_NONE)
                       ? _characteristic->indicate(_txConnHandle)
                       : _characteristic->indicate();
    if (success)
    {
      _ackStatus = AckStatus::PENDING;
      _lastIndicationTime = millis();
      return true;
    }
    vTaskDelay(60 / portTICK_PERIOD_MS);
  }
  _txActive = false;
  _pendingConnHandle = BLE_HS_CONN_HANDLE_NONE;
  _ackStatus = AckStatus::TIMEOUT;
  bleSDCardTransferActive = false;
  LOG_E("[BLE SD] Chunk transmission failed, transfer flag reset");
  return false;
}

static uint16_t maxPayloadForPeer(uint16_t connHandle)
{
  // ATT data per indication: (MTU - 3)
  // If your NimBLE-Arduino version supports getMTU(connHandle), use the handle-aware variant.
  uint16_t mtu =
#if 0 // set to 1 when getMTU(connHandle) is available
    NimBLEDevice::getMTU(connHandle);
#else
      NimBLEDevice::getMTU();
#endif

  const uint16_t attBytes = (mtu > 3) ? (mtu - 3) : 0;
  const uint16_t pktHeader = 10; // Transfer_Packet_t header overhead
  uint16_t maxPayload = (attBytes > pktHeader) ? (attBytes - pktHeader) : 0;

  if (maxPayload >= 502)
    return 502; // large chunk (only if MTU is high enough)
  if (maxPayload >= 242)
    return 242;      // safely fits in one LL-PDU
  return maxPayload; // e.g., ~182 on iOS
}

void SDCardService::sendChunkedResponse(String response, uint16_t connHandle)
{
  if (_txActive)
    _txActive = false;
  _txPayload = response;
  _txConnHandle = connHandle;
  //_txMaxChunkSize = 480; //!160
  _txMaxChunkSize = maxPayloadForPeer(connHandle);

  const uint16_t num_full = _txPayload.length() / _txMaxChunkSize;
  const uint16_t last_size = _txPayload.length() % _txMaxChunkSize;
  _txTotalChunks = num_full + (last_size ? 1 : 0);
  _txNextIndex = 0;
  _txActive = (_txTotalChunks > 0);

  if (_txActive)
  {
    if (!sendCurrentChunk())
    {
      LOG_E("[BLE SD] Chunk transfer start failed");
    }
  }
  else
  {
    bleSDCardTransferActive = false;
    LOG_W("[BLE SD] No data for chunk transfer, transfer flag reset");
  }
}

bool SDCardService::sendToApp(String response, uint16_t connHandle)
{
  constexpr size_t MAX_SINGLE_SIZE = 160; //! 160

  LOG_I("[BLE SD] send len=%u (single=%s)", response.length(), response.length() <= 160 ? "yes" : "no"); //! 160

  if (response.length() <= MAX_SINGLE_SIZE)
  {
    Transfer_Packet_t packet;
    memset(&packet, 0, sizeof(packet));
    packet.metadata.total_num_packets = 1;
    packet.metadata.whole_packet_checksum = calcChecksum((uint8_t *)response.c_str(), response.length());
    packet.chunk.index = 0;
    packet.chunk.payload_size = response.length();
    memcpy(packet.chunk.payload, response.c_str(), response.length());
    packet.chunk.checksum = calcChecksum((uint8_t *)response.c_str(), response.length());

    const uint16_t dynamic_len =
        sizeof(packet.metadata) +
        sizeof(packet.chunk.index) +
        sizeof(packet.chunk.checksum) +
        sizeof(packet.chunk.payload_size) +
        packet.chunk.payload_size;

    _characteristic->setValue((uint8_t *)&packet, dynamic_len);

    int retries = 15;
    while (retries-- > 0)
    {
      bool success = (connHandle != BLE_HS_CONN_HANDLE_NONE)
                         ? _characteristic->indicate(connHandle)
                         : _characteristic->indicate();
      if (success)
      {
        _ackStatus = AckStatus::PENDING;
        _lastIndicationTime = millis();
        _pendingConnHandle = connHandle;
        return true;
      }
      vTaskDelay(60 / portTICK_PERIOD_MS);
    }
    _ackStatus = AckStatus::TIMEOUT;
    _pendingConnHandle = BLE_HS_CONN_HANDLE_NONE;
    bleSDCardTransferActive = false;
    LOG_E("[BLE SD] Indication failed, transfer flag reset");
    return false;
  }
  else
  {
    sendChunkedResponse(response, connHandle);
    return true;
  }
}

void SDCardService::processCommand(uint8_t command, uint16_t connHandle)
{
  String response;
  LOG_I("[BLE SD] process cmd=0x%02X", command);
  switch (command)
  {
  case CMD_ALLE_ANZAHL:
  {
    response += (char)CMD_ALLE_ANZAHL;
    uint32_t measured = 0;
    uint32_t transmitted = 0;
    getMeasurementIndexCounts(measured, transmitted);

    response += String(measured);
    break;
  }
  case CMD_NEUE_ANZAHL:
  {
    response += (char)CMD_NEUE_ANZAHL;
    uint32_t measured = 0;
    uint32_t transmitted = 0;

    getMeasurementIndexCounts(measured, transmitted);

    uint32_t pending = (measured > transmitted) ? (measured - transmitted) : 0;
    response += String(pending);
    break;
  }
  case CMD_ALLE_BEREICH:
  {
    // Leading byte for the CLI
    response += (char)CMD_ALLE_BEREICH;

    // Parse range from the most recently received payload
    int start = 0, end = 59;
    if (_pendingPayload.length())
    {
      int dash = _pendingPayload.indexOf('-');
      if (dash > 0)
      {
        start = _pendingPayload.substring(0, dash).toInt();
        end = _pendingPayload.substring(dash + 1).toInt();
      }
    }

    if (start < 0)
      start = 0;
    if (end < start)
      end = start;

    // Clamp bounds based on measured count
    uint32_t measured = 0, transmitted = 0;
    bool haveCounts = getMeasurementIndexCounts(measured, transmitted);
    if (haveCounts && (int)measured > 0)
    {
      int maxIdx = (int)measured - 1;
      if (start > maxIdx)
        start = maxIdx;
      if (end > maxIdx)
        end = maxIdx;
      if (start < 0)
        start = 0;
      if (end < start)
        end = start;
    }

    // Provide data
    String data = _getDataCallback(start, end, false);

    if (data == "[]")
    {
      delay(100);
      for (int i = 0; i < 5; i++)
      {
        Serial.println("data");
        Serial.println(data);

        data = _getDataCallback(start, end, false);

        if (data != "[]")
        {
          Serial.println("data");
          Serial.println(data);
          break;
        }
        delay(100);
      }
    }

    response += String(start) + "-" + String(end) + "/" + String(measured);
    response += data;

    // Update transfer counters
    if (haveCounts && (int)measured > 0 && start <= end)
    {
      uint32_t rangeEndPlusOne = (uint32_t)(end + 1);
      if (rangeEndPlusOne > measured)
      {
        rangeEndPlusOne = measured;
      }
      if (rangeEndPlusOne > transmitted)
      {
        int32_t delta = (int32_t)(rangeEndPlusOne - transmitted);
        if (delta > 0)
        {
          updateMeasurementIndex(0, delta);
        }
      }
    }
    break;
  }
  case CMD_NEUE_BEREICH:
  {
    response += (char)CMD_NEUE_BEREICH;

    int requestedStart = 0;
    int requestedEnd = 59;
    if (_pendingPayload.length())
    {
      int dash = _pendingPayload.indexOf('-');
      if (dash > 0)
      {
        requestedStart = _pendingPayload.substring(0, dash).toInt();
        requestedEnd = _pendingPayload.substring(dash + 1).toInt();
      }
    }

    if (requestedStart < 0)
      requestedStart = 0;
    if (requestedEnd < requestedStart)
      requestedEnd = requestedStart;

    uint32_t measured = 0;
    uint32_t transmitted = 0;
    (void)getMeasurementIndexCounts(measured, transmitted);

    const uint32_t availableNew = (measured > transmitted) ? (measured - transmitted) : 0;

    int newStart = requestedStart;
    int newEnd = requestedEnd;

    if (availableNew == 0)
    {
      newStart = 0;
      newEnd = -1;
    }
    else
    {
      if ((uint32_t)newStart >= availableNew)
        newStart = (int)(availableNew - 1);
      if ((uint32_t)newEnd >= availableNew)
        newEnd = (int)(availableNew - 1);
      if (newEnd < newStart)
        newEnd = newStart;
    }

    const bool hasData = (availableNew > 0) && (newStart >= 0) && (newEnd >= newStart);

    const uint32_t transmittedBefore = transmitted;
    const int absStart = hasData ? int(transmittedBefore + (uint32_t)newStart) : int(transmittedBefore);
    const int absEnd = hasData ? int(transmittedBefore + (uint32_t)newEnd) : int(transmittedBefore - 1);

    String data = "[]";
    if (_getDataCallback)
    {
      // Important: read absolute indices, NO onlyNew filter!
      data = _getDataCallback(absStart, absEnd, /*onlyNew=*/false);

      if (data == "[]")
      {
        delay(100);
        for (int i = 0; i < 5; i++)
        {
          Serial.println("data");
          Serial.println(data);

          data = _getDataCallback(absStart, absEnd, /*onlyNew=*/false);

          if (data != "[]")
          {
            Serial.println("data");
            Serial.println(data);
            break;
          }
          delay(100);
        }
      }

      // Update transferredCount (as before, just with an absolute end index)
      if (hasData)
      {
        uint32_t newTransmitted = (uint32_t)(absEnd + 1);
        if (newTransmitted > measured)
          newTransmitted = measured;
        if (newTransmitted > transmittedBefore)
        {
          int32_t delta = (int32_t)(newTransmitted - transmittedBefore);
          if (delta > 0)
          {
            updateMeasurementIndex(0, delta);
          }
        }
      }
    }

    // Output the response header consistently as 0-based like ALLE_BEREICH:
    response += String(absStart) + "-" + String(absEnd) + "/" + String(measured);
    response += data;
    break;
  }
  default:
  {
    response += (char)command;
    response += (char)0x01;
    response += "Unknown command";
    break;
  }
  }
  (void)sendToApp(response, connHandle);
}

void SDCardService::enqueueCommand(uint8_t command, uint16_t connHandle)
{
  _pendingCommand = command;
  _pendingCmdHandle = connHandle;
  _hasPendingCommand = true;
}

void SDCardService::tick()
{
  if (_txActive)
  {
    if (_txAdvanceRequested)
    {
      _txAdvanceRequested = false;
      _txNextIndex++;
      if (_txNextIndex < _txTotalChunks)
      {
        // Taking a short pause helps some stacks
        vTaskDelay(1); // ~1 Tick
        sendCurrentChunk();
      }
      else
      {
        _txActive = false;
        _pendingConnHandle = BLE_HS_CONN_HANDLE_NONE;
        bleSDCardTransferActive = false;
        LOG_I("[BLE SD] Transfer complete (all chunks sent)");
      }
      return;
    }
    checkAckTimeout();
    return;
  }
  if (_hasPendingCommand)
  {
    uint8_t cmd = _pendingCommand;
    uint16_t hdl = _pendingCmdHandle;
    _hasPendingCommand = false;
    processCommand(cmd, hdl);
  }
}

void SDCardService::CommandHandler::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
{
  if (!_service)
    return;
  NimBLEAttValue value = pCharacteristic->getValue();
  if (value.length() == 0)
    return;

  uint8_t command = 0;
  String payloadStr = "";

  if (value.length() >= sizeof(Transfer_Packet_t))
  {
    auto *packet = (Transfer_Packet_t *)value.data();
    if (packet->chunk.payload_size > 0)
    {
      const uint8_t *p = packet->chunk.payload;
      command = p[0];

      // Collect everything from byte 1 to the first '\0' as ASCII
      for (uint16_t i = 1; i < packet->chunk.payload_size; ++i)
      {
        char c = (char)p[i];
        if (c == '\0')
          break;
        payloadStr += c;
      }
    }
  }
  else
  {
    command = value[0];
    for (size_t i = 1; i < value.length(); ++i)
    {
      char c = (char)value[i];
      if (c == '\0')
        break;
      payloadStr += c;
    }
  }

    _service->_pendingPayload = payloadStr; // New: store the range
  LOG_I("[BLE SD] onWrite cmd=0x%02X, payload='%s'", command, payloadStr.c_str());
  bleSDCardTransferActive = true;
  _service->enqueueCommand(command, connInfo.getConnHandle());
}

void SDCardService::CommandHandler::onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
{
  (void)pCharacteristic;
  (void)connInfo;
}

void SDCardService::CommandHandler::onStatus(NimBLECharacteristic *pCharacteristic, int code)
{
  if (!_service)
    return;

  if (code == BLE_HS_EDONE)
  {
    _service->_ackStatus = AckStatus::ACKNOWLEDGED;
    if (_service->_txActive)
    {
      _service->_txAdvanceRequested = true; // <— nur markieren
    }
    else
    {
      _service->_pendingConnHandle = BLE_HS_CONN_HANDLE_NONE;
      bleSDCardTransferActive = false;
      LOG_I("[BLE SD] Transfer complete (single packet)");
    }
    return;
  }
  else if (code == BLE_HS_ETIMEOUT)
  {
    _service->_ackStatus = AckStatus::TIMEOUT;
    _service->_txActive = false;
    _service->_pendingConnHandle = BLE_HS_CONN_HANDLE_NONE;
    bleSDCardTransferActive = false;
    LOG_W("[BLE SD] Transfer aborted (NimBLE timeout)");
  }
  else
  {
    _service->_ackStatus = AckStatus::IDLE;
    _service->_txActive = false;
    _service->_pendingConnHandle = BLE_HS_CONN_HANDLE_NONE;
    bleSDCardTransferActive = false;
    LOG_W("[BLE SD] Unhandled status code %d - transfer flag reset", code);
  }
}

void SDCardService::CommandHandler::onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue)
{
  (void)pCharacteristic;
  (void)connInfo;
  (void)subValue;
}