/*******************************************************************************
 * FILENAME :        app_settings.h
 *
 * DESCRIPTION :
 *      App-wide Clay settings: low power mode and count-up disable.
 *
 */

#pragma once

#include <pebble.h>

#define APP_SETTINGS_KEY     1
#define APP_SETTINGS_VER_KEY 2   //< separate key storing the settings schema version
#define APP_SETTINGS_VERSION 1   //< increment whenever the AppSettings struct layout changes

typedef struct AppSettings {
  bool    low_power_enabled;      //< master toggle for low power mode
  uint8_t low_power_threshold;    //< battery % at-or-below which low power activates (default 100)
  bool    show_countup;           //< show +time elapsed display after timer ends (default true)
  bool    skip_delete_animation;  //< skip the timer-deleted popup animation
  bool    countup_expiry_enabled; //< hide count-up after a set duration
  uint8_t countup_expiry_hours;   //< hours after expiry to hide count-up (1-24, default 1)
} __attribute__((__packed__)) AppSettings;

// Runtime computed state (not persisted)
// true when low_power_enabled && current battery <= low_power_threshold
extern bool g_low_power_active;

// Shared settings instance (read-only from other modules)
extern AppSettings g_app_settings;

void app_settings_load(void);
void app_settings_save(void);

// Called each time battery state changes; recomputes g_low_power_active
void app_settings_apply_battery(uint8_t battery_percent);

// AppMessage inbox handler — call from app_message_register_inbox_received
void app_settings_inbox_received(DictionaryIterator *iter, void *context);
