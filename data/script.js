// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

var gaugePot = new RadialGauge({
  renderTo: 'gauge-potentiometer',
  width: 300,
  height: 300,
  units: "Distance (cm)",
  minValue: 0,
  maxValue: 1500,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueInt: 4,
  majorTicks: [
      "0",
      "300",
      "600",
      "900",
      "1200",
      "1500"

  ],
  highlights: [
    {
        "from": 0,
        "to": 600,
        "color": "#51FFEA"
    }
  ],
  minorTicks: 4,
  strokeTicks: true,
  colorPlate: "#fff",
  borderShadowWidth: 0,
  borders: false,
  needleType: "line",
  colorNeedle: "#E81313",
  colorNeedleEnd: "#E81313",
  needleWidth: 2,
  needleCircleSize: 3,
  colorNeedleCircleOuter: "#E81313",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 500,
  animationRule: "linear"
}).draw();

// Create Temperature Gauge
var gaugeTemp = new LinearGauge({
  renderTo: 'gauge-temperature',
  width: 120,
  height: 400,
  units: "Temperature C",
  minValue: 0,
  startAngle: 90,
  ticksAngle: 180,
  maxValue: 50,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueDec: 2,
  valueInt: 2,
  majorTicks: [
      "0",
      "5",
      "10",
      "15",
      "20",
      "25",
      "30",
      "35",
      "40"
  ],
  minorTicks: 4,
  strokeTicks: true,
  highlights: [
      {
          "from": 35,
          "to": 50,
          "color": "rgba(200, 50, 50, .75)"
      }
  ],
  colorPlate: "#fff",
  colorBarProgress: "#CC2936",
  colorBarProgressEnd: "#049faa",
  borderShadowWidth: 0,
  borders: false,
  needleType: "arrow",
  needleWidth: 2,
  needleCircleSize: 7,
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 500,
  animationRule: "linear",
  barWidth: 10,
}).draw();
  
// Create Humidity Gauge
var gaugeHum = new RadialGauge({
  renderTo: 'gauge-humidity',
  width: 300,
  height: 300,
  units: "Humidity (%)",
  minValue: 0,
  maxValue: 100,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueInt: 2,
  majorTicks: [
      "0",
      "20",
      "40",
      "60",
      "80",
      "100"

  ],
  minorTicks: 4,
  strokeTicks: true,
  highlights: [
      {
          "from": 0,
          "to": 40,
          "color": "#51FFEA"
      }
  ],
  colorPlate: "#fff",
  borderShadowWidth: 0,
  borders: false,
  needleType: "line",
  colorNeedle: "#E81313",
  colorNeedleEnd: "#E81313",
  needleWidth: 2,
  needleCircleSize: 3,
  colorNeedleCircleOuter: "#E81313",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 500,
  animationRule: "linear"
}).draw();

// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhrg = new XMLHttpRequest();
  xhrg.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var temp = myObj.temperature;
      var hum = myObj.humidity;
      gaugeTemp.value = temp;
      gaugeHum.value = hum;
    }
  }; 
  xhrg.open("GET", "/readings", true);
  xhrg.send();
}

let scene, camera, rendered, cube;

function parentWidth(elem) {
  return elem.parentElement.clientWidth;
}

function parentHeight(elem) {
  return elem.parentElement.clientHeight;
}

function init3D(){
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0xffffff);

  camera = new THREE.PerspectiveCamera(75, parentWidth(document.getElementById("3Dcube")) / parentHeight(document.getElementById("3Dcube")), 0.1, 1000);

  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(parentWidth(document.getElementById("3Dcube")), parentHeight(document.getElementById("3Dcube")));

  document.getElementById('3Dcube').appendChild(renderer.domElement);

  // Create a geometry
  const geometry = new THREE.BoxGeometry(5, 1, 4);

  // Materials of each face
  var cubeMaterials = [
    new THREE.MeshBasicMaterial({color:0x03045e}),
    new THREE.MeshBasicMaterial({color:0x023e8a}),
    new THREE.MeshBasicMaterial({color:0x0077b6}),
    new THREE.MeshBasicMaterial({color:0x03045e}),
    new THREE.MeshBasicMaterial({color:0x023e8a}),
    new THREE.MeshBasicMaterial({color:0x0077b6}),
  ];

  const material = new THREE.MeshFaceMaterial(cubeMaterials);

  cube = new THREE.Mesh(geometry, material);
  scene.add(cube);
  camera.position.z = 5;
  renderer.render(scene, camera);
}

// Resize the 3D object when the browser window changes size
function onWindowResize(){
  camera.aspect = parentWidth(document.getElementById("3Dcube")) / parentHeight(document.getElementById("3Dcube"));
  //camera.aspect = window.innerWidth /  window.innerHeight;
  camera.updateProjectionMatrix();
  //renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setSize(parentWidth(document.getElementById("3Dcube")), parentHeight(document.getElementById("3Dcube")));

}

window.addEventListener('resize', onWindowResize, false);

// Create the 3D representation
init3D();

// Create events for the sensor readings
if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);

  source.addEventListener('gyro_readings', function(e) {
    console.log("gyro_readings", e.data);
    var obj = JSON.parse(e.data);
    document.getElementById("gyroX").innerHTML = obj.gyroX;
    document.getElementById("gyroY").innerHTML = obj.gyroY;
    document.getElementById("gyroZ").innerHTML = obj.gyroZ;

    // Change cube rotation after receiving the readinds
    cube.rotation.x = obj.gyroY;
    cube.rotation.z = obj.gyroX;
    cube.rotation.y = obj.gyroZ;
    renderer.render(scene, camera);
  }, false);

  source.addEventListener('accelerometer_readings', function(e) {
    console.log("accelerometer_readings", e.data);
    var obj = JSON.parse(e.data);
    document.getElementById("accX").innerHTML = obj.accX;
    document.getElementById("accY").innerHTML = obj.accY;
    document.getElementById("accZ").innerHTML = obj.accZ;
  }, false);

  source.addEventListener('distance_reading', function (e) {
    console.log("distance_reading", e.data);
    gaugePot.value = e.data;
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    gaugeTemp.value = myObj.temperature;
    gaugeHum.value = myObj.humidity;
  }, false);
}

function resetPosition(element){
  var xhrr = new XMLHttpRequest();
  xhrr.open("GET", "/"+element.id, true);
  console.log(element.id);
  xhrr.send();
}

function startRecording(){
  var xhrr = new XMLHttpRequest();
  xhrr.open("GET", "/start", true);
  console.log("start");
  xhrr.send();
}

function stopRecording(){
  var xhrr = new XMLHttpRequest();
  xhrr.open("GET", "/stop", true);
  console.log("stop");
  xhrr.send();
}

function onSubmitRun() {
  event.preventDefault();

  // Get the input value
  var runInput = document.getElementById('run').value; 

  // Construct the API URL with the parameter
  var apiUrl = 'https://final-project-sistem-berbasis-mikroprosessor.vercel.app/runs/' + encodeURIComponent(runInput);

  fetch(apiUrl)
    .then(function(response) {
      if (response.ok) {
        return response.json();
      } else {
        throw new Error('Error: ' + response.status);
      }
    })
    .then(function(data) {
      // Handle the data returned by the API and populate the table
      var table = document.getElementById('runTable');
      var tbody = table.querySelector('tbody');
      tbody.innerHTML = ''; // Clear existing table rows

      data.forEach(function(rowData) {
        var row = document.createElement('tr');

        // Add cells and populate with data
        var temperatureCell = document.createElement('td');
        temperatureCell.textContent = rowData.temperature;
        row.appendChild(temperatureCell);

        var humidityCell = document.createElement('td');
        humidityCell.textContent = rowData.humidity;
        row.appendChild(humidityCell);

        var distanceCell = document.createElement('td');
        distanceCell.textContent = rowData.distance;
        row.appendChild(distanceCell);

        var gyroCell = document.createElement('td');
        gyroCell.textContent = rowData.gyro;
        row.appendChild(gyroCell);

        var runCell = document.createElement('td');
        runCell.textContent = rowData.run;
        row.appendChild(runCell);

        var timestampCell = document.createElement('td');
        timestampCell.textContent = rowData.run_time;
        row.appendChild(timestampCell);

        // Append the row to the table body
        tbody.appendChild(row);
      });

      // Show the populated table
      table.style.display = 'table'; 
    })
    .catch(function(error) {
      console.error(error);
    });
}

