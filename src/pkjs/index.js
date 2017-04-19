// Initialize configuration
var Clay = require('pebble-clay');
var clayConfig = require('./config');
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

function GetSettings(){
  var favHour = 12;
  var spot = 177;
  var color = 0;  
  try {    
    console.log("settings: " + localStorage.getItem('clay-settings'));
    var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
    if (typeof settings.FavouriteHour != 'undefined'){  
      favHour = parseInt(settings.FavouriteHour);
    } 
    if (typeof settings.Color != 'undefined'){
      color = parseInt(settings.Color);  
    }
    if (typeof settings.Spot != 'undefined'){
      spot = parseInt(settings.Spot); 
    }
  } catch (e) {  
    console.log('Exception Getting settings');
  }

  var dictionary = {};
  dictionary[keys.FavouriteHour] = favHour;
  dictionary[keys.Color] = color;
  dictionary[keys.Spot] = spot;

  return dictionary;
}

function SendSettings(){
  var dictionary = GetSettings();
  SendMessageToPebble(dictionary, "SendSettings");
}

function SendForecast(){
  var dict = GetSettings();
  // Construct URL
  var url = 'http://magicseaweed.com/api/' + seaWeedAPIKey + 
      '/forecast/?spot_id=' + dict[keys.Spot] + '&units=eu'+
      '&fields=timestamp,fadedRating,solidRating,'+
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
      var timezoneOffsetHours = new Date().getTimezoneOffset() / 60;
      // Get only the first 2 forecasts
      for(var k in json){
        var date = new Date(json[k].timestamp*1000);     
        if(date.getHours() == dict[keys.FavouriteHour] - timezoneOffsetHours){
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
    SendForecast();
});

// Get AppMessage events
Pebble.addEventListener('appmessage', function(e) {
  SendForecast();
});

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }
  SendSettings();
});