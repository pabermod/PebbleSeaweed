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
      {
        "label": "Australia",
        "value": "2391"
      }
    ]
  },
  {
    "type": "select",
    "messageKey": "FavouriteHour",
    "defaultValue": "18",
    "label": "Favourite Hour (GMT)",
    "options": [
      { 
        "label": "0:00",
        "value": "0" 
      },
      { 
        "label": "3:00",
        "value": "3" 
      },
      { 
        "label": "6:00",
        "value": "6" 
      },
      { 
        "label": "9:00",
        "value": "9" 
      },
      { 
        "label": "12:00",
        "value": "12" 
      },
      { 
        "label": "15:00",
        "value": "15" 
      },
      { 
        "label": "18:00",
        "value": "18" 
      },
      { 
        "label": "21:00",
        "value": "21" 
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
