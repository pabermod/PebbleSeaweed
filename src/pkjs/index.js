// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay with manual event handling
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

function SendMessageToPebble(dict, messageType){
  Pebble.sendAppMessage(dict, function() {
    console.log(messageType + ' sent: ' + JSON.stringify(dict));
  }, function(e) {
    console.log('Failed to send ' + messageType);
    console.log(JSON.stringify(e));
  });
}

var seaWeedAPIKey = '';

var favHour = '13';

function sendSettings(){
  var settings = {};
  var colorStr = '0';

  try {    
    settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
    if (typeof settings.FavouriteHour != 'undefined'){
      favHourStr = settings.FavouriteHour;  
    } 
    if (typeof settings.Color != 'undefined'){
      colorStr = settings.Color;  
    }
  } catch (e) {  
    console.log('Exception Getting Settings');
  }

  var dictionary = {};
  dictionary[keys.FavouriteHour] = parseInt(favHourStr);
  dictionary[keys.Color] = parseInt(colorStr);

  SendMessageToPebble(dictionary, "sendSettings");
}

function sendResponse(spotId){
  var settings = {};

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

      SendMessageToPebble(dictionary, "forecast");
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

  dict[keys.FavouriteHour] = parseInt(dict[keys.FavouriteHour]);
  dict[keys.Color] = parseInt(dict[keys.Color]);

  SendMessageToPebble(dict, "clay settings");
});