// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var pm1 = myObj.pm1;
      var pm2_5 = myObj.pm2_5;
      var pm10 = myObj.pm10;
      var air_quality = myObj.air_quality;
      var temperature = myObj.temperature;
      var humidity = myObj.humidity;

      // Display variables in the HTML
      document.getElementById('pm1').innerText = myObj.pm1;
      document.getElementById('pm2_5').innerText = myObj.pm2_5;
      document.getElementById('pm10').innerText = myObj.pm10;
      document.getElementById('air_quality').innerText = myObj.air_quality;
      document.getElementById('temperature').innerText = myObj.temperature;
      document.getElementById('humidity').innerText = myObj.humidity;
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

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
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);

    // Update HTML with new readings
    document.getElementById('pm1').innerText = myObj.pm1;
    document.getElementById('pm2_5').innerText = myObj.pm2_5;
    document.getElementById('pm10').innerText = myObj.pm10;
    document.getElementById('air_quality').innerText = myObj.air_quality;
    document.getElementById('temperature').innerText = myObj.temperature;
    document.getElementById('humidity').innerText = myObj.humidity;
  }, false);
}

function showCharts() {
  // Create an iframe element to embed the new webpage
  var iframe = document.createElement('iframe');
  iframe.src = 'C:\Users\felya\Documents\Arduino\WebServer\data\graphs.html'; // Update the path accordingly
  iframe.style.width = '100%';
  iframe.style.height = '600px'; // Set an appropriate height

  // Append the iframe to the body or any other container element
  document.body.appendChild(iframe);
}

function showCharts() {
  // Navigate to the charts page
  window.location.href = 'graphs.html';
}


function showCharts() {
  // Navigate to the charts page
  window.location.href = 'graphs.html';
}

// Add event listener to the button
document.getElementById('showChartsButton').addEventListener('click', showCharts);
