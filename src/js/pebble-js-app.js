
var messages = [];
 
function sendMessage() {
  if (messages.length === 0) {
    return;
  }

  var message = messages.shift();
  console.log("SENDING " + JSON.stringify(message));
  Pebble.sendAppMessage(message, ack, nack);

  function ack() {
    console.log("**** ACK "); // + JSON.stringify(message));
    sendMessage();
    // Do nothing!
  }
  function nack() {
    console.log("NACK " + JSON.stringify(message));
    messages.unshift(message);
    sendMessage();
  }
}

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();

  req.addEventListener("error", function(e) {
    console.log("Error handler");
    messages.push({"data":'{"error":"http error"}'});
    sendMessage();
  }, false);
  req.addEventListener("abort", function(e) {
    console.log("Abort handler");
    messages.push({"data":'{"error":"http abort"}'});
    sendMessage();
  }, false);

  console.log("Requesting data");
  req.open('GET', "http://api.openweathermap.org/data/2.5/forecast/daily?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=5&units=metric", true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        console.log(req.responseText);

        messages.push({"data": req.responseText.replace(/[^\x00-\x7F]/g, "")});
        //messages.push({"data": '{"test_label":"test_value"}'});
        sendMessage(); 
        /*
        response = JSON.parse(req.responseText);
        if (response && response.list && response.list.length > 0) {
          for (var i = 0; i < response.list.length; i++) {
            //var weatherResult = response.list[i];
            console.log(JSON.stringify(response.list[i]));
            messages.push({"data": JSON.stringify(response.list[i])});
            sendMessage();    
          }      
        }
        */
      } else {
        console.log("HTTP Error");
        messages.push({"data":'{"error":"http error"}'});
        sendMessage();
      }
    }
  }
  req.send(null);
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
  fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);

  messages.push({"data":'{"error":"location error"}'});
  sendMessage();
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 


Pebble.addEventListener("ready",
                        function(e) {
                          console.log("connect!" + e.ready);
                          locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                          console.log("message!");
                        });

Pebble.addEventListener("webviewclosed",
                                     function(e) {
                                     console.log("webview closed");
                                     console.log(e.type);
                                     console.log(e.response);
                                     });


