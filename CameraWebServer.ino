#include "esp_camera.h"
#include <WiFi.h>

#define PWDN_GPIO     32
#define RESET_GPIO    -1
#define XCLK_GPIO      0
#define SIOD_GPIO     26
#define SIOC_GPIO     27
#define Y9_GPIO       35
#define Y8_GPIO      34
#define Y7_GPIO       39
#define Y6_GPIO      36
#define Y5_GPIO       21
#define Y4_GPIO       19
#define Y3_GPIO       18
#define Y2_GPIO        5
#define VSYNC_GPIO    25
#define HREF_GPIO     23
#define PCLK_GPIO     22



// const char* ssid = "PMSCS-661";
// const char* password = "";
const char* ssid = "Hemlodge";
const char* password = "psw#ofhemlodgeAP";

void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO;
  config.pin_d1 = Y3_GPIO;
  config.pin_d2 = Y4_GPIO;
  config.pin_d3 = Y5_GPIO;
  config.pin_d4 = Y6_GPIO;
  config.pin_d5 = Y7_GPIO;
  config.pin_d6 = Y8_GPIO;
  config.pin_d7 = Y9_GPIO;
  config.pin_xclk = XCLK_GPIO;
  config.pin_pclk = PCLK_GPIO;
  config.pin_vsync = VSYNC_GPIO;
  config.pin_href = HREF_GPIO;
  config.pin_sscb_sda = SIOD_GPIO;
  config.pin_sscb_scl = SIOC_GPIO;
  config.pin_pwdn = PWDN_GPIO;
  config.pin_reset = RESET_GPIO;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera initialization failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initially sensors are vertically flipped and colors are slightly saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flipping it back
    s->set_brightness(s, 1); // slightly increase the brightness
    s->set_saturation(s, -2); // decrease the saturation
  }
  s->set_framesize(s, FRAMESIZE_QVGA); // Decreasing frame size for higher initial frame rate
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(555);
    Serial.print("-");
  }
  Serial.println("");
  Serial.println("WiFi connection established");

  startCameraServer();

  Serial.print("Camera is running! Plesae use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' for connection");
}

void loop() {
  //delay(10000);
}
