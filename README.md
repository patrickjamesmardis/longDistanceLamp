# Long-Distance Lamp

This project is a solution that provides long-distance couples with a communication tool outside of our standard text or voice-based communications. Two lamps use an [ESP8266 WiFi Module](https://www.sparkfun.com/products/17146) connected to [Arudino's IoT Cloud](https://www.arduino.cc/en/IoT/HomePage), storing a color variable formatted as a string: 
```cpp
    String color = R + "," + G + "," + B;
```
Each lamp uses an [HC-SR04 Ultrasonic Distance Sensor](https://www.sparkfun.com/products/15569) to activate a light, a [NeoPixel 12-LED Ring](https://www.adafruit.com/product/1643) or [RGB LED](https://www.sparkfun.com/products/11120), with the most recent color when the distance from the sensor is less than 30 centimeters.

# Parts
## Hardware
- [Arduino Uno](https://www.sparkfun.com/products/11021)
- [ESP8266](https://www.sparkfun.com/products/17146)
- [HC-SR04 Ultrasonic Distance Sensor](https://www.sparkfun.com/products/15569)
- [NeoPixel 12-LED Ring](https://www.adafruit.com/product/1643) or [RGB LED](https://www.sparkfun.com/products/11120)
## Software
- [Arduino IDE](https://www.arduino.cc/en/software)
- [Arduino IoT Cloud Account](https://www.arduino.cc/en/IoT/HomePage)
- [Node.js and npm](https://docs.npmjs.com/downloading-and-installing-node-js-and-npm)
- [Arduino IoT Cloud's API](https://www.npmjs.com/package/@arduino/arduino-iot-client)
- [Three JS](https://www.npmjs.com/package/three)
- [Three OBJ and MTL loaders](https://www.npmjs.com/package/three-obj-mtl-loader)
- [require-promise](https://www.npmjs.com/package/require-promise)
- [browserify](https://www.npmjs.com/package/browserify)
# Putting it together

<img alt="Breadboard schematic with LED Ring" src="documentationAssets/finalObject_neoPixel_bb.jpg" width="50%"><img alt="Breadboard schematic with RGB LED" src="documentationAssets/finalObject_bb.jpg" width="50%">

The left image shows a breadboard schematic for the lamp with the NeoPixel 12-LED Ring, and the right image shows the breadboard for the lamp with the RGB LED.

The HC-SR04's echo pin is connected to pin 5 and its trig pin to pin 6. It is also connected to 5V and GND. The LED Ring receives data from pin 9, while the standard RGB LED receives R from pin 9, G from pin 10, and B from pin 11. The ESP's RX and TX pins are on pins 2 and 3, respectively. The EN, RST, and 3V3 are all connected to 3.3V, and GND is connected to GND; however, uploading code requires a different circuitry. [When uploading code](https://create.arduino.cc/projecthub/Niv_the_anonymous/esp8266-beginner-tutorial-project-6414c8#toc-esp8266-pinout-1), the IO0 pin needs to be connected to GND, and RX and TX connected to the Arduino's RX and TX. Additionally, the Arduino's RST pin should be connected to GND, or its ATMEGA328P chip must be removed to allow the code to pass through to the ESP. With all of this together, I struggled to get code to upload until I connected the ESP's RST to ground until the "Connecting ..." message appears in the console. To make this a little easier during the development process, I put two buttons on a breadboard, one between IO0 and GND and the other between RST and GND.

<img alt="Circuit set up during the development process. The Arduino's ATMEGA328P chip is removed, and two buttons ground the ESP's RST and IO0 pin." src="documentationAssets/buttons.jpg" width="50%">

# ESP8266
Setting up a "thing" and property on my IoT cloud account provides a template for the code the ESP will use. A thingProperties header file includes the IoT [library](https://github.com/arduino-libraries/ArduinoIoTCloud) to initialize the ```color``` variable and its ```onColorChange()``` function. Adding the ESP's WiFi, WebServer, and mDNS [libraries](https://github.com/esp8266/Arduino/tree/master/libraries) to the .ino file gives the ESP the ability to serve a web page on its local network, and [LittleFS](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html) gives the ability to upload files to the ESP. WiFi and secret variables are stored in a secret header file.
## ESP's [.ino file](final-esp/final-esp.ino)
The ESP's [server capability](https://lastminuteengineers.com/creating-esp8266-web-server-arduino-ide/) is initialized with

```cpp
    ESP8266WebServer server(80);
```

Next, the functions for each client request are initialized

```cpp
    void handleRoot();
    void handleNotFound();
    void handleObjFile();
    void handleMtlFile();
    void handleColor();
```
### ```setup()``` | [final-esp.ino](final-esp/final-esp.ino)
In the ``` setup() ``` function, Serial communication begins with a speed of 115200 baud. The WiFi connection starts in station mode to serve on the local WiFi network. An mDNS hostname is started to use lamp.local to connect; however, this method isn't supported in all browsers, so the local IP address is printed to the Serial. ``` initProperties() ``` is provided by Arduino to initialize the properties from the thing properties header file. ```LittleFS.begin()``` gives access to the file system. The server is set up to handle requests related to the functions above:

```cpp
    server.on("/", handleRoot);
    server.on("/lampBase.obj", handleObjFile);
    server.on("/lampBase.mtl", handleMtlFile);
    server.on("/setColor", handleColor);
    
    server.onNotFound(handleNotFound);
    server.begin();
```

### Server Request Handlers | [final-esp.ino](final-esp/final-esp.ino)
These functions are defined as follows, using LittleFS to get the ESP's file system. The tool to upload files and its installation process is detailed in the [ESP's docs](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#uploading-files-to-file-system). I found the server's ```streamFile()``` in this [ESP8266 guide](https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html).

```cpp
    void handleRoot() {
    File index = LittleFS.open("/index.html", "r");
    server.streamFile(index, "text/html");
    index.close();
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
The [MDN docs](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers) layout the use of HTTP headers. Without setting the cache control on the MTL and OBJ files, they reload every time the color is updated..
When the browser sends a request to set the color, ```onColorChange()``` is called to print the formatted color to the Serial.

### ```loop()``` | [final-esp.ino](final-esp/final-esp.ino)

The ```loop()``` function uses the IoT cloud's update method to update the cloud variable. This is supposed to work to both read and write the value; however we were only seeing it work as a read of the cloud's value, as we saw an update when changing it on the IoT Dashboard, but we didn't see an update on the dashboard when updating it locally. Implementing their npm package and controlling the read and write processes with two separate functions, [```getColor()```](#getcolor--mainjs) and [```updateColor()```](), in JS worked for keeping the variable in sync regardless of where it is (re)defined.

The ESP's complete .ino file can be found [here](final-esp/final-esp.ino)

## ESP's [index.html](web/index.html)
The web page for updating colors is defined in an HTML document with a section for a Three JS scene, a color input, and the local times in both time zones. It calls for the bundle.js file to produce the Three JS scene and connect to the IoT cloud.

```html
    <section id="threeScene"></section>
    <section id="buttons">
        <input type="color" id="colorPicker" value="#FF0000">
    </section>
    <section id="time1" class="time">
        <h1>Patrick</h1>
        <h2>11:59 PM</h2>
        <h3>Johnson City, TN</h3>
    </section>
    <section id="time2" class="time">
        <h1>Hunter</h1>
        <h2>11:59 PM</h2>
        <h3>London, UK</h3>
    </section>
    <script src="https://creative.colorado.edu/~pama4904/object/lamp/bundle.js"></script>
```
### CSS | [index.html](web/index.html)
To keep things simple, our CSS is included in a ```style``` tag in index.html

```css
    body {
        margin: 0;
    }

    section {
        position: fixed;
        top: 0;
        left: 0;
    }

    .time {
        bottom: 30px;
        top: auto;
        font-family: Helvetica, Arial, sans-serif;
        color: #fff;
    }

    #time1 {
        left: 30px;
    }

    #time2 {
        right: 30px;
        left: auto;
        text-align: right;
    }

    #colorPicker {
        margin: 30px;
        width: 100px;
        height: 100px;
    }
```

## ESP's [bundle.js](web/bundle.js)
bundle.js is the result of using browserify to bundle our main.js and node modules. It is hosted on creative.colorado because the file size is a too large for the memory of the ESPs we purchased. Additionally, this makes debugging much simpler as updating the JavaScript for the lamp doesn't require the ESP's upload process.

## ESP's [main.js](web/main.js)
main.js is where the magic happens. First, you'll want to make sure you have node and [npm](https://www.npmjs.com) installed. I'm working on macOS and prefer the hyper terminal and fish shell for work like this as they provide great customization and autocomplete features, but you can accomplish this in whichever environment pleases you. 
```fish
    node -v
    npm -v
```
I've previously installed these, but if the above commands don't return anything, you'll need to follow npm's [instructions](https://docs.npmjs.com/downloading-and-installing-node-js-and-npm) on installing node and npm. Alternatively, homebrew provides a [command](https://formulae.brew.sh/formula/node#default) to install node and npm.
```fish
    brew install node
```
### Setting up npm | [main.js](web/main.js)
The first step is to create a package.json file with the ```npm init``` command. Going through this set up is pretty straight forware, but I'm setting the entry point to main.js out of habit. The standard index.js is fine, or anything else you'd like, but you'll need to name your file accordingly.

Additionally, the following packages need to be installed and then required in main.js.

[Arduino's IoT API Client ](https://www.npmjs.com/package/@arduino/arduino-iot-client) to connect to the IoT cloud and sync the color variable. This brings request-promise with it for retrieving an OAuth2 access token.
```fish
    npm i @arduino/arduino-iot-client
```
```js
    const IotApi = require("@arduino/arduino-iot-client");
    const rp = require("request-promise");
```

[three](https://www.npmjs.com/package/three) to render a 3D animation of the lamp
```fish
    npm i three
```
```js
    const THREE = require("three");
```
[three obj/mtl loader](https://www.npmjs.com/package/three-obj-mtl-loader) to load the .obj and .mtl files for the lamp's base
```fish
    npm i three-obj-mtl-loader
```
looking at this package's index.js (node_modules/three-obj-mtl-loader/index.js) file shows the exports at the very bottom. it exports an object with an ```MTLLoader``` property and an ```OBJLoader``` property.
```js
    const loaders = require("three-obj-mtl-loader");
    const MTLLoader = loaders.MTLLoader;
    const OBJLoader = loaders.OBJLoader;
```
### ```getToken()``` | [main.js](web/main.js)
After requiring all of these packages, I define a ```getToken()```  function to get an OAuth2 access token to authenticate our interactions with the IoT cloud. This is detailed in the [Cloud API's npm documentation](https://www.npmjs.com/package/@arduino/arduino-iot-client); however, I struggled with a CORS Access Control error for a while. I found [this medium article](https://medium.com/@dtkatz/3-ways-to-fix-the-cors-error-and-how-access-control-allow-origin-works-d97d55946d9) that provides a proxy, cors-anywhere.herokuapp.com, I can send the requests to and get a response without any CORS errors. For the sake of convenience and time, I'm going to continue with this proxy; however, when moving on to another iteration of this product, I would want to serve this proxy from my server instead as the cors-anywhere app sets a rate limit.

```js
const getToken = async () => {
    let options = {
        method: "POST",
        url: "https://cors-anywhere.herokuapp.com/https://api2.arduino.cc/iot/v1/clients/token ",
        headers: { "content-type": "application/x-www-form-urlencoded", "Access-Control-Allow-Origin": "*" },
        json: true,
        form: {
            grant_type: "client_credentials",
            client_id: process.env.CLIENTID,
            client_secret: process.env.CLIENTSECRET,
            audience: "https://api2.arduino.cc/iot "
        }
    };
    try {
        const response = await rp(options);
        return response["access_token"];
    } catch (error) {
        console.log("Failed getting an access token: " + error);
    }
}
```

### ```getColor()``` | [main.js](web/main.js)
This function is called in a ```getColor()``` function before getting the IoT cloud's color variable value. The process required for this is detailed in the [Cloud API's documentation](https://www.arduino.cc/reference/en/iot/api/#api-PropertiesV2-propertiesV2Show). This function will use a few global variables:

```js
    let threeStarted = false;
    let userColor;
    let api = null;
    let deviceID = process.env.DEVICEID;
    let thingID = process.env.THINGID;
    let propertyID = process.env.PROPID;
    let lastWebColor = "";
```

The device and thing IDs are available in the Arduino IoT Cloud account. The property ID is obtained with ```console.log(data)``` where data is the response from the [propertiesV2List method](https://www.arduino.cc/reference/en/iot/api/#api-PropertiesV2-propertiesV2List).

First, the ```getColor()``` function checks if the ```api``` variable is still null. If so, it requests an access token and defines an IoT ```PropertiesV2Api```. 

```js
if (api == null) {
    const client = IotApi.ApiClient.instance;
    const oauth2 = client.authentications["oauth2"];
    oauth2.accessToken = await getToken();
    api = new IotApi.PropertiesV2Api();
}
```

Next this ```api```'s ```propertiesV2Show``` method is used to "show" the property's value, in this case the value of the ```color``` variable. The ```api``` returns an object with a property ```last_value``` which holds the last value the IoT Cloud recorded. This value is formatted as an RGB string to define a ```new THREE.Color```. Again, I ran into the CORS error with this API method, but looking at the ApiClient.js file in node_modules/@arduino/arduino-iot-client/dist, there is a ```this.basePath``` variable storing the base URL. Prepending this URL with the cors-anywhere.herokuapp.com URL sovles the problem.
```js
api.propertiesV2Show(thingID, propertyID).then(data => {
    let val = data.last_value;
    let lastCloudColor = "rgb(" + val + ")";
    userColor = new THREE.Color(lastCloudColor);
})
```

The HTML color input's value needs to be changed as well, but it uses HEX instead of RGB. I found [this function](https://stackoverflow.com/questions/5623838/rgb-to-hex-and-hex-to-rgb) to do the conversion. 

Next, the function uses the ```threeStarted``` variable to initialize the three scene if it hasn't done so already. If the three scene has started, the function compares the previous and current colors. If they are different, it updates the scene's lighting and material and informs the ESP of an updated color with the ```tellESP()``` function.

```js
    if (!threeStarted) {
        initThree();
        threeStarted = true;
        lastWebColor = val;
    } else if (val != lastWebColor) {
        pointLight.color = userColor;
        ambientLight.color = userColor;
        cylinder.material.color = userColor;
        lastWebColor = val;
        tellESP(rgbObject);
    }
```

The ```tellESP()``` function is how we ensure the local lamp's color always stays current, whether changed locally or remotely. It receives the color as an object with 3 properties: r, g, and b. It then formats this object into a URL path with three parameters and opens an [XML HTTP GET](https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest/open) request at this URL path. This sends the color to the ESP, where it is then printed to the Serial.

```js
    const tellESP = (rgbObj) => {
        let url = `/setColor?r=${rgbObj.r}&g=${rgbObj.g}&b=${rgbObj.b}`;
        let xhr = new XMLHttpRequest();
        xhr.open("GET", url, true);
        xhr.send();
    }
```

Sending an updated color to the IoT Cloud begins when a user selects a color with the color input's picker. I use an event listener to know when this happens and trigger the update. This immediately updates the lights and material in the three scene, and calls ```updateLight()``` to begin the update process. ```updateLight()``` begins by taking the HEX value of the color picker and converting it to RGB. I found a [function for this](https://stackoverflow.com/questions/5623838/rgb-to-hex-and-hex-to-rgb) along with the previous function I used to convert the other way. It then sets the ```waiting``` varaiable to ```true```

```js
    const colorPicker = document.getElementById("colorPicker");
    colorPicker.addEventListener("input", () => {
        userColor = new THREE.Color(colorPicker.value);
        pointLight.color = userColor;
        ambientLight.color = userColor;
        cylinder.material.color = userColor;
        updateLight(colorPicker.value);
    });
```
