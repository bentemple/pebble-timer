/*******************************************************************************
 * FILENAME :        countdown_timer.c
 *
 * DESCRIPTION :
 *      Timer creation, destruction, modification, etc.
 *
 * PUBLIC FUNCTIONS :
 *      CountdownTimer  *countdown_timer_create(int64_t duration);
 *      void            countdown_timer_destroy(CountdownTimer
 *                          *countdown_timer);
 *      void            countdown_timer_start(CountdownTimer *countdown_timer);
 *      void            countdown_timer_stop(CountdownTimer *countdown_timer);
 *      void            countdown_timer_update(CountdownTimer *countdown_timer,
 *                          int64_t duration, bool update_duration);
 *      CountdownTimer  *countdown_timer_check_ended(CountdownTimer
 *                          **timer_array, uint8_t timer_array_count);
 *      void            countdown_timer_list_add(CountdownTimer **timer_array,
 *                          uint8_t timer_array_max, uint8_t *timer_array_count,
 *                          CountdownTimer *countdown_timer);
 *      void            countdown_timer_list_remove(CountdownTimer
 *                          **timer_array, uint8_t *timer_array_count,
 *                          uint8_t timer_index);
 *      int16_t         countdown_timer_list_get_timer_index(CountdownTimer
 *                          **timer_array, uint8_t timer_array_count,
 *                          CountdownTimer *countdown_timer);
 *      CountdownTimer  *countdown_timer_list_get_closest_timer(CountdownTimer
 *                          **timer_array, uint8_t timer_array_count);
 *      CountdownTimer  *countdown_timer_list_get_timer_by_id(
 *                          CountdownTimer **timer_array,
 *                          uint8_t timer_array_count, int32_t id);
 *      void            countdown_timer_list_destroy_all(CountdownTimer
 *                          **timer_array, uint8_t *timer_array_count);
 *      void            countdown_timer_list_save(CountdownTimer **timer_array,
 *                          uint8_t timer_array_count, uint32_t key);
 *      void            countdown_timer_list_load(CountdownTimer **timer_array,
 *                          uint8_t *timer_array_count, uint32_t key);
 *      bool            countdown_timer_get_paused(CountdownTimer
 *                          *countdown_timer);
 *      int64_t         countdown_timer_get_start(CountdownTimer
 *                          *countdown_timer);
 *      int64_t         countdown_timer_get_current_time(CountdownTimer
 *                          *countdown_timer);
 *      void            countdown_timer_rand_id(CountdownTimer
 *                          *countdown_timer);
 *      int32_t         countdown_timer_get_id(CountdownTimer *countdown_timer);
 *      int64_t         countdown_timer_get_duration(CountdownTimer
 *                          *countdown_timer);
 *      void            countdown_timer_format_text(int64_t value,
 *                          char *buff, uint8_t size);
 *      char            *countdown_timer_format_own_buff(CountdownTimer
 *                          *countdown_timer);
 *
 * NOTES :      The actual timer structure definition is not exposed to
 *              prevent direct modification of the structure.
 *
 * AUTHOR :     Eric Phillips        START DATE :    07/09/15
 *
 */


#include <pebble.h>
#include "countdown_timer.h"



/*
 * the structure of a CountdownTimer
 */

struct CountdownTimer {
  int64_t     start_ms;     //< epoch of start time in milliseconds
  int64_t     duration_ms;  //< total duration in milliseconds
  int32_t     ID;           //< random unique integer for timeline pins
  char        buff[16];     //< buffer for printing time string into
  bool        paused;       //< current state
};



/*
 * creates a CountdownTimer and sets its values to defaults
 */

CountdownTimer *countdown_timer_create(int64_t duration) {
  CountdownTimer *countdown_timer =
    (CountdownTimer*)malloc(sizeof(CountdownTimer));
  if (countdown_timer != NULL){
    countdown_timer->start_ms = 0;
    countdown_timer->duration_ms = duration;
    countdown_timer->paused = true;
    countdown_timer_rand_id(countdown_timer);
    return countdown_timer;
  }
  // error handling
  APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create CountdownTimer");
  return NULL;
}



/*
 * destroys a CountdownTimer freeing its memory
 */

void countdown_timer_destroy(CountdownTimer *countdown_timer) {
  if (countdown_timer != NULL){
    free(countdown_timer);
    countdown_timer = NULL;
    return;
  }
  // error handling
  APP_LOG(APP_LOG_LEVEL_ERROR, "Attempted to destroy NULL CountdownTimer!");
}



/*
 * starts a CountdownTimer
 *
 * the "start" time is kept up to date by adding the current
 * epoch when starting, and subtracting it when stopping
 */

void countdown_timer_start(CountdownTimer *countdown_timer) {
  if (countdown_timer->paused){
    countdown_timer->start_ms +=
      (int64_t)time(NULL) * 1000 + (int64_t)time_ms(NULL, NULL);
    countdown_timer->paused = false;
  }
}



/*
 * stops a CountdownTimer
 *
 * the "start" time is kept up to date by adding the current
 * epoch when starting, and subtracting it when stopping
 */

void countdown_timer_stop(CountdownTimer *countdown_timer) {
  if (!countdown_timer->paused){
    countdown_timer->start_ms -=
      (int64_t)time(NULL) * 1000 + (int64_t)time_ms(NULL, NULL);
    countdown_timer->paused = true;
    // give a new ID for pins
    countdown_timer_rand_id(countdown_timer);
  }
}



/*
 * update the current time of a CountdownTimer
 *
 * also modifies the start time as necessary to reflect the change
 * and if the duration is less than the update, changes the duration
 */

void countdown_timer_update(CountdownTimer *countdown_timer, int64_t duration,
  bool update_duration) {
  if (countdown_timer->duration_ms < duration || update_duration)
    countdown_timer->duration_ms = duration;
  int64_t now = (int64_t)time(NULL) * 1000 + (int64_t)time_ms(NULL, NULL);
  countdown_timer->start_ms =  ((countdown_timer->paused) ? 0 : now)
    + duration - countdown_timer->duration_ms;
}



/*
 * finds the first expired CountdownTimer
 *
 * checks through all timers and returns a pointer to the first CountdownTimer
 * ended also resets those timers that have ended
 */

CountdownTimer *countdown_timer_check_ended(CountdownTimer **timer_array,
  uint8_t timer_array_count) {
  // get current time
  CountdownTimer *return_timer = NULL;
  int64_t now = (int64_t)time(NULL) * 1000 + (int64_t)time_ms(NULL, NULL);
  // loop over timers
  for (uint8_t ii = 0; ii < timer_array_count; ii++) {
    // check if expired and not paused
    if (!timer_array[ii]->paused &&
      timer_array[ii]->start_ms + timer_array[ii]->duration_ms
      <= now){
      timer_array[ii]->start_ms = 0;
      timer_array[ii]->paused = true;
      if (return_timer == NULL) return_timer = timer_array[ii];
    }
  }
  return return_timer;
}



/*
 * adds a new CountdownTimer to an array of them
 *
 * indexes CountdownTimer count as well, and pops oldest CountdownTimer if full
 */

void countdown_timer_list_add(CountdownTimer **timer_array,
            uint8_t timer_array_max, uint8_t *timer_array_count,
            CountdownTimer *countdown_timer) {
  if ((*timer_array_count) == timer_array_max)
    countdown_timer_destroy(timer_array[timer_array_max - 1]);
  else
    (*timer_array_count)++;
  memmove(&timer_array[1], &timer_array[0],
    sizeof(CountdownTimer*) * (timer_array_max - 1));
  timer_array[0] = countdown_timer;
}



/*
 * removes a CountdownTimer
 *
 * removes a CountdownTimer pointer from an array of pointers
 * also decreases array size by one and moves memory to fill empty place
 */

void countdown_timer_list_remove(CountdownTimer **timer_array,
  uint8_t *timer_array_count, uint8_t timer_index) {
  if (timer_array != NULL) {
    if (timer_index < (*timer_array_count)) {
      memmove(&timer_array[timer_index], &timer_array[timer_index + 1],
        sizeof(CountdownTimer*) * ((*timer_array_count) - timer_index));
    }
    (*timer_array_count)--;
    return;
  }
  // error logging
  APP_LOG(APP_LOG_LEVEL_ERROR,
    "Attempted to remove CountdownTimer from NULL array");
}



/*
 * gets CountdownTimer index by pointer
 *
 * loops through an array of CountdownTimer pointers looking for a specific one
 * and returns the index of that pointer
 */

int16_t countdown_timer_list_get_timer_index(CountdownTimer **timer_array,
        uint8_t timer_array_count, CountdownTimer *countdown_timer) {
  for (uint8_t ii = 0; ii < timer_array_count; ii++)
    if (timer_array[ii] == countdown_timer)
      return ii;
  // if it failed to find the pointer
  return -1;
}



/*
 * gets the nearest to expired CountdownTimer
 *
 * finds the CountdownTimer with the least time left and returns a pointer to it
 */

CountdownTimer *countdown_timer_list_get_closest_timer(
  CountdownTimer **timer_array, uint8_t timer_array_count) {
  CountdownTimer *countdown_timer = NULL;
  for (uint8_t ii = 0; ii < timer_array_count; ii++){
    if (!countdown_timer_get_paused(timer_array[ii])) {
      if (countdown_timer !=  NULL) {
        if (countdown_timer_get_current_time(timer_array[ii]) <
        countdown_timer_get_current_time(countdown_timer)) {
          countdown_timer = timer_array[ii];
        }
      }
      else
        countdown_timer = timer_array[ii];
    }
  }
  return countdown_timer;
}



/*
 * gets a CountdownTimer by its ID or returns NULL if none were found
 */

CountdownTimer *countdown_timer_list_get_timer_by_id(
  CountdownTimer **timer_array, uint8_t timer_array_count, int32_t id) {
  for (uint8_t ii = 0; ii < timer_array_count; ii++){
    if (countdown_timer_get_id(timer_array[ii]) == id)
      return timer_array[ii];
  }
  // no match was found
  return NULL;
}



/*
 * destroys all CountdownTimers in an array
 *
 * iterates over all timers and calls countdown_timer_destroy,
 * which checks for NULL pointers automatically
 */

void countdown_timer_list_destroy_all(CountdownTimer **timer_array,
                  uint8_t *timer_array_count) {
  for (uint8_t ii = 0; ii < (*timer_array_count); ii++){
    countdown_timer_destroy(timer_array[ii]);
  }
}



/*
 * saves all the timers to persistent storage
 */

void countdown_timer_list_save(CountdownTimer **timer_array,
                uint8_t timer_array_count, uint32_t key) {
  persist_write_int(key++, timer_array_count);
  for (uint8_t ii = 0; ii < timer_array_count; ii++){
    persist_write_data(key++, timer_array[ii], sizeof(CountdownTimer));
  }
}



/*
 * loads all timers from persistent storage
 */

void countdown_timer_list_load(CountdownTimer **timer_array,
                uint8_t *timer_array_count, uint32_t key) {
  (*timer_array_count) = persist_read_int(key++);
  for (uint8_t ii = 0; ii < (*timer_array_count); ii++){
    timer_array[ii] = (CountdownTimer*)malloc(sizeof(CountdownTimer));
    persist_read_data(key++, timer_array[ii], sizeof(CountdownTimer));
  }
}



/*
 * gets whether a CountdownTimer is paused or not
 */

bool countdown_timer_get_paused(CountdownTimer *countdown_timer) {
  return countdown_timer->paused;
}



/*
 * gets the start time of a CountdownTimer
 */

int64_t countdown_timer_get_start(CountdownTimer *countdown_timer) {
  return countdown_timer->start_ms;
}



/*
 * gets the current remaining time of the CountdownTimer in milliseconds
 */

int64_t countdown_timer_get_current_time(CountdownTimer *countdown_timer) {
  int64_t current_time = (int64_t)time(NULL) * 1000
              + (int64_t)time_ms(NULL, NULL);
  if (countdown_timer->start_ms != 0)
    current_time = countdown_timer->duration_ms - (current_time
      - ((countdown_timer->start_ms + current_time - 1) % current_time));
  else
    current_time = countdown_timer->duration_ms;
  return current_time;
}



/*
 * gives the CountdownTimer a random new ID
 */

void countdown_timer_rand_id(CountdownTimer *countdown_timer) {
  countdown_timer->ID = rand() / 100;
}



/*
 * gets the ID of the CountdownTimer
 */

int32_t countdown_timer_get_id(CountdownTimer *countdown_timer) {
  return countdown_timer->ID;
}



/*
 * gets the total duration of the CountdownTimer
 */

int64_t countdown_timer_get_duration(CountdownTimer *countdown_timer) {
  return countdown_timer->duration_ms;
}



/*
 * gets the buffer of the provided CountdownTimer
 */

char *countdown_timer_get_buffer(CountdownTimer *countdown_timer) {
  return countdown_timer->buff;
}



/*
 * formats a value as text and prints it onto the provided memory
 */

void countdown_timer_format_text(int64_t value, char *buff, uint8_t size) {
  uint8_t hr = value / 3600000;
  uint8_t min = value % 3600000 / 60000;
  uint8_t sec = value % 60000 / 1000;
  if (hr > 0)
    snprintf(buff, size, "%d:%02d:%02d", hr, min, sec);
  else
    snprintf(buff, size, "%d:%02d", min, sec);
}



/*
 * formats the timers value and prints it into it's own buffer
 * also returns a pointer to that buffer for convenience
 */

char *countdown_timer_format_own_buff(CountdownTimer *countdown_timer) {
  countdown_timer_format_text(
    countdown_timer_get_current_time(countdown_timer),
    countdown_timer->buff, sizeof(countdown_timer->buff));
  return countdown_timer->buff;
}