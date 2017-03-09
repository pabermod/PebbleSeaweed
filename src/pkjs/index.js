// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig);

// Require the keys' numeric values.
var keys = require('message_keys');

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

var seaWeedAPIKey = 'apiKeyHere';

function getForecast(spotId){

  var settings = {};
  var favHourStr = '13';
  
  try {    
    settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
    if (typeof settings.FavouriteHour != 'undefined'){
      favHourStr = settings.FavouriteHour;  
    }
    console.log('fav hour is: ' + favHourStr); 
  } catch (e) {  console.log('Exception Getting Settings');}

  var favHour = parseInt(favHourStr);
    
  // Construct URL
  var url = 'http://magicseaweed.com/api/' + seaWeedAPIKey + 
      '/forecast/?spot_id=' + spotId + '&units=eu'+
      '&fields=localTimestamp,fadedRating,solidRating,'+
      'swell.components.combined.*,'+
      'wind.speed,wind.direction';
  
  // Send request to MagicSeaWeed
  xhrRequest(url, 'GET',
    function (responseText) {
      // responseText contains a JSON object with forecast info
      var json = JSON.parse(responseText);
      var num = 0;
      
      // Create dictionary
      var dictionary = {};
      dictionary[keys.SwellHeight] = '';
      // Get only the first 2 forecasts
      for(var k in json){
        var date = new Date(json[k].localTimestamp*1000);
        if(date.getHours() == favHour){
          // Swell Keys
          dictionary[keys.FadedRating + num] = json[k].fadedRating;
          dictionary[keys.SolidRating + num] = json[k].solidRating;
          dictionary[keys.SwellPeriod + num] = 
            json[k].swell.components.combined.period;
          dictionary[keys.SwellHeight] += 
            json[k].swell.components.combined.height.toString();
          dictionary[keys.SwellDirection + num] = 
            Math.round(json[k].swell.components.combined.direction);
          // Wind Keys
          dictionary[keys.WindSpeed + num] = json[k].wind.speed;
          dictionary[keys.WindDirection + num] = json[k].wind.direction;
          num++;
          if(num === 2) {
            break;
          }
          dictionary[keys.SwellHeight] += '|';
        }
      }

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function (e) {
          console.log('Forecast info sent to Pebble!');
        },
        function (e) {
          console.log('Error sending Forecast info to Pebble!');
        }
      );
    }
  );
}

function locationSuccess (pos) {
}

function locationError (err) {
  console.log('Error requesting location!');
}

function getWeather () {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
  function (e) {
    console.log('PebbleKit JS ready!');
    // Get the initial forecast
    getForecast(177);
  });

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function (e) {
    console.log('AppMessage received!');
    getForecast(177);
  }
);