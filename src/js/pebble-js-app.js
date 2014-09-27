
var req_type = 0; // 0 = Current weather, 1 = 5 day forecast

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();

  if (req_type == 0) {
    req.open('GET', "http://api.openweathermap.org/data/2.1/find/city?" +
      "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
    req.onload = function(e) {
      if (req.readyState == 4) {
        if(req.status == 200) {
          console.log(req.responseText);
          response = JSON.parse(req.responseText);
          var temperature, icon, city;
          if (response && response.list && response.list.length > 0) {
            var weatherResult = response.list[0];
            temperature = Math.round(weatherResult.main.temp - 273.15);
            icon = weatherResult.weather[0].icon;
            city = weatherResult.name;
            console.log(temperature);
            console.log(icon);
            console.log(city);
            Pebble.sendAppMessage({
              "icon":icon,
              "temperature":temperature + "\u00B0C",
              "city":city});
          }

        } else {
          console.log("Error");
        }
      }
    }
  } else {
    var day = req_type - 1;
    req.open('GET', "http://api.openweathermap.org/data/2.5/forecast/daily?" +
      "lat=" + latitude + "&lon=" + longitude + "&cnt=5", true);
    req.onload = function(e) {
      if (req.readyState == 4) {
        if(req.status == 200) {
          console.log(req.responseText);
          response = JSON.parse(req.responseText);
          var fcast_dt, fcast_icon, fcast_desc;
          if (response && response.list && response.list.length > 0) {
            var weatherResult = response.list[day];
            fcast_dt = weatherResult.dt;
            fcast_icon = weatherResult.weather[0].icon;
            fcast_desc = weatherResult.weather[0].description;
            console.log(fcast_dt);
            console.log(fcast_icon);
            console.log(fcast_desc);
            Pebble.sendAppMessage({
              "fcast_dt": fcast_dt,
              "fcast_icon": fcast_icon,
              "fcast_desc": fcast_desc});
          }

        } else {
          console.log("Error");
        }
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
  Pebble.sendAppMessage({
    "city":"Location Error",
    "temperature":"?\u00B0C"
  });
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
                          req_type = e.payload.type;
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                          console.log(e.payload.type);
                          console.log("message!");
                        });

Pebble.addEventListener("webviewclosed",
                                     function(e) {
                                     console.log("webview closed");
                                     console.log(e.type);
                                     console.log(e.response);
                                     });


