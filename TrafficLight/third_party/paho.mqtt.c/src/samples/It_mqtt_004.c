
#include "It_mqtt_test.h"



volatile MQTTClient_deliveryToken deliveredtoken;
volatile char g_subEnd = 0;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    (void)context;
    hi_at_printf("Message with token value %d delivery confirmed\r\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;
    (void)context;
    (void)topicLen;
    hi_at_printf("Message arrived\r\n");
    hi_at_printf("     topic: %s\r\n", topicName);
    hi_at_printf("   message: ");

    payloadptr = message->payload;
    for (i = 0; i < message->payloadlen; i++) {
        putchar(*payloadptr++);
    }
    putchar('\n');

    if(memcmp(message->payload,"byebye",message->payloadlen) == 0) {
        g_subEnd = 1;
        hi_at_printf("g_subEnd = %d\r\n",g_subEnd);
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    (void)context;
    hi_at_printf("\nConnection lost\r\n");
    hi_at_printf("     cause: %s\r\n", cause);
}

int mqtt_002(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = KEEPALIVEINTERVAL;
    conn_opts.cleansession = CLEANSESSION;
    conn_opts.username = USERNAME;
    conn_opts.password = PASSWORD;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        hi_at_printf("Failed to connect, return code %d\r\n", rc);
        return rc;
    }
    g_subEnd = 0;
    hi_at_printf("Subscribing to topic %s\nfor client %s using QoS%d\r\n\n"
           "wait for msg \" byebye\"\r\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);
    do {
        LOS_TaskDelay(10);
    } while((g_subEnd == 0));
    hi_at_printf("Subscribing End\r\n", rc);
    MQTTClient_unsubscribe(client, TOPIC);
    MQTTClient_disconnect(client, TIMEOUT);
    MQTTClient_destroy(&client);
    return rc;
}
