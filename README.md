# ESP8266-Octoprint-FastLED-Status
WS2812b statusbar for OctoPrint via API

A Dirty progress bar for my Ender3.

# States and colors
Startup: Red, then Yellow, then Green

Connecting to WiFi: Pink leds scrolling

Printer offline (not connected to OctoPrint, or switched off): No light

Printer online: Green

Bed heating: First half of leds, Orange/Pink progress bar

HotEnd heating: Second half of leds, Orange/Pink progress bar

Cooldown, but bed is over 40C: first half of leds, Red

Cooldown, but hotend is over 70C: second half of leds, Red

Printing: Blue / White progress bar
