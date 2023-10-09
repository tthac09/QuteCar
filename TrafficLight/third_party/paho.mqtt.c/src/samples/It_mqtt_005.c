
#include "It_mqtt_test.h"
#define MQTT_RSA_1024 1
#if MQTT_RSA_1024
unsigned char client_crt[] = "\
-----BEGIN CERTIFICATE-----\r\n\
MIIB6DCCAVECCQC8JXlkriXk+TANBgkqhkiG9w0BAQUFADAyMTAwLgYDVQQKEydU\r\n\
TFMgUHJvamVjdCBEb2RneSBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkwHhcNMjAwMjE5\r\n\
MDcxODA4WhcNMzMxMDI4MDcxODA4WjA/MScwJQYDVQQKDB5UTFMgUHJvamVjdCBE\r\n\
ZXZpY2UgQ2VydGlmaWNhdGUxFDASBgNVBAMMCzE5Mi4xNjguMy40MIGfMA0GCSqG\r\n\
SIb3DQEBAQUAA4GNADCBiQKBgQDWKsfY1gtEccg8ZEBnOEXvFamAAPs30k6MJMwu\r\n\
PYV4H/ENwkcIqfnw06BsMKRox6wBZo3/hAQPXW1shj+cIH9WsLgsJTXx8qL2bljs\r\n\
zODYIMldMkyqQcwo974MEHR5yqczg7/K5u4/7EbFtgVFL1acZBXbTypELkn/Vzi5\r\n\
iu/drwIDAQABMA0GCSqGSIb3DQEBBQUAA4GBAKE9j1hhuwHq1X9t9chwQ3tjAA0V\r\n\
YpkzCTb3lcvnAlprgdb8B1E8zoMI0fXWIX05K5hvTSCA+lmVJgVsvL3zAn4Pf3B6\r\n\
l4IFBphXA1KugCYcQFZKoRdLldzKVZU/SxsUKdXn1Ad64Sca2hSgAY5ip3bx7RVB\r\n\
MowcmAZz594S7fcW\r\n\
-----END CERTIFICATE-----\r\n\
";

unsigned char client_key[] = "\
-----BEGIN RSA PRIVATE KEY-----\r\n\
MIICXAIBAAKBgQDWKsfY1gtEccg8ZEBnOEXvFamAAPs30k6MJMwuPYV4H/ENwkcI\r\n\
qfnw06BsMKRox6wBZo3/hAQPXW1shj+cIH9WsLgsJTXx8qL2bljszODYIMldMkyq\r\n\
Qcwo974MEHR5yqczg7/K5u4/7EbFtgVFL1acZBXbTypELkn/Vzi5iu/drwIDAQAB\r\n\
AoGAUfLat5DnjNAQ83LU5eo1cE+UpSM6/VgesCdgbY0i5h0qyr39GkaYGduQdfWC\r\n\
44kLuxl32j60owB331/bNS2GK30STjS4ha2PlErR4VV61KLsXbzCUbf8fYLFOMNN\r\n\
nTL8ceGJjYi/67P7r1Bfiz56+/AW+wIscAEVht4vDzXiAtECQQD8Ia7qBqEW/kp4\r\n\
usbAfJ75LOeR3f8d235HUZRHjfJxVhDcEKJR0rz/7NNT/Ljm+umcdQtEkK8lS9lm\r\n\
BD1FrpaHAkEA2XP7Ps/HtIV8bz2ipn+KSN7hO9tlHeT+SMyjJTZvqK6QwSZvQNIr\r\n\
JA403EhGEWSbLUaOzARUzCzKpmhcfeuhmQJAQVwF+NrBFbHT4logvbNQjq2KVjDj\r\n\
On00zg4izy3H5IN4GWQif+2OWxWsbsA7unze5FUfF6YeESAJej2tGIw6rwJALAAQ\r\n\
7aPDiB6ekC/Lkp8qDxayQpdhPYxRe8+Uj3oVW+9+sOajCl28hv4s6gnhy7EsyRuG\r\n\
13dk5S+HDeb+GCiuuQJBANr6gWNyc6GZLO+ZPawCRZIOZN/l4AKLzt5FeiHd7h6P\r\n\
ZlEei1zyjX1Dj/e2zU1KxAsuBsPP8ctIq9Z9O0eKUpU=\r\n\
-----END RSA PRIVATE KEY-----\r\n\
";

unsigned char ca_crt[] = "\
-----BEGIN CERTIFICATE-----\r\n\
MIIB2zCCAUQCCQD1n6KLgLUl8DANBgkqhkiG9w0BAQUFADAyMTAwLgYDVQQKEydU\r\n\
TFMgUHJvamVjdCBEb2RneSBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkwHhcNMjAwMjE4\r\n\
MDgwODAzWhcNMzMxMDI3MDgwODAzWjAyMTAwLgYDVQQKEydUTFMgUHJvamVjdCBE\r\n\
b2RneSBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkwgZ8wDQYJKoZIhvcNAQEBBQADgY0A\r\n\
MIGJAoGBANDyXWQ/GRICYR2Hwp+t9zti20PDcYZhWAxLFVyM1lf7xlbQnmQsyzop\r\n\
30TEwt+3x17IoTk4OxaQnNvKES+4nYujKh3aqFDV27VmnQPf471OyrXzFQbSr8Sv\r\n\
k8m2YtW2zaOxfFFTCiKrkBlLsxKZhc2wS3gXhdjdpX5XZHFyOTP3AgMBAAEwDQYJ\r\n\
KoZIhvcNAQEFBQADgYEATz0WK9Ca3TW+0KdPUa+hpxMA/6syKH+0vZ7Kx6Igg/d8\r\n\
ABhhXUp1/OzNbzzVvGVrUYAj0101rYlQrLrS/XdmOhy/jPOUJr4CVahMVPulxo+E\r\n\
5VTd3gVa9UosNAYR42c5NEZRvtb9BzjNeXcAFfPaZxMk6Or30142NAR/K4pf1PA=\r\n\
-----END CERTIFICATE-----\r\n\
";
#else
unsigned char client_crt[] = "\
-----BEGIN CERTIFICATE-----\r\n\
MIIC+DCCAeACFEZZ0SVag8Pv/rRM2mMb8mtiuTBfMA0GCSqGSIb3DQEBBQUAMDIx\r\n\
MDAuBgNVBAoMJ1RMUyBQcm9qZWN0IERvZGd5IENlcnRpZmljYXRlIEF1dGhvcml0\r\n\
eTAeFw0yMDAzMTAwMjQyMzdaFw0zMzExMTcwMjQyMzdaMD8xJzAlBgNVBAoMHlRM\r\n\
UyBQcm9qZWN0IERldmljZSBDZXJ0aWZpY2F0ZTEUMBIGA1UEAwwLMTkyLjE2OC4z\r\n\
LjQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDHQL3senI252fj3x5Z\r\n\
QnikOAMcjwQQpBOFgzhp3a4NUjnfjCB97GJMvmMB6ZmAde/fMGBywr2uvXOkmG4K\r\n\
SH7vdSJMlZkYq2gHqE1fS9R26vrP/d1lz+uvl4t+uglwP1VOxLhXdhGDWGQH5ypA\r\n\
2I1DlVTh25zlxQUQh9Yh0fU+X/y76d3S24peMrkH9uBiSMpzjRoOcZFEod5Ydy0A\r\n\
itljmcSNLIL6VsMXseH8b1EXW04ilh/CdDzduoMw/1HeL9jfquicJSLOGlJkpybG\r\n\
DvxrOswmqOeEdytqg//PDf6aZm6Lq6lwC8YVRfGqQHRkFO1BSajzT17B+Hgq0z4j\r\n\
T/lnAgMBAAEwDQYJKoZIhvcNAQEFBQADggEBAIz7DrYhRswmukwB10AJWl96yjGU\r\n\
ZDMyjh7C59sHb2IJp7Mtqop8rYRvWjzUW07ioy2FG5ZclPVpOKWnCFsTfNs5JIZE\r\n\
82KvkOjFD87UO27jHfsKYGmLQkiZppKzt1tRVS+14KjQkCMjmUjNYIDzHBUIdKLk\r\n\
dE4oBJVNqKqq0+CSXU5EGgUouwpKnpefazsICNU6sl0kcMV/3bmTihD10kJV9vac\r\n\
GZVwEgohMfWuRl/QmDXTJeH/Z/NbB1VDIT8+3aWvMNvJ/QovgJXtvaCo6ilYD5Yb\r\n\
adXi9EK7xLVrJrZ+bSa5/ta7thGuxPaDtoHf7XSyMala9ibOamqIkVE5JeE=\r\n\
-----END CERTIFICATE-----\r\n\
";

unsigned char client_key[] = "\
-----BEGIN RSA PRIVATE KEY-----\r\n\
MIIEowIBAAKCAQEAx0C97HpyNudn498eWUJ4pDgDHI8EEKQThYM4ad2uDVI534wg\r\n\
fexiTL5jAemZgHXv3zBgcsK9rr1zpJhuCkh+73UiTJWZGKtoB6hNX0vUdur6z/3d\r\n\
Zc/rr5eLfroJcD9VTsS4V3YRg1hkB+cqQNiNQ5VU4duc5cUFEIfWIdH1Pl/8u+nd\r\n\
0tuKXjK5B/bgYkjKc40aDnGRRKHeWHctAIrZY5nEjSyC+lbDF7Hh/G9RF1tOIpYf\r\n\
wnQ83bqDMP9R3i/Y36ronCUizhpSZKcmxg78azrMJqjnhHcraoP/zw3+mmZui6up\r\n\
cAvGFUXxqkB0ZBTtQUmo809ewfh4KtM+I0/5ZwIDAQABAoIBADSVCknw2llQ4iLJ\r\n\
i8nEd+/mdDPCLjFyC1DNm0Kc5MTRMUKkSSLSEfLsv1YO3pp/BSliK+G9MK9/gZgr\r\n\
Pcbq8MyincWWKQvQmCiFKr3+Vfh39G6VMSsgHrN9C6wKzljX7dxWn8s89kwyNFu9\r\n\
jnIEc+sk6nycJyCfyMFfB4xvSwgFdwRCQsZrvXFs0sLNYIaSFRQCdikgoHMesXle\r\n\
0N/t05rAKvnKWUG1YR9Hgo7zpI1QQiNSxv9QEXNFj6mnKVcNJ8OaThWfou5On65t\r\n\
Bk93S2vUZ48DJw03hlfYhUeVwlQrIqxlX5N5pOoG/xeWUcfrom3Sf01Iv8++rIjL\r\n\
Ta2SlIkCgYEA9/eY5BJi3Ny7KeNzXseqdmjbeZg4E//C6RUms8svxHvLAGiqhwfF\r\n\
VU2B1WhXm8U+iF2n6z9gnTa1+t4PZOjH3alseZLdoG1j8qmiXgwJQn0D/IzZsNig\r\n\
2CDgoOsJZRXReuYpKE5zy+xs03g9aOilqSIuONe7/LWjzlYqWdRX/R0CgYEAzbUn\r\n\
rXebxnWbUWT1nBmrZ1POvWBKjCYUqmTG/TseQKMC8nkvW+DTbu9B27mkaIdfZ4Bx\r\n\
a3hrE8EGS3/g6qGtzg3/DmzEeV09k0UvQTyrnRRkFZuiZGssdukWQvzH9RGyaPWD\r\n\
gujiUlrC/Lv5lLjyCUqvbRdi0PtJVfSxjw0NPVMCgYAvHhAiPlAk0ZiK5BpXBy+f\r\n\
4lrZ5w+41R+PNKKvBUvlVPSE9l543AQmKl0iVPpYsgko1ksDC37rQKshqTJZpVXd\r\n\
C8V98GdGhaK5SHx+zGCyDplEUutGjFM73jtwuFiHqbatWLC3ZPjh5eHj6PQaTCK2\r\n\
WbVYvb2NzmH64iqX+R5jDQKBgANqPn5ivfNNgIaZZnpw8qeEfKooLxSkjTNG+Qa7\r\n\
NIVeL25XFLIxyVDOKAm9yzzxAmR1fWyLUzvAuJoIRXOfu4LwOIvEwn5BTgRj4aTz\r\n\
nOW+sOqq6sdAADz5IaV7DNbEtHGJoeVKeHdlilcnx7zwVRRSaRcmjD1r7ou+xDPb\r\n\
w5yBAoGBAMmo0HpPZrhkwDCwjS7OUoRo5xoHLdRlUVyz+UC3W/KxBZrMoBjDD0p/\r\n\
hv7WKbOw1oLCSQvMwY1hSiy5oDzmgm5kbtFacrxyolJvkD4pz/9WCsYD0aIpb2oI\r\n\
lUGEOoUPIqpbKLYkxT6FLT7PBFpzNXq4bYxPPwZlhGG/dlflax+G\r\n\
-----END RSA PRIVATE KEY-----\r\n\
";

unsigned char ca_crt[] = "\
-----BEGIN CERTIFICATE-----\r\n\
MIIC6zCCAdMCFAm/DCJPXb6i8Rc5Ci7xgSWhfWpKMA0GCSqGSIb3DQEBBQUAMDIx\r\n\
MDAuBgNVBAoMJ1RMUyBQcm9qZWN0IERvZGd5IENlcnRpZmljYXRlIEF1dGhvcml0\r\n\
eTAeFw0yMDAzMTAwMjQxNTZaFw0zMzExMTcwMjQxNTZaMDIxMDAuBgNVBAoMJ1RM\r\n\
UyBQcm9qZWN0IERvZGd5IENlcnRpZmljYXRlIEF1dGhvcml0eTCCASIwDQYJKoZI\r\n\
hvcNAQEBBQADggEPADCCAQoCggEBALchxPTs+qqVfyS3IhNIrjl0bXsMlDLvBrMN\r\n\
rmSS3/xYxxWy5b0MjD3zNimSWQpsgyPGoXyJDeHF7BVH4aZlJyHoSqe18sDkPCPI\r\n\
WavFBPqjruumSh8t1O3AIgT1NtvIeTIlh99Xwna74O9oaRyMXC5/g3sFL4XBh1mh\r\n\
/5PK532VXjsjtaPKKztA/vRXTGtDPjnA53GRePPuCVjs2dJ6moUlUYf8lwSDTVN7\r\n\
EbBgIpQWBKTf+hNfc9JfcX+9W4tTRXfC/nNm7yY6qqhFdtjQMBs/aBQgN+4sVGVo\r\n\
0s0sn5bM7BkKJpeUXAAJo3UDKn41r9E6Ax7GO8SOiHQ5sVQLdTsCAwEAATANBgkq\r\n\
hkiG9w0BAQUFAAOCAQEAsR/Mb4zehoOgaF4keqKUBbnoYcAtLXGyalEM1Y3BOpRa\r\n\
a0evz1q4JUfFZDLGAgOvf4e5fFBE7At1NJXT6Lw22QarUfp5f7EBxChyDWo/Ot6j\r\n\
BwLLaoRpKxKORyyluw81b4weIKEcoY18P0VW59sSx2FnipIfNxVxW4Cdif5fpVin\r\n\
jsCIbBWHuB8/aNZzbub2y1lE7s62185Xw3jgX8MXCvBLPC9G8AumklU7QQOhM1RL\r\n\
Ck3FoClV235aOZxB6C+qINRxrGtvtcgZDgVXLEYkkc4kNKMtnt7Np5OOX44NuszW\r\n\
UkHLwb00nvm/1eByROi5iQtoUJhcilY87RSrP2j7lQ==\r\n\
-----END CERTIFICATE-----\r\n\
";
#endif

int mqtt_005(int argc, char **argv)
{
    hi_at_printf("start mqtt sync publication test.\r\n");
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    LOS_MEM_STATUS mem_status;
    LOS_MemInfoGet(m_aucSysMem0,&mem_status);
    hi_at_printf("start mqtt sync publication test.\r\n");
    hi_at_printf("usedSize = %d usageWaterLine %d\r\n",mem_status.usedSize,mem_status.usageWaterLine);
    LOS_MemTotalUsedGet(m_aucSysMem0);
    (void)argc;
    (void)argv;
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    cert_string keyStore = {client_crt, sizeof(client_crt)};
    cert_string trustStore = {ca_crt, sizeof(ca_crt)};
    key_string privateKey = {client_key, sizeof(client_key)};
    ssl_opts.los_keyStore = &keyStore;
    ssl_opts.los_trustStore = &trustStore;
    ssl_opts.los_privateKey = &privateKey;

    MQTTClient_create(&client, SSLADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = KEEPALIVEINTERVAL;
    conn_opts.cleansession = CLEANSESSION;
    conn_opts.ssl = &ssl_opts;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        hi_at_printf("Failed to connect, return code %d\r\n", rc);
    }
    pubmsg.payload = PAYLOAD;
    pubmsg.payloadlen = (int)strlen(PAYLOAD);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    hi_at_printf("Waiting for up to %d seconds for publication of %s\r\n"
            "on topic %s for client with ClientID: %s\r\n",
            (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    hi_at_printf("Message with delivery token %d delivered\r\n", token);
    MQTTClient_disconnect(client, TIMEOUT);
    MQTTClient_destroy(&client);
    return rc;
}

