# ESP8266-Octoprint-FastLED-Status
English below! :)
# A projektről
Szerettem volna egyszerű vizuális visszajelzést kapni a nyomtató állapotáról, de minden irányba falakba ütköztem. Az SKR 1.3-on BLTouch-al együtt nem lehetett működésre bírni, ezért alaplapot cserélni SKR1.4-re nem akartam, viszont OctoPrintet használok, így adódott a lehetőség, hogy jöjjenek az adatok onnan. Ekkor találtam rá az OctoPrintAPI ESP8266 könyvtárra:
https://github.com/chunkysteveo/OctoPrintAPI
Az abban lévő példakódok tök jók, de egyik sem fedte le 100%-ig az igényeimet, ezért több cuccból összeollózva elkészítettem ezt.

# Színjelzésekkel megkülönböztetett állapotok
* Mikrokontroller bootol
* WiFi-re kapcsolódás
* Nyomtató offline
* Nyomtató online
* Ágyfűtés bekapcsolva
* Hotend fűtés bekapcsolva
* Mindkét fűtés bekapcsolva
* Üzemmeleg állapot elérve
* Nyomtatás folyamatban
* Ágy visszahül
* Hotend visszahül

# WS2812b statusbar for OctoPrint via API

# States and colors
* Startup: Red, then Yellow, then Green
* Connecting to WiFi: Purple leds scrolling
* Printer offline (not connected to OctoPrint, or switched off): No light
* Printer online: Green
* Bed heating: First half of leds, Orange/Pink progress bar
* HotEnd heating: Second half of leds, Orange/Pink progress bar
* Cooldown, but bed is over 40C: first half of leds, Red
* Cooldown, but hotend is over 70C: second half of leds, Red
* Printing: Blue / White progress bar
