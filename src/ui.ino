// Arduino-TFT_eSPI board-template main routine. There's a TFT_eSPI create+flush driver already in LVGL-9.1 but we create our own here for more control (like e.g. 16-bit color swap).
#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>
#include <XPT2046_Touchscreen.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Ticker.h>
#include <WiFi.h>
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define ROTATION 3 // 0=landscape, 1=portrait, 2=landscape flipped, 3=portrait flipped
#define SEALEVELPRESSURE_HPA (1013.25)
#define I2C_SDA 22
#define I2C_SCL 27
TwoWire I2CBME = TwoWire(0);
// Set up the I2C bus for BME280
Adafruit_BME280 bme; // I2C
const char *ApSSID = "CYD_AP";
SPIClass touchscreenSpi = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
#include <Preferences.h>

Preferences preferences;
uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

/*Don't forget to set Sketchbook location in File/Preferences to the path of your UI project (the parent foder of this INO file)*/

/*Change to your screen resolution*/
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

enum
{
  SCREENBUFFER_SIZE_PIXELS = screenWidth * screenHeight / 10
};
static lv_color_t buf[SCREENBUFFER_SIZE_PIXELS];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf)
{
  Serial.printf(buf);
  Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  if (LV_COLOR_16_SWAP)
  {
    size_t len = lv_area_get_size(area);
    lv_draw_sw_rgb565_swap(pixelmap, len);
  }

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)pixelmap, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  if (touchscreen.touched())
  {
    TS_Point p = touchscreen.getPoint();
    // Some very basic auto calibration so it doesn't go out of range
    if (p.x < touchScreenMinimumX)
      touchScreenMinimumX = p.x;
    if (p.x > touchScreenMaximumX)
      touchScreenMaximumX = p.x;
    if (p.y < touchScreenMinimumY)
      touchScreenMinimumY = p.y;
    if (p.y > touchScreenMaximumY)
      touchScreenMaximumY = p.y;
    // Map this to the pixel position
    data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 1, screenWidth);  /* Touchscreen X calibration */
    data->point.y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 1, screenHeight); /* Touchscreen Y calibration */
    data->state = LV_INDEV_STATE_PRESSED;
    /*
    Serial.print("Touch x ");
    Serial.print(data->point.x);
    Serial.print(" y ");
    Serial.println(data->point.y);
    */
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/*Set tick routine needed for LVGL internal timings*/
static uint32_t my_tick_get_cb(void) { return millis(); }

void readBME280()
{
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F; // Convert Pa to hPa

  Serial.printf("Temperature: %.2f °C, Humidity: %.2f %%, Pressure: %.2f hPa\n", temperature, humidity, pressure);

  char buffer[32];

  snprintf(buffer, sizeof(buffer), "%.2f °C", temperature);
  lv_label_set_text(ui_tempHome, buffer);

  snprintf(buffer, sizeof(buffer), "%.2f %%", humidity);
  lv_label_set_text(ui_HumHome, buffer);

  snprintf(buffer, sizeof(buffer), "%.2f hPa", pressure);
  lv_label_set_text(ui_pressHome, buffer);
}
void InitWifi(lv_event_t *e)
{
  WiFi.mode(WIFI_STA);                                  // Set WiFi mode to Station
  const char *ssid = lv_textarea_get_text(ui_SSID);     // Get SSID from the UI text area
  const char *password = lv_textarea_get_text(ui_PASS); // Get password from the UI text area
  Serial.printf("Connecting to WiFi SSID: %s, Password: %s\n", ssid, password);
  WiFi.begin(ssid, password); // Connect to the WiFi network
  Serial.println("Initializing WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    attempts++;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected");
    lv_label_set_text(ui_WifiMode, "Disconnected");
    lv_img_set_src(ui_uiwifiIcon, &ui_img_376556310);
  }
  else
  {
    String wifi = WiFi.SSID();
    lv_label_set_text(ui_WifiMode, wifi.c_str());
    lv_img_set_src(ui_uiwifiIcon, &ui_img_wifi_png);
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", password);
    preferences.end();
    Serial.println("Connected to WiFi.");
  }

  // Optionally, you can set up a WiFiManager here if you want to allow users to configure WiFi settings
  // WiFiManager wifiManager; // WiFi manager for easy WiFi setup
  // wifiManager.setConfigPortalBlocking(false); // Set to false to avoid blocking the main loop
  // wifiManager.setTimeout(30); // Set a timeout for the config portal
  //   bool res;
  // // res = wm.autoConnect(); // auto generated AP name from chipid
  // // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  // res = wifiManager.autoConnect(ApSSID); // password protected ap

  // if (!res)
  // {
  //   Serial.println("Failed to connect");
  //   lv_label_set_text(ui_WifiMode, ApSSID);
  //   lv_img_set_src(ui_uiwifiIcon, &ui_img_376556310);
  //   // ESP.restart();
  // }
  // else
  // {
  //   // if you get here you have connected to the WiFi
  //   String wifi = WiFi.SSID();
  //   lv_label_set_text(ui_WifiMode, wifi.c_str());
  //   lv_img_set_src(ui_uiwifiIcon, &ui_img_wifi_png);
  //   Serial.println("connected...yeey :)");
  // }
}
// bool chceckWifi()
// {
//   if (WiFi.status() != WL_CONNECTED)
//   {
//     Serial.println("WiFi not connected");
//     lv_label_set_text(ui_WifiMode, "No WiFi");
//     lv_img_set_src(ui_uiwifiIcon, &ui_img_376556310);
//     return false;
//   }
//   else
//   {
//     String wifi = WiFi.SSID();
//     lv_label_set_text(ui_WifiMode, wifi.c_str());
//     lv_img_set_src(ui_uiwifiIcon, &ui_img_wifi_png);
//     Serial.println("Connected to WiFi.");
//     return true;
//   }
// }

Ticker bmeTicker(readBME280, 6E5, 0, MILLIS); // Call readBME280 every second
// Ticker wifiTicker(chceckWifi, 1E5, 0, MILLIS); // Call chceckWifi every second
void setup()
{
  Serial.begin(9600); /* prepare for possible serial debug */
  I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
  WiFi.mode(WIFI_STA);
  // Initialize the BME280 sensor
  if (!bme.begin(0x76, &I2CBME))
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
  }
  else
  {
    bmeTicker.start(); // Start the ticker to read BME280 data every 10 seconds
    Serial.println("BME280 sensor initialized successfully.");
  }
  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println(LVGL_Arduino);
  Serial.println("I am LVGL_Arduino");
  touchscreenSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS); /* Start second SPI bus for touchscreen */
  touchscreen.begin(touchscreenSpi);                                         /* Touchscreen init */
  touchscreen.setRotation(ROTATION);                                         /* Inverted landscape orientation to match screen */

  lv_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  tft.begin();               /* TFT init */
  tft.setRotation(ROTATION); /* Landscape orientation, flipped */

  static lv_disp_t *disp;
  disp = lv_display_create(screenWidth, screenHeight);
  lv_display_set_buffers(disp, buf, NULL, SCREENBUFFER_SIZE_PIXELS * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);

  static lv_indev_t *indev;
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  lv_tick_set_cb(my_tick_get_cb);
  ui_init();

  Serial.println("Setup done");
  lv_obj_add_event_cb(ui_done, InitWifi, LV_EVENT_CLICKED, NULL);

  // wifiTicker.start(); // Start the ticker to check WiFi status every 10 seconds
  preferences.begin("wifi-config", false);
  String saved_ssid = preferences.getString("ssid", "");
  String saved_pass = preferences.getString("pass", "");

  WiFi.begin(saved_ssid, saved_pass); // Set WiFi mode to Station
  Serial.printf("Connecting to WiFi SSID: %s, Password: %s\n", saved_ssid.c_str(), saved_pass.c_str());
  preferences.end();
   int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    attempts++;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected");
    lv_label_set_text(ui_WifiMode, "Disconnected");
    lv_img_set_src(ui_uiwifiIcon, &ui_img_376556310);
  }
  else
  {
    Serial.println("Connected to WiFi.");
    String wifi = WiFi.SSID();
    lv_label_set_text(ui_WifiMode, wifi.c_str());
    lv_img_set_src(ui_uiwifiIcon, &ui_img_wifi_png);
  }

  readBME280(); // Read BME280 data on startup
}

void loop()
{
  lv_timer_handler(); /* let the GUI do its work */
  bmeTicker.update(); // Update the ticker to read BME280 data
  if (WiFi.status() != WL_CONNECTED)
  {
    lv_label_set_text(ui_WifiMode, "Disconnected");
    lv_img_set_src(ui_uiwifiIcon, &ui_img_376556310);
  }

  delay(5);
}
