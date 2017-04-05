module.exports = [
  {
    "type": "heading",
    "defaultValue": "App Configuration"
  },
  {
    "type": "text",
    "defaultValue": "Set your configuration"
  },
  {
    "type": "select",
    "messageKey": "Spot",
    "defaultValue": "177",
    "label": "Spot",
    "options": [
      { 
        "label": "Rodiles",
        "value": "178" 
      },
      { 
        "label": "Salinas/San Juan",
        "value": "177" 
      },    
      { 
        "label": "San Lorenzo",
        "value": "4387" 
      },
      { 
        "label": "Verdicio",
        "value": "4386" 
      },
      { 
        "label": "Xagó",
        "value": "4385" 
      },
    ]
  },
  {
    "type": "select",
    "messageKey": "FavouriteHour",
    "defaultValue": "13",
    "label": "Favourite Hour",
    "options": [
      { 
        "label": "1:00",
        "value": "1" 
      },
      { 
        "label": "4:00",
        "value": "4" 
      },
      { 
        "label": "7:00",
        "value": "7" 
      },
      { 
        "label": "10:00",
        "value": "10" 
      },
      { 
        "label": "13:00",
        "value": "13" 
      },
      { 
        "label": "16:00",
        "value": "16" 
      },
      { 
        "label": "19:00",
        "value": "19" 
      },
      { 
        "label": "22:00",
        "value": "22" 
      }
    ]
  },
  {
    "type": "select",
    "messageKey": "Color",
    "defaultValue": "0",
    "label": "Color",
    "options": [
      { 
        "label": "White text / Black background",
        "value": "0" 
      },
      { 
        "label": "Black text / White background",
        "value": "1" 
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
