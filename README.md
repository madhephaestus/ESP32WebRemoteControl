# ESP32WebRemoteControl 
A library to support introduction to robotics engineering. 

# Documentation by Doxygen

[ESP32WebRemoteControl  Doxygen](https://madhephaestus.github.io/ESP32WebRemoteControl/annotated.html)

# Depenancies

* Esp32WifiManager 0.12.0
* ESPmDNS 1.0
* FS  1.0
* Preferences 1.0
* WebServer 1.0
* WiFi 1.0

# WiFi RC control

The RC controller is hosted by the ESP32 as a website. 

The Detailed documentation is here: https://github.com/madhephaestus/Esp32WifiManager

Protocol information is here: https://github.com/WPIRoboticsEngineering/RBE1001Lib/blob/master/PROTOCOL.md


## Infrastructure network

HOWTO enter passwords:

You will use the Serial Monitor to enter the name of the network and the password to use.
The ESP32 will store them for later.
First type the SSID you want and hit send
It will prompt you for a password, type that and hit send

The ESP will default to trying to connect to a network, then fail over to AP mode


To access the RC control interface for Station mode you will watch the serial monitor as the
ESP boots, it will print out the IP address. It should look like this:

```

WiFi connected! DHCP took 8 IP address: 192.168.37.44
```


Enter that address in your computer or phones web browser.
Make sure your ESP and computer or phone are on the same network.

## AP mode (IN LAB use)


In the lab, you will want to use AP mode. To set the AP, type AP:myNetName
to set myNetName as the AP mode and hit send
the ESP will prompt you for a password to use, enter it and hit send

To make the ESP use AP mode by default on boot, change the line in RCCTL.ino

```
manager.setup();
```
 
to

```
 manager.setupAP();
```


To access the RC Control interface in AP mode, connect to the ESP with either you phone or laptop
then open in a browser on that device and go to:

```
http://192.168.4.1
```

to access the control website.

NOTE you can use this class in your final project code to visualize the state of your system while running wirelessly.
