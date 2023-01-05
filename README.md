# kepu-control
Control lights and water for a kitchen garden (keittiöpuutarha, kepu) 

## Setup firmware project
Create WiFiConfig.h with your wifi configuration
```
#define WIFI_SSID <your wifi ssid>
#define WIFI_PASSWORD <your wifi password>
```

## Setup client project
Change default host and port
```
DEFAULT_HOST = <your ESP8266 host ip>
DEFAULT_PORT = <your ESP8266 host port>
```
