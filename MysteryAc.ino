#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Tcl.h>
#include <DHT.h>
#include <GyverHub.h>


#define AP_SSID "Xiaomi_98E7_AIoT"
#define AP_PASS ""

const uint8_t IRPIN = 4;
const uint8_t DHTPIN = 2;


GyverHub hub("EspHome", "ESP32AC", "");
IRTcl112Ac irsend(IRPIN);
DHT dht(DHTPIN, DHT22);

//build переменные

String fanSpeedName = F("Turbo");

bool buttonOnOff = false;
bool buttonModeAc = true;

uint8_t fanSpeedIndex = 5;

// utility ac

uint8_t temperatureMode = kTcl112AcTempMin;
uint8_t modeAc = kTcl112AcCool;
uint8_t fanSpeedMode = kTcl112AcFanHigh;

bool fanSpeedTurbo = true;

// global 
float inDoorTemperature = 0.0;
float inDoorHumidity = 0.0;

void build() {
  hub.Title(F("Управление кондиционером Mystery"));

  hub.BeginWidgets();
  
  hub.WidgetSize(33);
  hub.Label_(F("fanSpeedName"), fanSpeedName, F("Режим вентилятора"));
  hub.LED_(F("status"), buttonOnOff, F("Статус кондиционера"), F(""));
  hub.LED_(F("mode"), buttonModeAc, F("Режим кондиционера"), F(""));
  
  hub.WidgetSize(50);
  if(hub.SwitchText_(F("buttonOnOff"), &buttonOnOff, F("Вкл/Выкл"), F("ON"), GH_GREEN)) {
    airConditioning();
  }

  if(hub.SwitchIcon_(F("buttonMode"), &buttonModeAc, F("Режим"), F(""), GH_GREEN)) {
    switch (buttonModeAc) {
      case 0:
        temperatureMode = kTcl112AcTempMax;
        modeAc = kTcl112AcHeat;
        break;
      case 1:
        temperatureMode = kTcl112AcTempMin;
        modeAc = kTcl112AcCool;
        break;
    }
    airConditioning();
  }

  hub.WidgetSize(50);
  hub.Gauge_(F("inDoorHumidity"), inDoorHumidity, F("%"), F("Влажность внутри"), 0.0, 100.0, 1.0, GH_BLUE);
  hub.Gauge_(F("inDoorTemperature"), inDoorTemperature, F("°C"), F("Температура внутри"), -40.0, 125.0, 0.5, GH_RED);

  hub.WidgetSize(100);
  if(hub.Slider_(F("fanSpeedIndex"), &fanSpeedIndex, GH_UINT8, F("Скорость вентилятора"), 0.0, 5.0, 1.0)) {
    switch (fanSpeedIndex) {
      case 0:
        fanSpeedName = "Auto";
        fanSpeedMode = kTcl112AcFanAuto;
        fanSpeedTurbo = false;
        break;
      case 1:
        fanSpeedName = "Quiet";
        fanSpeedMode = kTcl112AcFanMin;
        fanSpeedTurbo = false;
        break;
      case 2:
        fanSpeedName = "Low";
        fanSpeedMode = kTcl112AcFanLow;
        fanSpeedTurbo = false;
        break;
      case 3:
        fanSpeedName = "Medium";
        fanSpeedMode = kTcl112AcFanMed;
        fanSpeedTurbo = false;
        break;
      case 4:
        fanSpeedName = "High";
        fanSpeedMode = kTcl112AcFanHigh;
        fanSpeedTurbo = false;
        break;
      case 5:
        fanSpeedName = "Turbo";
        fanSpeedTurbo = true;
        break;
    }
    airConditioning();
  }
}

void setup() {
  Serial.begin(115200);
  irsend.begin();
  dht.begin();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println(WiFi.localIP());
  
  hub.setupMQTT("m4.wqtt.ru", 8128, "u_KLU5V4", "sWCncsUp");
  
  hub.onBuild(build);
  hub.begin();
  hub.sendGetAuto(true);
}

void loop() {
  hub.tick();
  static GHtimer timer(2000); 
  if(timer.ready()) {
    inDoorTemperature = dht.readTemperature();
    inDoorHumidity = dht.readHumidity();

    hub.sendUpdate("inDoorTemperature", String(inDoorTemperature, 1));
    hub.sendUpdate("inDoorHumidity", String(inDoorHumidity, 0));
    hub.sendGet("inDoorTemperature", String(inDoorTemperature, 1));
    hub.sendGet("inDoorHumidity", String(inDoorHumidity, 0));
    hub.turnOn();
  }

}

void airConditioning() {
  irsend.setModel(GZ055BE1);
  irsend.setPower(buttonOnOff);
  irsend.setMode(modeAc);
  irsend.setTemp(temperatureMode);
  irsend.setFan(fanSpeedMode);
  irsend.setTurbo(fanSpeedTurbo);
  irsend.setSwingVertical(kTcl112AcSwingVHighest);
  irsend.send(2);

  printState();

  hub.sendUpdate("status", String(buttonOnOff));
  hub.sendUpdate("fanSpeedName", fanSpeedName);
  hub.sendUpdate("mode", String(buttonModeAc));
}

void printState() {
  Serial.println("Mystery A/C remote is in the following state:");
  Serial.printf("%s\n", irsend.toString().c_str());

  unsigned char* ir_code = irsend.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kTeknopointStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}
