#include <Arduino.h>
#include <OctoPrintAPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <FastLED.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>

#include "config.h"

#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_INTERRUPT_RETRY_COUNT 0

#define LED_PIN D2
#define NUM_LEDS 16
#define LED_BRIGHTNESS 16
#define MILLI_AMPS 1500
#define CONNECTION_COLOR 0xFF00FF

CRGB rawleds[NUM_LEDS];
CRGBSet leds(rawleds, NUM_LEDS);
CRGBSet leds1(leds(0, (NUM_LEDS / 2) - 1));
CRGBSet leds2(leds(NUM_LEDS / 2, NUM_LEDS - 1));

WiFiClient client;

unsigned long api_mtbs = 5000; //mean time between api requests (10 seconds)
unsigned long api_lasttime = 0; //last time api request has been done
byte connection_retry = 0;
byte point = 0;
int opstatus = 0;
int bedtempa;
int hetempa;
int printstatus;

long printed_timeout = 600000; //10 mins in milliseconds - timeout after printing completed, to clear strip
long printed_timeout_timer = printed_timeout;

OctoprintApi api(client, ip, octoprint_httpPort, octoprint_apikey); //If using IP address

void setup()
{
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  delay(100);
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  delay(1000);
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Yellow);
  delay(1000);
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  delay(1000);
  FastLED.show();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(400);
    Serial.print(".");
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    leds[connection_retry] = CONNECTION_COLOR;
    delay(100);
    FastLED.show();
    if (connection_retry == NUM_LEDS)
    {
      connection_retry = 0;
    }
    else
    {
      connection_retry++;
    }
  }
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  ArduinoOTA.setPassword("OTAUpgradePassword");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      Serial.println("Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      Serial.println("Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      Serial.println("Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      Serial.println("Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (millis() - api_lasttime > api_mtbs || api_lasttime == 0)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      if (api.getPrintJob())
      {
        if (api.printJob.printerState == "Offline")
        {
          opstatus = 0;
        }
        else if (api.printJob.printerState == "Operational")
        {
          opstatus = 1;
          if (api.getPrinterStatistics())
          {
            if (api.printerStats.printerBedTempTarget != 0 && api.printerStats.printerTool0TempTarget == 0.0 && api.printerStats.printerBedTempActual < api.printerStats.printerBedTempTarget)
            {
              opstatus = 2;
            }
            if (api.printerStats.printerBedTempTarget == 0.0 && api.printerStats.printerTool0TempTarget != 0 && api.printerStats.printerTool0TempActual < api.printerStats.printerTool0TempTarget)
            {
              opstatus = 3;
            }
            if (api.printerStats.printerTool0TempTarget != 0 && api.printerStats.printerBedTempTarget != 0 && api.printerStats.printerBedTempActual < api.printerStats.printerBedTempTarget && api.printerStats.printerTool0TempActual < api.printerStats.printerTool0TempTarget)
            {
              opstatus = 4;
            }
            if ((api.printerStats.printerBedTempTarget != 0 && api.printerStats.printerTool0TempTarget != 0) && (api.printerStats.printerBedTempActual < api.printerStats.printerBedTempTarget || api.printerStats.printerTool0TempActual < api.printerStats.printerTool0TempTarget))
            {
              opstatus = 4;
            }
            if (api.printerStats.printerBedTempTarget != 0 && api.printerStats.printerBedTempActual >= api.printerStats.printerBedTempTarget && api.printerStats.printerTool0TempTarget == 0.0)
            {
              opstatus = 5;
            }
            if (api.printerStats.printerTool0TempTarget != 0 && api.printerStats.printerTool0TempActual >= api.printerStats.printerTool0TempTarget && api.printerStats.printerBedTempTarget == 0.0)
            {
              opstatus = 6;
            }
            if (api.printerStats.printerTool0TempTarget != 0 && api.printerStats.printerBedTempTarget != 0 && api.printerStats.printerBedTempActual >= api.printerStats.printerBedTempTarget && api.printerStats.printerTool0TempActual >= api.printerStats.printerTool0TempTarget)
            {
              opstatus = 7;
            }
            if (api.printerStats.printerTool0TempTarget == 0.0 && api.printerStats.printerBedTempTarget == 0.0 && api.printerStats.printerBedTempActual > 40.0 && api.printerStats.printerTool0TempActual < 70.0)
            {
              opstatus = 8;
            }
            if (api.printerStats.printerTool0TempTarget == 0.0 && api.printerStats.printerBedTempTarget == 0.0 && api.printerStats.printerTool0TempActual > 70.0 && api.printerStats.printerBedTempActual < 40.0)
            {
              opstatus = 9;
            }
            if (api.printerStats.printerTool0TempTarget == 0.0 && api.printerStats.printerBedTempTarget == 0.0 && api.printerStats.printerTool0TempActual > 70.0 && api.printerStats.printerBedTempActual > 40.0)
            {
              opstatus = 10;
            }
          }
        }
        else if (api.printJob.printerState == "Printing")
        {
          opstatus = 11;
          if (api.getPrinterStatistics())
          {
            if (api.printJob.progressCompletion < 1 && api.printerStats.printerBedTempTarget != 0 && api.printerStats.printerTool0TempTarget == 0.0 && api.printerStats.printerBedTempActual < api.printerStats.printerBedTempTarget)
            {
              opstatus = 2;
            }
            else if (api.printJob.progressCompletion < 1)
            {
              opstatus = 4;
            }
            else
            {
              opstatus = 11;
            }
          }
        }
        else
        {
          opstatus = 0;
        }
      }
      switch (opstatus)
      {
      case 0: // Printer is offline
        Serial.println("OP0 - Nyomtato offline");
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        delay(100);
        FastLED.show();
        break;
      case 1: // Printer is operational
        Serial.println("OP1 - Nyomtato uzemkesz");
        fill_solid(leds, NUM_LEDS, CRGB::Green);
        delay(100);
        FastLED.show();
        break;
      case 2: // Bed is heating
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        fill_solid(leds, NUM_LEDS / 2, CRGB::DarkOrange);
        bedtempa = map(api.printerStats.printerBedTempActual, 20, api.printerStats.printerBedTempTarget, 0, NUM_LEDS / 2);
        fill_solid(leds, bedtempa, CRGB::DeepPink);
        delay(100);
        FastLED.show();
        Serial.println("OP2 - Bed heating");
        break;
      case 3: // Hotend is heating
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        fill_solid(leds2, NUM_LEDS / 2, CRGB::DarkOrange);
        hetempa = map(api.printerStats.printerTool0TempActual, 20, api.printerStats.printerTool0TempTarget, 0, NUM_LEDS / 2);
        fill_solid(leds2, hetempa, CRGB::DeepPink);
        delay(100);
        FastLED.show();
        Serial.println("OP3 - Hotend Heating");
        break;
      case 4: // BED+HE is heating
        fill_solid(leds, NUM_LEDS, CRGB::DarkOrange);
        bedtempa = map(api.printerStats.printerBedTempActual, 20, api.printerStats.printerBedTempTarget, 0, NUM_LEDS / 2);
        hetempa = map(api.printerStats.printerTool0TempActual, 20, api.printerStats.printerTool0TempTarget, 0, NUM_LEDS / 2);
        fill_solid(leds1, bedtempa, CRGB::DeepPink);
        fill_solid(leds2, hetempa, CRGB::DeepPink);
        delay(100);
        FastLED.show();
        Serial.println("OP4 - All heating");
        break;
      case 5: // Bed is ready (warm)
        fill_solid(leds, NUM_LEDS / 2, CRGB::Yellow);
        delay(100);
        FastLED.show();
        Serial.println("OP5 - Bed ready");
        break;
      case 6: // HE0 is ready (warm)
        fill_solid(leds2, NUM_LEDS / 2, CRGB::Yellow);
        delay(100);
        FastLED.show();
        Serial.println("OP6 - Hotend ready");
        break;
      case 7: // Bed+HE is ready (warm)
        fill_solid(leds, NUM_LEDS, CRGB::Yellow);
        delay(100);
        FastLED.show();
        Serial.println("OP7 - All ready");
        break;
      case 8: // Cooldown, but Bed is warm
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        fill_solid(leds, NUM_LEDS / 2, CRGB::Red);
        delay(100);
        FastLED.show();
        Serial.println("OP8 - CD, bed is warm");
        break;
      case 9: // Cooldown, but HE0 is war
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        fill_solid(leds2, NUM_LEDS / 2, CRGB::Red);
        delay(100);
        FastLED.show();
        Serial.println("OP9 - CD, Hotend is warm");
        break;
      case 10: // Cooldown, but BED+HE is warm
        Serial.println("OP10 - CD, All is warm");
        fill_solid(leds, NUM_LEDS, CRGB::Red);
        delay(100);
        FastLED.show();
        break;
      case 11: // Printing, status needed
        fill_solid(leds, NUM_LEDS, CRGB::Blue);
        printstatus = map(api.printJob.progressCompletion, 0, 100, 0, NUM_LEDS);
        fill_solid(leds, printstatus, CRGB::Aqua);
        delay(100);
        FastLED.show();
        Serial.print("Status: ");
        Serial.println(printstatus);
        Serial.println("OP11 - Printing");
        break;
      default:
        Serial.println("Unhandled status code");
      }
    }
    ArduinoOTA.handle();
    api_lasttime = millis(); //Set api_lasttime to current milliseconds run
  }
}