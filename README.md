License: MIT

- Firmware to control the Lilygo T-Relay-8 board.  The board consists of 8 relays with an ESP32-Wroom-I, with 4MB of onboard Flash, Wifi & Bluetooth.  

- This Firmware enables the wifi, checks to see whether or not it has stored credentials.  If it does not, it provides an AP and a web page for the user to enter AP credentials.  The board then reboots and attempts to get online with those credentials. It then provides a web page at the IP Address assigned to it by DHCP, with 8 buttons for relay control.  The buttons also show relay status and are updated in real time.  

- This firmware is written based on Espressiff's ESP-IDF framework.  No Arduino.

- This is fairly rudimentary.  It still needs further work:
    * Add a reset credentials button to the main relay web interface
    * Handle wifi connectivity failure
    * Add Matter connectivity for Home Assistant
    * Add the ability to control multiple relay boards together from one web interface


- References: 
  * [lilygo product link](https://www.lilygo.cc/products/t-relay-5v-8-channel-relay)
  * [lilygo github](https://github.com/Xinyuan-LilyGO/LilyGo-T-Relay)
