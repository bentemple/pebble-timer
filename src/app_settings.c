/*******************************************************************************
 * FILENAME :        app_settings.c
 *
 * DESCRIPTION :
 *      Load, save, and apply app-wide Clay settings.
 *
 */

#include "app_settings.h"

// Global instances
AppSettings g_app_settings;
bool g_low_power_active = false;

static void prv_default_settings(void) {
  g_app_settings.low_power_enabled      = false;
  g_app_settings.low_power_threshold    = 100;
  g_app_settings.show_countup           = true;
  g_app_settings.skip_delete_animation  = false;
  g_app_settings.countup_expiry_enabled = true;
  g_app_settings.countup_expiry_hours   = 1;
}

void app_settings_load(void) {
  prv_default_settings();
  // Only restore persisted settings if the schema version matches.
  // A mismatch means the struct layout changed and the raw bytes are invalid.
  if (persist_exists(APP_SETTINGS_VER_KEY) &&
      persist_read_int(APP_SETTINGS_VER_KEY) == APP_SETTINGS_VERSION) {
    persist_read_data(APP_SETTINGS_KEY, &g_app_settings, sizeof(g_app_settings));
  }
  // g_low_power_active starts false until first battery reading arrives
  g_low_power_active = false;
}

void app_settings_save(void) {
  persist_write_int(APP_SETTINGS_VER_KEY, APP_SETTINGS_VERSION);
  persist_write_data(APP_SETTINGS_KEY, &g_app_settings, sizeof(g_app_settings));
}

void app_settings_apply_battery(uint8_t battery_percent) {
  g_low_power_active = g_app_settings.low_power_enabled &&
                       battery_percent <= g_app_settings.low_power_threshold;
}

void app_settings_inbox_received(DictionaryIterator *iter, void *context) {
  bool changed = false;

  Tuple *low_power_t = dict_find(iter, MESSAGE_KEY_LowPowerEnabled);
  if (low_power_t) {
    g_app_settings.low_power_enabled = (bool)(low_power_t->value->int32);
    changed = true;
  }

  Tuple *threshold_t = dict_find(iter, MESSAGE_KEY_LowPowerThreshold);
  if (threshold_t) {
    g_app_settings.low_power_threshold = (uint8_t)(threshold_t->value->int32);
    changed = true;
  }

  Tuple *skip_del_t = dict_find(iter, MESSAGE_KEY_SkipDeleteAnimation);
  if (skip_del_t) {
    g_app_settings.skip_delete_animation = (bool)(skip_del_t->value->int32);
    changed = true;
  }

  Tuple *countup_t = dict_find(iter, MESSAGE_KEY_ShowCountup);
  if (countup_t) {
    g_app_settings.show_countup = (bool)(countup_t->value->int32);
    changed = true;
  }

  Tuple *expiry_en_t = dict_find(iter, MESSAGE_KEY_CountupExpiryEnabled);
  if (expiry_en_t) {
    g_app_settings.countup_expiry_enabled = (bool)(expiry_en_t->value->int32);
    changed = true;
  }

  Tuple *expiry_hr_t = dict_find(iter, MESSAGE_KEY_CountupExpiryHours);
  if (expiry_hr_t) {
    uint8_t hours = (uint8_t)(expiry_hr_t->value->int32);
    if (hours < 1)  hours = 1;
    if (hours > 24) hours = 24;
    g_app_settings.countup_expiry_hours = hours;
    changed = true;
  }

  if (changed) {
    app_settings_save();
    // Re-evaluate low power state using the current battery reading
    BatteryChargeState batt = battery_state_service_peek();
    app_settings_apply_battery(batt.charge_percent);
  }
}
