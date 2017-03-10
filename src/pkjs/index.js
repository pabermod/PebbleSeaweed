// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

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

function sendResponse(spotId){

  var settings = {};
  var favHourStr = '13';
  var colorStr = '0';
  
  try {    
    settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
    if (typeof settings.FavouriteHour != 'undefined'){
      favHourStr = settings.FavouriteHour;  
    }
    console.log('fav hour is: ' + favHourStr); 
    if (typeof settings.Color != 'undefined'){
      colorStr = settings.Color;  
    }
    console.log('color is: ' + colorStr); 
  } catch (e) {  console.log('Exception Getting Settings');}

  var favHour = parseInt(favHourStr);
  var color = parseInt(colorStr);

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
      

      dictionary[keys.FavouriteHour] = favHour;
      dictionary[keys.Color] = color;

      // Send to Pebble
      // Send the object
      Pebble.sendAppMessage(dictionary, function() {
        console.log('Message sent successfully: ' + JSON.stringify(dictionary));
      }, function(e) {
        console.log('Message failed: ' + JSON.stringify(e));
      });
    }
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
  function (e) {
    console.log('PebbleKit JS ready!');
    // Get the initial forecast
    sendResponse(177);
  });

// Get AppMessage events
Pebble.addEventListener('appmessage', function(e) {
  // Get the dictionary from the message
  var dict = e.payload;

  sendResponse(177);
});

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }

  // Get the keys and values from each config item
  var dict = clay.getSettings(e.response);

  // Send settings values to watch side
  Pebble.sendAppMessage(dict, function(e) {
    console.log('Config data sent: ' + JSON.stringify(dict));
  }, function(e) {
    console.log('Failed to send config data!');
    console.log(JSON.stringify(e));
  });
});