/*******************************************************************************
 * FILENAME :        main.c
 *
 * DESCRIPTION :
 *      Entry point and main logic for program. Controls everything
 *      of importance that happens.
 *
 * AUTHOR :     Eric Phillips        START DATE :    07/10/15
 *
 */

 #include <pebble.h>
 #include "countdown_timer.h"
 #include "menu_window.h"
 #include "detail_window.h"
 #include "setting_window.h"
 #include "popup_window.h"
 #include "phone.h"

 // constants
#define COUNTDOWN_TIMER_PERSIST_KEY 72445846
#ifdef PBL_COLOR
#define HIGHLIGHT_COLOR GColorPictonBlue
#else
#define HIGHLIGHT_COLOR GColorWhite
#endif
#define COUNTDOWN_TIMERS_MAX 8
#define COUNTDOWN_TIMER_SNOOZE_DELAY 60000
#define TIMER_MIN_LENGTH 5000
#define TIMELINE_MIN_LENGTH 900000


/*******************************************************************************
 * MAIN LOCAL VARIABLES
 */

static MenuWindow *s_menu_window = NULL;
static DetailWindow *s_detail_window = NULL;
static SettingWindow *s_setting_window = NULL;
static PopupWindow *s_popup_window = NULL;
static uint8_t s_countdown_timers_count = 0;
static CountdownTimer *s_countdown_timers[COUNTDOWN_TIMERS_MAX] = {};
static AppTimer *s_app_timer = NULL;



/*******************************************************************************
 * CALLBACKS
 */

/*
 * AppTimer callback
 *
 * update callback which determines refresh rate
 */

static void app_timer_callback(void *data) {
    s_app_timer = NULL;

    // check for expired timers
    CountdownTimer *countdown_timer =
        countdown_timer_check_ended(s_countdown_timers,
        s_countdown_timers_count);
    if (countdown_timer != NULL) {
        // deep refresh the DetailWindow in case it was that timer
        detail_window_deep_refresh(s_detail_window);
        // show timer confirmation window
        popup_window_set_countdown_timer(s_popup_window, countdown_timer);
        popup_window_set_title(s_popup_window, "Time's Up!");
        popup_window_set_highlight_color(s_popup_window, HIGHLIGHT_COLOR);
#ifdef PBL_SDK_3
        popup_window_set_pdc(s_popup_window,
            RESOURCE_ID_ICON_ALARM_CLOCK, true);
        int64_t pdc_duration = popup_window_get_pdc_duration(s_popup_window);
        popup_window_set_auto_close_duration(s_popup_window, 15000);
#else
        popup_window_set_image(s_popup_window, RESOURCE_ID_IMAGE_ALARM);
        popup_window_set_auto_close_duration(s_popup_window, 15000);
#endif
        popup_window_add_action_bar(s_popup_window);
        popup_window_push(s_popup_window, true);
        // start vibration
        static const uint32_t vibe_seg[] = {300, 200, 300, 200, 300, 1000, 300,
        200, 300, 200, 300, 1000, 300, 200, 300, 200, 300, 1000, 300, 200, 300,
        200, 300, 1000, 300, 200, 300, 200, 300, 1000, 300, 200, 300, 200, 300};
        static VibePattern pat_vibe = {
            .durations = vibe_seg,
            .num_segments = ARRAY_LENGTH(vibe_seg),
        };
        vibes_enqueue_custom_pattern(pat_vibe);
    }

    // refresh
    bool menu_top = menu_window_get_topmost_window(s_menu_window);
    bool detail_top = detail_window_get_topmost_window(s_detail_window);
    bool popup_top = popup_window_get_topmost_window(s_popup_window);
    if (menu_top) menu_window_refresh(s_menu_window);
    if (detail_top) detail_window_refresh(s_detail_window);
    if (popup_top) popup_window_refresh(s_popup_window);

    // schedule next refresh
    uint16_t refresh_rate = 35;
    if (detail_top && !detail_window_get_update_needed(s_detail_window)){
        refresh_rate = 1000;
    }
    else if (menu_top)
        refresh_rate = 1000;
    else if (popup_top)
        refresh_rate = 20;
    if (refresh_rate == 0) return;
    s_app_timer = app_timer_register(refresh_rate, app_timer_callback, NULL);
}



/*
 * PopupWindow snooze timer callback
 * snoozes the vibrating timer for one minute
 */

static void popup_window_snooze_timer_callback(CountdownTimer *countdown_timer,
    void *context) {
    countdown_timer_update(countdown_timer,
        COUNTDOWN_TIMER_SNOOZE_DELAY, false);
    countdown_timer_start(countdown_timer);
    popup_window_pop(s_popup_window, true);
}



/*
 * PopupWindow stop timer callback
 * cancels the current timer vibration sequence
 */

static void popup_window_stop_timer_callback(void *context) {
    // pop the window
    popup_window_pop(s_popup_window, true);
}



/*
 * SettingWindow complete callback
 * simple window to create or edit timer durations
 */

static void setting_window_complete_callback(int64_t duration, void *context){
    SettingWindow *setting_window = (SettingWindow*)context;
    CountdownTimer *countdown_timer = setting_window_get_timer(setting_window);
    // check if long enough
    if (duration < TIMER_MIN_LENGTH) {
        setting_window_pop(setting_window, true);
        if (s_app_timer != NULL) app_timer_reschedule(s_app_timer, 10);
        return;
    }
    // check if new timer or editing
    if (countdown_timer == NULL) {
        countdown_timer = countdown_timer_create(duration);
        countdown_timer_list_add(s_countdown_timers, COUNTDOWN_TIMERS_MAX,
            &s_countdown_timers_count, countdown_timer);
        countdown_timer_start(countdown_timer);
        // update visuals
        menu_window_reload_data(s_menu_window);
        detail_window_set_countdown_timer(s_detail_window, countdown_timer);
        detail_window_deep_refresh(s_detail_window);
        detail_window_push(s_detail_window, true);
        setting_window_pop(setting_window, false);
        // delete the Timeline pin
        if (countdown_timer_get_duration(countdown_timer) >=
            TIMELINE_MIN_LENGTH)
            phone_send_pin(countdown_timer);
    }
    else {
        countdown_timer_update(countdown_timer, duration, true);
        countdown_timer_start(countdown_timer);
        detail_window_deep_refresh(s_detail_window);
        setting_window_pop(setting_window, true);
        // deal with timeline
        phone_delete_pin(countdown_timer);
        if (countdown_timer_get_duration(countdown_timer) >=
            TIMELINE_MIN_LENGTH) {
            countdown_timer_rand_id(countdown_timer);
            phone_send_pin(countdown_timer);
        }
    }

    // show timer confirmation window
    popup_window_set_title(s_popup_window, "Timer set!");
    popup_window_set_highlight_color(s_popup_window, HIGHLIGHT_COLOR);
#ifdef PBL_SDK_3
    popup_window_set_pdc(s_popup_window, RESOURCE_ID_ICON_CONFIRMATION, false);
    int64_t pdc_duration = popup_window_get_pdc_duration(s_popup_window);
    popup_window_set_auto_close_duration(s_popup_window, pdc_duration);
#else
    popup_window_set_image(s_popup_window, RESOURCE_ID_IMAGE_STAR);
    popup_window_set_auto_close_duration(s_popup_window, 1000);
#endif
    popup_window_remove_action_bar(s_popup_window);
    popup_window_push(s_popup_window, true);
    // refresh now
    if (s_app_timer != NULL) app_timer_reschedule(s_app_timer, 10);
}



/*
 * DetailWindow edit timer callback
 * edit the timer currently in the detail view
 */

static void detail_window_edit_timer_callback(CountdownTimer *countdown_timer,
                                    void *context) {
    setting_window_set_timer(s_setting_window, countdown_timer);
    setting_window_push(s_setting_window, true);
}



/*
 * DetailWindow play pause timer callback
 * plays or pauses the timer currently in the detail view
 */

static void detail_window_playpause_timer_callback(
                            CountdownTimer *countdown_timer,    void *context) {
    if (countdown_timer_get_paused(countdown_timer)) {
        // push the Timeline pin
        if (countdown_timer_get_duration(countdown_timer) >=
            TIMELINE_MIN_LENGTH)
            phone_send_pin(countdown_timer);
        // start the timer
        countdown_timer_start(countdown_timer);
    }
    else {
        // delete the Timeline pin
        if (countdown_timer_get_duration(countdown_timer) >=
            TIMELINE_MIN_LENGTH) {
            phone_delete_pin(countdown_timer);
            countdown_timer_rand_id(countdown_timer);
        }
        // stop the timer
        countdown_timer_stop(countdown_timer);
    }
    // refresh DetailWindow
    detail_window_deep_refresh(s_detail_window);
}



/*
 * DetailWindow delete timer callback
 * delete the timer currently in the detail view
 */

static void detail_window_delete_timer_callback(CountdownTimer *countdown_timer,
                                    void *context) {
    // delete the Timeline pin
    if (countdown_timer_get_duration(countdown_timer) >= TIMELINE_MIN_LENGTH)
        phone_delete_pin(countdown_timer);

    // delete the timer
    int16_t timer_index = countdown_timer_list_get_timer_index(
        s_countdown_timers, s_countdown_timers_count, countdown_timer);
    countdown_timer_destroy(countdown_timer);
    countdown_timer_list_remove(s_countdown_timers, &s_countdown_timers_count,
         timer_index);
    // reload MenuWindow data
    menu_window_reload_data(s_menu_window);
    menu_window_refresh(s_menu_window);
    // pop detail off stack
    detail_window_pop(s_detail_window, true);

    // show timer confirmation window
    popup_window_set_title(s_popup_window, "Timer Deleted");
#ifdef PBL_SDK_3
    popup_window_set_highlight_color(s_popup_window, GColorSunsetOrange);
    popup_window_set_pdc(s_popup_window, RESOURCE_ID_ICON_DELETED, false);
    int64_t pdc_duration = popup_window_get_pdc_duration(s_popup_window);
    popup_window_set_auto_close_duration(s_popup_window, pdc_duration);
#else
    popup_window_set_image(s_popup_window, RESOURCE_ID_IMAGE_SHREADER);
    popup_window_set_auto_close_duration(s_popup_window, 1000);
#endif
    popup_window_remove_action_bar(s_popup_window);
    popup_window_push(s_popup_window, true);

    // refresh now
    if (s_app_timer != NULL) app_timer_reschedule(s_app_timer, 10);
}



/*
 * MenuWindow get timer callback
 * gets a pointer to a timer at a specific index
 */

static CountdownTimer *menu_window_get_timer_callback(uint8_t index,
                                                        void *context) {
    if (index < s_countdown_timers_count)
        return s_countdown_timers[index];
    // error handling
    APP_LOG(APP_LOG_LEVEL_ERROR,
        "Attempted to access timer outside array bounds");
    return NULL;
}



/*
 * MenuWindow get timer count callback
 * get the total number of timers
 */

static uint8_t menu_window_get_timer_count_callback(void *context) {
    return s_countdown_timers_count;
}



/*
 * MenuWindow click callback
 */

static void menu_window_click_callback(uint8_t index, void *context) {
    // add a timer if on the "+", otherwise, open the detailed view
    if (index == 0) {
        setting_window_set_timer(s_setting_window, NULL);
        setting_window_push(s_setting_window, true);
    }
    else {
        // show timer in detail window
        detail_window_set_countdown_timer(s_detail_window,
                                        s_countdown_timers[index - 1]);
        detail_window_deep_refresh(s_detail_window);
        detail_window_push(s_detail_window, true);
        // start timer refreshing quickly
        if (s_app_timer != NULL) app_timer_reschedule(s_app_timer, 10);
    }
}



/*******************************************************************************
 * INITIALIZE AND DEINITIALIZE
 */

/*
 * initialize the program
 */

static void initialize(void) {
    // connect to phone
    phone_connect();
    // load the CountdownTimer data
    if (persist_exists(COUNTDOWN_TIMER_PERSIST_KEY))
        countdown_timer_list_load(s_countdown_timers, &s_countdown_timers_count,
            COUNTDOWN_TIMER_PERSIST_KEY);
    // cancel wakeup
    wakeup_cancel_all();

    // create menu window
    MenuWindowCallbacks menu_callbacks = {
        .get_timer = menu_window_get_timer_callback,
        .get_timer_count = menu_window_get_timer_count_callback,
        .clicked = menu_window_click_callback,
    };
    s_menu_window = menu_window_create(menu_callbacks, true);
    menu_window_set_highlight_color(s_menu_window, HIGHLIGHT_COLOR);
    menu_window_refresh(s_menu_window);

    // create detail window
    DetailWindowCallbacks detail_callbacks = {
        .edit_timer = detail_window_edit_timer_callback,
        .playpause_timer = detail_window_playpause_timer_callback,
        .delete_timer = detail_window_delete_timer_callback,
    };
    s_detail_window = detail_window_create(detail_callbacks);
    detail_window_set_highlight_color(s_detail_window, HIGHLIGHT_COLOR);

    // create setting window
    SettingWindowCallbacks setting_callbacks = {
        .setting_complete = setting_window_complete_callback,
    };
    s_setting_window = setting_window_create(setting_callbacks);
    setting_window_set_highlight_color(s_setting_window, HIGHLIGHT_COLOR);

    // create pop-up window
    PopupWindowCallbacks popup_callbacks = {
        .up_click = popup_window_snooze_timer_callback,
        .down_click = popup_window_stop_timer_callback,
    };
    s_popup_window = popup_window_create();
    popup_window_set_action_bar_callbacks(s_popup_window, popup_callbacks);

    // check wakeup in case launched by pin
    if (launch_reason() == APP_LAUNCH_TIMELINE_ACTION) {
        uint32_t args = launch_get_args() % 100;
        if (args == 10) {
            CountdownTimer *countdown_timer =
                countdown_timer_list_get_timer_by_id(s_countdown_timers,
                s_countdown_timers_count, launch_get_args() / 100);
            if (countdown_timer != NULL) {
                // show timer in detail window
                detail_window_set_countdown_timer(s_detail_window,
                                                countdown_timer);
                detail_window_deep_refresh(s_detail_window);
                detail_window_push(s_detail_window, true);
            }
        }
    }

    // open the setting screen if no timers
    if (s_countdown_timers_count == 0) {
        setting_window_set_timer(s_setting_window, NULL);
        setting_window_push(s_setting_window, true);
    }

    // start the main update timer
    // update it really fast the first time so everything looks right
    s_app_timer = app_timer_register(5, app_timer_callback, NULL);
}



/*
 * deinitialize the program
 */

static void deinitialize(void) {
    // cancel the timer if it is still registered
    if (s_app_timer != NULL) app_timer_cancel(s_app_timer);
    // disconnect from phone
    phone_disconnect();

    // persist state
    countdown_timer_list_save(s_countdown_timers, s_countdown_timers_count,
        COUNTDOWN_TIMER_PERSIST_KEY);
    // schedule the wakeup
    CountdownTimer *countdown_timer =
        countdown_timer_list_get_closest_timer(s_countdown_timers,
        s_countdown_timers_count);
    if (countdown_timer != NULL) {
        time_t timestamp = time(NULL) +
            countdown_timer_get_current_time(countdown_timer) / 1000;
        // add one second to ensure it opens straight to the PopupWindow
        wakeup_schedule(timestamp + 1, 0, true);
    }

    // destroy classes
    popup_window_destroy(s_popup_window);
    setting_window_destroy(s_setting_window);
    detail_window_destroy(s_detail_window);
    menu_window_destroy(s_menu_window);
    countdown_timer_list_destroy_all(s_countdown_timers,
                                &s_countdown_timers_count);
}



/*
 * main entry point
 */

int main(void) {
    initialize();
    app_event_loop();
    deinitialize();
}
