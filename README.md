# pebble-timer
Simple pebble timer built to be published by Pebble in the store.

![alt Timer Menu](https://github.com/pebble/pebble-timer/blob/release-1.0/assets/MenuWindow.png)
![alt Timer Setting](https://github.com/pebble/pebble-timer/blob/release-1.0/assets/SettingWindow.png)
![alt Timer Detail](https://github.com/pebble/pebble-timer/blob/release-1.0/assets/DetailWindow.png)
![alt Timer Pin](https://github.com/pebble/pebble-timer/blob/release-1.0/assets/Pin.png)

Timers appear in a list on the main screen. Add a timer via the "+" icon at the top of the list similarly
to the alarm app. When setting a timer, the time the timer will end is displayed under the entry fields if
the timer duration is greater than 15 minutes. Also, the timer will send a pin to the timeline if the duration
is greater than 15 minutes. A detailed view of the timer can be accessed by either selecting one of the 
menu items on the main screen, or by choosing "Open timer" in the pin's action menu. In the detailed view,
the timer can be played and paused as well as edited and deleted.

If the app is closed out, it will automatically open and vibrate when a timer goes off. From this timer ended
screen, the user can dismiss the timer or snooze the timer. The former will leave it as a paused timer in
the menu window, and the latter will roll the timer back one minute. 
