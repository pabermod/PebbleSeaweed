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

var seaWeedAPIKey = '';

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
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      var num = 0;
      
      // Create dictionary
      var dictionary = {};
      dictionary[keys.Height] = '';
      // Get only the first 2 forecasts
      for(var k in json){
        var date = new Date(json[k].localTimestamp*1000);
        if(date.getHours() == favHour){
          dictionary[keys.Period+num] = json[k].swell.components.combined.period;
          dictionary[keys.Height] += json[k].swell.components.combined.height.toString();
          num++;
          if(num === 2) {
            break;
          }
          dictionary[keys.Height] += '|';
        }
      }

      console.log('period is: ' + dictionary[keys.Period] + ' ' + dictionary[keys.Period+1]);
      console.log('height is: ' + dictionary[keys.Height]);

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