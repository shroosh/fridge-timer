# Fridge Timer

## Central unit controls

Connect four momentary buttons between the listed GPIO and GND:

- GPIO18: select Tag 1 or Tag 2
- GPIO19: add one day to the selected timer
- GPIO22: subtract one day from the selected timer (minimum one day)
- GPIO23: reset the selected timer

Changing the day value starts or restarts that timer immediately. The LCD shows
both timers as days, hours, minutes, and seconds. Timers are held in memory and
reset when the central unit restarts.

The tag warning activates when its timer has less than 24 hours remaining.
Each tag only acts on messages addressed to its own ID; the tag's light sensor
continues to gate its LED and buzzer.

## Build targets

- `central_unit`: central controller
- `tag1`: firmware for Tag 1
- `tag2`: firmware for Tag 2

Build all three with `pio run -e central_unit -e tag1 -e tag2`.
