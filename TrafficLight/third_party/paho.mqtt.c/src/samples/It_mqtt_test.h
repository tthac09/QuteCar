#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"

#define USELOCALHOST 0

#if USELOCALHOST
#define ADDRESS     "tcp://localhost:1883"
#else
#define ADDRESS     "tcp://192.168.43.101:1883"
#endif

#define SSLADDRESS     "ssl://192.168.43.101:8883"

#define CLIENTID    "ExampleClientPub"
#define TOPIC       "abc"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

#define KEEPALIVEINTERVAL 20
#define CLEANSESSION      1
#define USERNAME    "xyq"
#define PASSWORD    "Hi3861"
