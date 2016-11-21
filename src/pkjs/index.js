// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig);

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
    favHourStr = settings.FavouriteHour;  
    console.log('fav hour is:' + favHourStr); 
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
      var num = 2;
      var period;
      var height;
      
      for(var k in json){
        if(num === 0) break;
        var date = new Date(json[k].localTimestamp*1000);
        if(date.getHours() == favHour){
          period = json[k].swell.components.combined.period;
          height = json[k].swell.components.combined.height;
          console.log(k +' '+ json[k].localTimestamp+' '+ date.toString());
          num--;
        }
      }

      console.log('period is:' + period);
      console.log('height is' + height);

      // Assemble keys dictionary
      var dictionary = {
        'Period': period,
        'Height': height,
        'FavouriteHour': favHourStr
      };

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

    // Get the initial weather
    //getWeather();
    getForecast(177);
  });

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function (e) {
    console.log('AppMessage received!');
    //getWeather();
    getForecast(177);
  }
);