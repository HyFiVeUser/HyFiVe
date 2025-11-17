#include "sd_csv_backend.h"
#include <SD.h>
#include <SPI.h>
#include "logging.h"

static String s_sd_csv_path = SD_CSV_PATH;

static bool s_sd_ok = false;
static uint32_t s_firstDataOffset = UINT32_MAX;
static uint32_t s_rowBytes        = 0;
static bool     s_layoutOk        = false;

static void resetLayout() {
  s_layoutOk        = false;
  s_rowBytes        = 0;
  s_firstDataOffset = UINT32_MAX;
}

static void skipHeader(File &f) {
  while (f.available()) {
    int b = f.read();
    if (b < 0) break;
    if (b == 10) break; // LF
  }
}

static bool sd_csv_init_layout(File &f) {
  if (s_layoutOk && s_rowBytes > 0 && s_firstDataOffset != UINT32_MAX) {
    return true;
  }

  resetLayout();

  if (!f.seek(0)) return false;

  // Optional BOM handling
  int first = f.peek();
  if (first == 0xEF) {
    int b0 = f.read();
    int b1 = f.read();
    int b2 = f.read();
    if (!(b0 == 0xEF && b1 == 0xBB && b2 == 0xBF)) {
      f.seek(0);
    }
  }

  // Skip header
  while (f.available()) {
    int b = f.read();
    if (b < 0) break;
    if (b == '\n') break;
  }
  s_firstDataOffset = f.position();

  const uint32_t pos0 = s_firstDataOffset;
  String firstLine = f.readStringUntil('\n');
  const uint32_t pos1 = f.position();
  if (pos1 <= pos0) {
    resetLayout();
    return false;
  }
  s_rowBytes = pos1 - pos0;
  if (s_rowBytes == 0) {
    resetLayout();
    return false;
  }

  const uint32_t pos2 = f.position();
  if (f.available()) {
    String secondLine = f.readStringUntil('\n');
    const uint32_t pos3 = f.position();
    if (secondLine.length() > 0 && (pos3 - pos2) != s_rowBytes) {
      resetLayout();
      return false;
    }
  }

  s_layoutOk = true;
  return true;
}

static bool readLine(File &f, String &out) {
  out = "";
  while (f.available()) {
    int b = f.read();
    if (b < 0) break;
    if (b == 13) continue; // CR
    if (b == 10) break;    // LF
    out += (char)b;
  }
  return out.length() > 0 || f.available();
}

static bool csvLineToJson(const String &line, String &out) {
  String t = line;
  t.trim();
  if (!t.length()) return false;

  // parse columns (a legacy paramUuid column may still be present)
  String col[7];
  int ci = 0;
  int from = 0;
  for (; ci < 6; ++ci) {
    int c = t.indexOf(',', from);
    if (c < 0) break;
    col[ci] = t.substring(from, c);
    from = c + 1;
  }
  col[ci++] = t.substring(from);
  if (ci != 7 && ci != 6) return false;

  String ts = col[0];
  ts.trim();
  const bool hasParamUuid = (ci == 7);
  const int baseIndex = hasParamUuid ? 2 : 1;

  String id = col[baseIndex + 0];
  id.trim();
  String lat = col[baseIndex + 1];
  lat.trim();
  String lng = col[baseIndex + 2];
  lng.trim();
  String nval = col[baseIndex + 3];
  nval.trim();
  String sval = col[baseIndex + 4];
  sval.trim();

  out = "{";
  out += "\"timestamp\":\"" + ts + "\",";
  out += "\"paramUuid\":\"\",";
  out += "\"id\":" + id + ",";
  out += "\"lat\":" + lat + ",";
  out += "\"lng\":" + lng + ",";
  out += "\"nvalue\":" + nval + ",";
  out += "\"svalue\":" + sval;
  out += "}";
  return true;
}

// CSV: timestamp,id,lat,lng,nvalue,svalue
int sd_csv_count_internal(bool onlyNew) {
  (void)onlyNew; // without an "old" flag all entries are considered new
  if (!s_sd_ok) return 0;
  File f = SD.open(s_sd_csv_path.c_str(), FILE_READ);
  if (!f) return 0;
  skipHeader(f);
  int cnt = 0;
  String line;
  while (readLine(f, line)) {
    String t = line; t.trim();
    if (!t.length()) continue;
    cnt++;
  }
  f.close();
  return cnt;
}

bool sd_csv_begin() {
  if (s_sd_ok) return true;
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  constexpr uint8_t SD_INIT_MAX_RETRIES = 5;
  bool sdReady = false;

  for (uint8_t attempt = 0; attempt < SD_INIT_MAX_RETRIES; ++attempt) {
    if (SD.begin(SD_CS_PIN)) {
      sdReady = true;
      break;
    }

    LOG_W("[SD CSV] init failed (try %u/%u)", attempt + 1, SD_INIT_MAX_RETRIES);
    SD.end();
    digitalWrite(SD_CS_PIN, HIGH);
    delay(50);
  }

  if (!sdReady) {
    LOG_E("[SD CSV] init failed");
    return false;
  }
  if (!SD.exists(s_sd_csv_path.c_str())) {
    LOG_W("[SD CSV] not found: %s", s_sd_csv_path.c_str());
    return false;
  }
  s_sd_ok = true;
  LOG_I("[SD CSV] ready: %s", s_sd_csv_path.c_str());
  return true;
}

void sd_csv_set_path(const String &path) {
  if (path.length()) {
    s_sd_csv_path = path; //! must be reset back to path later
  } else {
    s_sd_csv_path = SD_CSV_PATH;
  }
  resetLayout();
  if (SD.exists(s_sd_csv_path.c_str())) {
    if (!s_sd_ok) {
      LOG_I("[SD CSV] backend ready after path update");
    }
    s_sd_ok = true;
  } else {
    s_sd_ok = false;
    LOG_W("[SD CSV] file not found: %s", s_sd_csv_path.c_str());
  }
  LOG_I("[SD CSV] path updated: %s", s_sd_csv_path.c_str());
}

int sd_csv_count_all() { return sd_csv_count_internal(false); }
int sd_csv_count_new() { return sd_csv_count_internal(true); }

String sd_csv_get_data(int start, int end, bool onlyNew) {
  (void)onlyNew; // compatibility: without "old" flag no filtering is required
  if (!s_sd_ok) return "[]";
  File f = SD.open(s_sd_csv_path.c_str(), FILE_READ);
  if (!f) return "[]";
  String json = "[";
  bool first = true;

  if (start < 0 || end < start) {
    f.close();
    json += "]";
    return json;
  }

  bool layoutReady = sd_csv_init_layout(f);
  if (layoutReady) {
    uint64_t offset = static_cast<uint64_t>(s_firstDataOffset) +
                      static_cast<uint64_t>(start) * static_cast<uint64_t>(s_rowBytes);
    if (!f.seek(static_cast<uint32_t>(offset))) {
      layoutReady = false;
    } else {
      const int linesToRead = end - start + 1;
      for (int i = 0; i < linesToRead; ++i) {
        String line;
        line.reserve(s_rowBytes);
        int got = 0;
        while (got < static_cast<int>(s_rowBytes) && f.available()) {
          int b = f.read();
          if (b < 0) break;
          line += static_cast<char>(b);
          got++;
        }
        if (got == 0) break;
        while (line.endsWith("\n") || line.endsWith("\r")) {
          line.remove(line.length() - 1);
        }

        String jsonLine;
        if (!csvLineToJson(line, jsonLine)) continue;
        if (!first) json += ",";
        first = false;
        json += jsonLine;
      }
    }
  }

  if (!layoutReady) {
    f.seek(0);
    skipHeader(f);
    int selectedIdx = -1; // index innerhalb gefilterter Menge
    String line;
    while (readLine(f, line)) {
      String jsonLine;
      if (!csvLineToJson(line, jsonLine)) continue;

      selectedIdx++;
      if (selectedIdx < start) continue;
      if (selectedIdx > end) break;

      if (!first) json += ",";
      first = false;
      json += jsonLine;
    }
  }

  f.close();
  json += "]";
  return json;
}