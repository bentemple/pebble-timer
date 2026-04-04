module.exports = [
  {
    "type": "heading",
    "defaultValue": "Timer Settings"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Low Power Mode"
      },
      {
        "type": "toggle",
        "messageKey": "LowPowerEnabled",
        "label": "Enable Low Power Mode",
        "defaultValue": false
      },
      {
        "type": "slider",
        "messageKey": "LowPowerThreshold",
        "label": "Low Power Mode Battery Threshold %",
        "defaultValue": 100,
        "min": 10,
        "max": 100,
        "step": 10
      },
      {
        "type": "toggle",
        "messageKey": "SkipDeleteAnimation",
        "label": "Skip timer deleted animation",
        "defaultValue": false
      }

    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Timer Behaviour"
      },
      {
        "type": "toggle",
        "messageKey": "ShowCountup",
        "label": "Show ended timer count-up",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "CountupExpiryEnabled",
        "label": "Auto-hide count-up after duration",
        "defaultValue": true
      },
      {
        "type": "slider",
        "messageKey": "CountupExpiryHours",
        "label": "Count-up hide after (hours)",
        "defaultValue": 1,
        "min": 1,
        "max": 24,
        "step": 1
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
