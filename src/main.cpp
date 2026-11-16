#include <Arduino.h>
#include <WiFi.h>
#include <driver/i2s.h> // Native ESP32 hardware I2S driver

const char* ssid = "THK_EXT";
const char* password = "thk_8794";
WiFiServer server(80);

// I2S Pin Definitions for ESP32-S3
#define I2S_SCK 4
#define I2S_WS  5
#define I2S_SD  6
#define I2S_PORT I2S_NUM_0

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // CRITICAL: Drops right channel at the hardware level, ensuring perfect mono timing.
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,  // Ideal hardware ring buffers to absorb Wi-Fi latency spikes
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  Serial.println(WiFi.localIP());

  setupI2S();
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  
  if (client) {
    String req = "";
    while (client.connected() && client.available()) {
      req += (char)client.read();
      if (req.endsWith("\r\n\r\n")) break; 
    }

    if (req.indexOf("favicon.ico") > 0) {
      client.print("HTTP/1.1 404 Not Found\r\n\r\n");
      client.stop();
      return; 
    }

    // HTTP headers for live audio streaming
    client.print("HTTP/1.1 200 OK\r\nContent-Type: audio/x-wav\r\nConnection: close\r\n\r\n");
    byte wavHeader[44] = {
      'R','I','F','F', 0xFF, 0xFF, 0xFF, 0xFF, 'W','A','V','E',
      'f','m','t',' ', 16, 0, 0, 0, 1, 0, 1, 0, 
      0x80, 0x3E, 0x00, 0x00, 0x00, 0x7D, 0x00, 0x00, 
      2, 0, 16, 0, 'd','a','t','a', 0xFF, 0xFF, 0xFF, 0xFF
    };
    client.write(wavHeader, 44);

    int32_t raw_samples[512];
    int16_t final_samples[512];
    size_t bytes_read = 0;

    // Continuous streaming loop
    while (client.connected()) {
      // Read raw audio directly from DMA (Zero software delay, auto-overwrites oldest packets if backed up)
      i2s_read(I2S_PORT, &raw_samples, sizeof(raw_samples), &bytes_read, portMAX_DELAY);
      
      int samples_read = bytes_read / 4; // Divide by 4 to get the exact count of 32-bit samples
      
      // Convert 32-bit raw data to 16-bit WAV format (Applies hardware gain/volume adjustment)
      for (int i = 0; i < samples_read; i++) {
        final_samples[i] = raw_samples[i] >> 14; 
      }
      
      // Transmit the processed chunk to the browser in a single write operation
      client.write((uint8_t*)final_samples, samples_read * 2);
    }
  }
}