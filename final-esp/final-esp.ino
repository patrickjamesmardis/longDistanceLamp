// esp wifi and web server headers
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
// esp mdns header - to provide a hostname instead of needing an IP address
#include <ESP8266mDNS.h>
// eps file system header - to access files needed for web page (obj + mtl)
#include "LittleFS.h"
// arduino properties header
#include "thingProperties.h"

ESP8266WebServer server(80);

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

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  MDNS.begin("lamp");

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(0);

  LittleFS.begin();

  server.on("/", handleRoot);
  server.on("/lampBase.obj", handleObjFile);
  server.on("/lampBase.mtl", handleMtlFile);
  server.on("/setcolor", handleColor);

  server.onNotFound(handleNotFound);
  server.begin();

  color = "255, 0, 0";
}

void loop()
{
  MDNS.update();
  server.handleClient();
  ArduinoCloud.update();
}

void handleRoot()
{
  String rgbColor = "\"rgb(";
  rgbColor += color;
  rgbColor += ")\"";
  String html;
  html.reserve(1024);
  html = F("<!DOCTYPE HTML><html lang=\"en\"><head><meta charset=\"UTF-8\" /><title>Lamp Data</title><style>body{margin:0}section{position:fixed;top:0;left:0}#colorPicker{margin:30px;width:100px;height:100px}</style></head>");
  html += F("<body> <section id=\"threeScene\"></section> <section id=\"buttons\"> <input type=\"color\" id=\"colorPicker\" value=\"#");
  html += rgbToHex(color);
  html += F("\"> </section>");
  html += F("<script type=\"module\">import*as THREE from\"https://unpkg.com/three@0.123.0/build/three.module.js\";import{OBJLoader}from\"https://unpkg.com/three@0.123.0/examples/jsm/loaders/OBJLoader.js\";import{MTLLoader}from\"https://unpkg.com/three@0.123.0/examples/jsm/loaders/MTLLoader.js\";");
  html += F("let userColor=new THREE.Color(");
  html += rgbColor;
  html += F(");const scene=new THREE.Scene();const pointLight=new THREE.PointLight(userColor,10,1200,2);pointLight.position.set(0,1000,0);pointLight.castShadow=true;scene.add(pointLight);const ambientLight=new THREE.AmbientLight(userColor);scene.add(ambientLight);let w=window.innerWidth;let h=window.innerHeight;const camera=new THREE.PerspectiveCamera(75,w/h,0.1,1000);camera.position.z=5;const renderer=new THREE.WebGLRenderer({antialias:true});renderer.setSize(w,h);renderer.setPixelRatio(window.devicePixelRatio);renderer.setClearColor(0x777777,1);document.getElementById(\"threeScene\").appendChild(renderer.domElement);const geometry=new THREE.CylinderGeometry(1.2,1.2,3.5,32,4,false);const material=new THREE.MeshBasicMaterial({color:userColor,transparent:true,opacity:0.7,});const cylinder=new THREE.Mesh(geometry,material);scene.add(cylinder);const bgGeometry=new THREE.PlaneGeometry(100,100);const bgMaterial=new THREE.MeshPhongMaterial({color:0x777777});const bg=new THREE.Mesh(bgGeometry,bgMaterial);bg.position.z=-10;scene.add(bg);let base;let rotate=Math.PI;const loader=new OBJLoader();const mLoader=new MTLLoader();mLoader.load(\"/lampBase.mtl\",(mtl)=>{mtl.preload();loader.setMaterials(mtl).load(\"/lampBase.obj\",(obj)=>{base=obj;console.log(base);obj.scale.x=0.25;obj.scale.y=0.25;obj.scale.z=0.25;obj.rotateY(Math.PI);obj.position.y=-1.8;scene.add(obj);});},undefined,(e)=>{console.log(e)});function animate(){if(base){base.rotation.y+=0.005;} requestAnimationFrame(animate);renderer.render(scene,camera);} animate();const colorPicker=document.getElementById(\"colorPicker\");colorPicker.addEventListener(\"input\",(e)=>{userColor=new THREE.Color(colorPicker.value);pointLight.color=userColor;ambientLight.color=userColor;cylinder.material.color=userColor;updateLight(colorPicker.value);});window.addEventListener(\"resize\",()=>{w=window.innerWidth;h=window.innerHeight;camera.aspect=w/h;camera.updateProjectionMatrix();renderer.setSize(w,h);});function toRGB(hex){var result=/^#?([a-f\\d]{2})([a-f\\d]{2})([a-f\\d]{2})$/i.exec(hex);return result?{r:parseInt(result[1],16),g:parseInt(result[2],16),b:parseInt(result[3],16)}:null;} function updateLight(color){let rgb=toRGB(color);let url=`/setcolor?r=${rgb.r}&g=${rgb.g}&b=${rgb.b}`;let xhr=new XMLHttpRequest();xhr.open(\"GET\",url);xhr.send();}let updateDate = () => { let date = new Date(); let minutes = date.getMinutes() < 10 ? \"0\" + date.getMinutes() : date.getMinutes(); let ampm = date.getHours() < 12 ? \"AM\" : \"PM\"; let hours = date.getHours() == 0 ? 12 : date.getHours() > 12 ? date.getHours() - 12 : date.getHours(); let leftTime = document.querySelector(\"#time1 h2\"); leftTime.innerHTML = hours + \":\" + minutes + \" \" + ampm;let rightHours = date.getHours() + 6; let rightAMPM = rightHours > 23 ? \"AM\" : rightHours > 12 ? \"PM\" : \"AM\"; rightHours = rightHours == 24 ? 12 : rightHours > 23 ? rightHours - 24 : rightHours > 12 ? rightHours - 12 : rightHours; let rightTime = document.querySelector(\"#time2 h2\"); rightTime.innerHTML = rightHours + \":\" + minutes + \" \" + rightAMPM; }serInterval(updateDate, 50);</script></body></html>");

  server.send(200, "text/html", html);
}

void handleColor()
{
  color = server.arg("r") + "," + server.arg("g") + "," + server.arg("b");
  onColorChange();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleMtlFile()
{
  File mtl = LittleFS.open("/lampBase.mtl", "r");
  server.sendHeader("Cache-Control", "public, max-stale=600, immutable");
  server.streamFile(mtl, "model/mtl");
  mtl.close();
}

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

String rgbToHex(String rgbString)
{
  int firstComma = rgbString.indexOf(',');
  int secondComma = rgbString.indexOf(',', firstComma + 1);
  int R = rgbString.substring(0, firstComma).toInt();
  int G = rgbString.substring(firstComma + 1, secondComma).toInt();
  int B = rgbString.substring(secondComma + 1).toInt();

  return String(((R << 16L) | (G << 8L) | B), HEX);
}

void onColorChange()
{
  Serial.println(color);
}
