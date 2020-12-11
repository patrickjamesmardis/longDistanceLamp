# Long-Distance Lamp | Object Final Project

This project is a solution that provides long-distance couples with a communication tool outside of our normal text or voice based communications. Two lamps use an [ESP8266 WiFi Module](https://www.sparkfun.com/products/17146) connected to [Arudino's IoT Cloud](https://www.arduino.cc/en/IoT/HomePage) storing a color variable formatted as a string: 

```c++
    String color = R + "," + G + "," + B;
```

Each lamp uses an [HC-SR04 Ultrasonic Distance Sensor](https://www.sparkfun.com/products/15569) to activate a light, a [NeoPixel 12-LED Ring](https://www.adafruit.com/product/1643) or [RGB LED](https://www.sparkfun.com/products/11120), with the most recent color when the distance from the sensor is less than 30 centimeters.

## Parts

### Hardware
- [Arduino Uno](https://www.sparkfun.com/products/11021)
- [ESP8266](https://www.sparkfun.com/products/17146)
- [Arudino's IoT Cloud](https://www.arduino.cc/en/IoT/HomePage)
- [HC-SR04 Ultrasonic Distance Sensor](https://www.sparkfun.com/products/15569)
- [NeoPixel 12 LED Ring](https://www.adafruit.com/product/1643) or [RGB LED](https://www.sparkfun.com/products/11120)

### Software
- [Arduino IDE](https://www.arduino.cc/en/software)
- [Arduino IoT Cloud Account](https://www.arduino.cc/en/IoT/HomePage)
- [Node.js and npm](https://docs.npmjs.com/downloading-and-installing-node-js-and-npm)
- [Arduino IoT Cloud's API](https://www.npmjs.com/package/@arduino/arduino-iot-client)
- [Three JS](https://www.npmjs.com/package/three)
- [Three OBJ and MTL loaders](https://www.npmjs.com/package/three-obj-mtl-loader)
- [require-promise](https://www.npmjs.com/package/require-promise)
- [browserify](https://www.npmjs.com/package/browserify)
    
## ESP-8266

Setting up a "thing" and property on my IoT cloud account provides a template for the code the ESP will use. A thingProperties header file includes the IoT [library](https://github.com/arduino-libraries/ArduinoIoTCloud) to initialize the ```color``` variable and it's ```onColorChange()``` function. Adding the ESP's Wifi, WebServer, and mDNS [libraries](https://github.com/esp8266/Arduino/tree/master/libraries) to the .ino file gives the ESP the ability to serve a web page on its local network, and [LittleFS](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html) gives the ability to upload files to the ESP. WiFi and secret variables are stored in a secret header file.

The ESP's [server capability](https://lastminuteengineers.com/creating-esp8266-web-server-arduino-ide/) is initialized with
```c++
    ESP8266WebServer server(80);
```

Next, the functions for each client request are initialized
```c++
    void handleRoot(); // to serve index.html
    void handleNotFound(); // any request not found (404)
    void handleObjFile(); // request for lampBase.obj
    void handleMtlFile(); // request for lampBase.mtl
    void handleColor(); // request to change color
```

In the ``` setup() ``` function, Serial communication begins with a speed of 115200 baud. The WiFi connection starts in station mode to serve on the local WiFi network. An mDNS hostname is started to use lamp.local to connnect; however, this method isn't supported in all browsers, so the local IP address is printed to the Serial. ``` initProperties() ``` is provided by Arduino to initialize the properties from the thing properties header file. ```LittleFS.begin()``` gives access to the file system. The server is set up to handle requests related to the functions above:
```c++
    server.on("/", handleRoot);
    server.on("/lampBase.obj", handleRoot);
    server.on("/lampBase.mtl", handleRoot);
    server.on("/setColor", handleRoot);
    server.onNotFound(handleNotFound);
    server.begin();
```

These functions are defined as follows using LittleFS to get the ESP's file system. The tool to upload files and it's installation process is detailed in the [ESP's docs](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#uploading-files-to-file-system). I found the server's ```streamFile()``` in this [ESP8266 guide](https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html).
```c++
    void handleRoot() {
        File index = LittleFS.open("/index.html", "r");
        server.streamFile(index, "text/html");
        index.close()
    }
    void handleColor() { // the browser will send a GET request with r, g, and b 
        color = server.arg("r") + "," + server.arg("g") + "," + server.arg("b");
        onColorChange();
        server.sendHeader("Location", "/");
        server.send(303);
    }
    void handleMtlFile() {
        File mtl = LittleFS.open("/lampBase.mtl", "r");
        server.sendHeader("Cache-Control", "public, max-stale=600, immutable");
        server.streamFile(mtl, "model/mtl");
        mtl.close();
    }
    void handleObjFile() {
        File obj = LittleFS.open("/lampBase.obj", "r");
        server.sendHeader("Cache-Control", "public, max-stale=600, immutable");
        server.streamFile(obj, "model/obj");
        obj.close();
    }
    void handleNotFound() {
        server.send(404, "text/plain", "404: Not Found");
    }
```
The [MDN docs](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers) layout the use of HTTP headers. Without setting the cache control on the MTL and OBJ files, they reload on every update of the color.
When the browser sends a request to set the color, ```onColorChange()``` is called to print the formatted color to the Serial.

The index.html file holds a section for a Three JS scene, a color input, and the local times in both time zones. It calls for the bundle.js file to produce the Three JS scene and connect to the IoT cloud.

The bundle.js 