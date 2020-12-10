const IotApi = require("@arduino/arduino-iot-client");
const rp = require("request-promise");
const THREE = require("three");
const MTLLoader = require("three-obj-mtl-loader").MTLLoader;
const OBJLoader = require("three-obj-mtl-loader").OBJLoader;

// get access token for iot cloud api
const getToken = async () => {
    let options = {
        method: "POST",
        url: "https://cors-anywhere.herokuapp.com/https://api2.arduino.cc/iot/v1/clients/token",
        headers: { "content-type": "application/x-www-form-urlencoded", "Access-Control-Allow-Origin": "*" },
        json: true,
        form: {
            grant_type: "client_credentials",
            client_id: process.env.CLIENTID,
            client_secret: process.env.CLIENTSECRET,
            audience: "https://api2.arduino.cc/iot"
        }
    };
    try {
        const response = await rp(options);
        return response["access_token"];
    } catch (error) {
        console.log("Failed getting an access token: " + error);
    }
}

// flag fore when three scene is started
let threeStarted = false;
// var to store user's current color
let userColor;

// convert rgb to hex
let componentToHex = (component) => {
    let hex = parseInt(component).toString(16);
    return hex.length == 1 ? "0" + hex : hex;
}
let toHEX = (r, g, b) => {
    return "#" + componentToHex(r) + componentToHex(g) + componentToHex(b);
}

// vars for three js elements
let scene, pointLight, ambientLight, camera, renderer, cylinder;

// listen to the color picker and update on input
const colorPicker = document.getElementById("colorPicker");
colorPicker.addEventListener("input", (e) => {
    userColor = new THREE.Color(colorPicker.value);
    pointLight.color = userColor;
    ambientLight.color = userColor;
    cylinder.material.color = userColor;
    updateLight(colorPicker.value);
});

const tellESP = (rgbObj) => {
    let url = `/setColor?r=${rgbObj.r}&g=${rgbObj.g}&b=${rgbObj.b}`;
    let xhr = new XMLHttpRequest();
    xhr.open("GET", url, true);
    xhr.send();
}

// no api until we've received an access token
let api = null;

// iot variables
let deviceID = process.env.DEVICEID;
let thingID = process.env.THINGID;
let propertyID = process.eng.PROPID;

// flag for when we're waiting on an iot update
let waiting = false;
// to store the last color the web has used to avoid constantly reupdating the same value
let lastWebColor = "";
// flaw for when we're checking for a color update;
let checking = false;

// get color from iot cloud
const getColor = async () => {
    // if we haven't initialized the api yet, get access token and define api
    if (api == null) {
        const client = IotApi.ApiClient.instance;
        const oauth2 = client.authentications["oauth2"];
        oauth2.accessToken = await getToken();
        api = new IotApi.PropertiesV2Api();
    }
    // get our property's value, format, and update web page colors
    api.propertiesV2Show(thingID, propertyID).then(data => {
        let val = data.last_value;
        let lastCloudColor = "rgb(" + val + ")";
        userColor = new THREE.Color(lastCloudColor);
        let rgbArray = val.split(",");
        let rgbObject = {
            r: rgbArray[0],
            g: rgbArray[1],
            b: rgbArray[2]
        }
        colorPicker.value = toHEX(rgbObject.r, rgbObject.g, rgbObject.b);
        // start the scene if we haven't started yet
        if (!threeStarted) {
            initThree();
            threeStarted = true;
            lastWebColor = val;
        } else if (val != lastWebColor) {
            // if the scene is running and there's a new color, update scene elements with color
            pointLight.color = userColor;
            ambientLight.color = userColor;
            cylinder.material.color = userColor;
            lastWebColor = val;
            // we also want to tell the ESP that there is a new color
            tellESP(rgbObject);
        }
    }, error => { console.log(error) });
}

getColor();

// update color on iot cloud
const updateColor = async (thisColor) => {
    let propertyVal = {
        "device_id": deviceID,
        "value": thisColor
    }
    api.propertiesV2Publish(thingID, propertyID, propertyVal).then(() => {
        // once the update is done, we're no longer waiting
        waiting = false;
        console.log("Color updated");
    }, error => {
        console.log("Failed to update color: " + error);
    });
}

// three js scene and elements ... wrapped in function to avoid starting when we don't have a color yet
let initThree = () => {
    scene = new THREE.Scene();

    pointLight = new THREE.PointLight(userColor, 10, 1200, 2);
    pointLight.position.set(0, 1000, 0);
    pointLight.castShadow = true;
    scene.add(pointLight);

    ambientLight = new THREE.AmbientLight(userColor);
    scene.add(ambientLight);

    let w = window.innerWidth;
    let h = window.innerHeight;

    camera = new THREE.PerspectiveCamera(75, w / h, 0.1, 1000);
    camera.position.z = 5;

    renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(w, h);
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setClearColor(0x777777, 1);
    document.getElementById("threeScene").appendChild(renderer.domElement);

    const geometry = new THREE.CylinderGeometry(1.2, 1.2, 3.5, 32, 4, false);
    const material = new THREE.MeshBasicMaterial({ color: userColor, transparent: true, opacity: 0.7, });
    cylinder = new THREE.Mesh(geometry, material);
    scene.add(cylinder);

    const bgGeometry = new THREE.PlaneGeometry(100, 100);
    const bgMaterial = new THREE.MeshPhongMaterial({ color: 0x777777 });
    const bg = new THREE.Mesh(bgGeometry, bgMaterial);
    bg.position.z = -10;
    scene.add(bg);

    let base;
    const loader = new OBJLoader();
    const mLoader = new MTLLoader();
    mLoader.load("lampBase.mtl", (mtl) => {
        mtl.preload();
        loader.setMaterials(mtl).load("lampBase.obj", (obj) => {
            base = obj;
            obj.scale.x = 0.25;
            obj.scale.y = 0.25;
            obj.scale.z = 0.25;
            obj.rotateY(Math.PI);
            obj.position.y = -1.8;
            scene.add(obj);
        });
    }, undefined, error => { console.log(error) });

    let animate = () => {
        if (base) {
            base.rotation.y += 0.005;
        }
        requestAnimationFrame(animate);
        renderer.render(scene, camera);
    }
    animate();
}

window.addEventListener("resize", () => {
    if (threeStarted) {
        w = window.innerWidth;
        h = window.innerHeight;
        camera.aspect = w / h;
        camera.updateProjectionMatrix();
        renderer.setSize(w, h);
    }
});

// convert hex to rgb
let toRGB = (hex) => {
    return {
        r: parseInt(hex.substring(1, 3), 16),
        g: parseInt(hex.substring(3, 5), 16),
        b: parseInt(hex.substring(5, 7), 16)
    }
}

let updateLight = (color) => {
    // prep the color var for iot cloud update
    let rgb = toRGB(color);
    console.log(rgb);
    rgbString = rgb.r + "," + rgb.g + "," + rgb.b;

    // start waiting flag until update is resolved
    waiting = true;
    // don't overwrite it with the old color if we're about to resync
    clearInterval(syncCloudColor);
    updateColor(rgbString);

    // tell ESP that we've update the color
    tellESP(rgb);
}

// keep the date up to date on an interval
let updateDate = setInterval(() => {
    let date = new Date();
    let minutes = date.getMinutes() < 10 ? "0" + date.getMinutes() : date.getMinutes();
    let ampm = date.getHours() < 12 ? "AM" : "PM";
    let hours = date.getHours() == 0 ? 12 : date.getHours() > 12 ? date.getHours() - 12 : date.getHours();
    let leftTime = document.querySelector("#time1 h2");
    leftTime.innerHTML = hours + ":" + minutes + " " + ampm;

    let rightHours = date.getHours() + 5;
    let rightAMPM = rightHours > 23 ? "AM" : rightHours > 12 ? "PM" : "AM";
    rightHours = rightHours == 24 ? 12 : rightHours > 23 ? rightHours - 24 : rightHours > 12 ? rightHours - 12 : rightHours;
    let rightTime = document.querySelector("#time2 h2");
    rightTime.innerHTML = rightHours + ":" + minutes + " " + rightAMPM;
}, 50);

// sync the color on an interval ... 10s is a slight delay, but it's better than getting hit with a rate limit
let syncCloudColor = setInterval(() => {
    if (!waiting && threeStarted) {
        checking = true;
        getColor();
    }
}, 10000);