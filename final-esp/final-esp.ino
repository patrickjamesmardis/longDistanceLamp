// esp wifi headers
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

// eps file system header - to access files needed for web page (obj + mtl)
#include "LittleFS.h"
// arduino properties header
#include "thingProperties.h"

// initialize web server: https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
ESP8266WebServer server(80);

// define variables for web handlers
void handleRoot();
void handleNotFound();
void handleObjFile();
void handleMtlFile();
void handleColor();

void setup()
{
  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);

  // begin wifi and give an mdns hostname ... SSID and PASS defined in espSecret.h
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  MDNS.begin("lamp");

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(4);

  // begin the file system for web files
  LittleFS.begin();

  // attach handler functions to server responses
  server.on("/", handleRoot);
  server.on("/lampBase.obj", handleObjFile);
  server.on("/lampBase.mtl", handleMtlFile);
  server.on("/setColor", handleColor);

  server.onNotFound(handleNotFound);
  server.begin();

  Serial.print("IP:");
  Serial.println(WiFi.localIP());
}

void loop()
{
  //  check for cloud updates
  ArduinoCloud.update();
  //  update the mdns hostname
  MDNS.update();
  //  handle any server requests
  server.handleClient();
}

void onColorChange()
{
  Serial.println(color);
}

// serve index.html
void handleRoot()
{
  File index = LittleFS.open("/index.html", "r");
  server.streamFile(index, "text/html");
  index.close();
}

void handleColor()
{
  color = server.arg("r") + "," + server.arg("g") + "," + server.arg("b");
  onColorChange();
  server.sendHeader("Location", "/");
  server.send(303);
}

// give the lamp base mtl file, read only
// give a cache header so it doesn't keep reloading on a color update (these files aren't changing or going anywhere) - https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Cache-Control
void handleMtlFile()
{
  File mtl = LittleFS.open("/lampBase.mtl", "r");
  server.sendHeader("Cache-Control", "public, max-stale=600, immutable");
  server.streamFile(mtl, "model/mtl");
  mtl.close();
}

// same as mtl file above, but with lamb base obj file
void handleObjFile()
{
  File obj = LittleFS.open("/lampBase.obj", "r");
  server.sendHeader("Cache-Control", "public, max-stale=600, immutable");
  server.streamFile(obj, "model/obj");
  obj.close();
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Not Found");
}
