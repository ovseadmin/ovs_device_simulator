#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <MQTTClient.h>
#include <json-c/json.h>

//
#define OVC_SERIAL_NUMBER           "csx13123451234500001"
#define OVC_DEVICE_TYPE             "OVC-G"
#define MQTT_CLIENT_ID              "ovs_device_01"
#define MQTT_SERVER_ADDRESS         "tcp://223.39.121.186:1883"
#define MQTT_KEEP_ALIVE_INTERVAL    120
#define MQTT_CONNECT_TIMEOUT        60
#define MQTT_MESSAGE_TIMEOUT        10000L
#define MQTT_QOS                    1
#define MQTT_USERNAME               "csx13123451234500001"
#define MQTT_PASSWORD               "csx13123451234500001"

//
volatile MQTTClient_deliveryToken deliveredtoken;

const double latitudeValue[] = {37.509141, 37.510296, 37.511334, 37.512353, 37.513743, 37.514770, 37.516314, 37.517931, 37.519846, 37.520759, 
    37.522624, 37.524782, 37.526018, 37.527895, 37.528641, 37.529740, 37.530824, 37.531986, 37.533009, 37.532616,
    37.531939, 37.531624, 37.531418, 37.530502, 37.527543, 37.526463, 37.525050, 37.523405, 37.521730, 37.520683,
    37.519330, 37.518079, 37.516871, 37.516062, 37.515373, 37.515084, 37.514828, 37.514556, 37.514016, 37.513697,
    37.513428, 37.513010, 37.511564, 37.510118, 37.508683, 37.506780, 37.507256, 37.507813, 37.508235, 37.508747};
const double longitudeValue[] = {127.063228, 127.062512, 127.061969, 127.061426, 127.060685, 127.060067, 127.059200, 127.058356, 127.057246, 127.056837, 
     127.055780, 127.054697, 127.054908, 127.056011, 127.056456, 127.057057, 127.057673, 127.058318, 127.059712, 127.061573,
     127.063994, 127.065731, 127.066625, 127.066153, 127.064765, 127.064304, 127.063639, 127.062914, 127.063069, 127.063960,
     127.065430, 127.066148, 127.066492, 127.066610, 127.066717, 127.065204, 127.063488, 127.061846, 127.059270, 127.057246,
     127.055474, 127.053350, 127.054029, 127.054846, 127.055550, 127.056604, 127.058326, 127.060275, 127.061543, 127.063367};

//
void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("       Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

////////////////////////////////////////////////
// Flow #5. Receive a V2N service message
////////////////////////////////////////////////
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {

    json_object *rxMsg, *dval;
    int msgType;

    //
    rxMsg = json_tokener_parse((char*)message->payload);
    msgType = json_object_get_int(json_object_object_get(rxMsg, "type"));

    //
     printf("Message arrived\n");
     printf("        topic: %s\n\n", topicName);
    if (msgType == 9999) {
        // notification message
        dval = json_object_object_get(rxMsg, "message");
        printf("         type: %s\n", "notification");
        printf("      message: %s\n\n", json_object_get_string(dval));
    } else {
        // v2n event message
        int tunnel, distToEvent;
        double lat, lon;

        dval = json_object_object_get(rxMsg, "tunnel");
        tunnel = json_object_get_int(dval);

        dval = json_object_object_get(rxMsg, "distanceToEvent");
        distToEvent = json_object_get_int(dval);

        dval = json_object_object_get(rxMsg, "location");
        dval = json_object_object_get(dval, "latitude");
        lat = json_object_get_double(dval);

        dval = json_object_object_get(rxMsg, "location");
        dval = json_object_object_get(dval, "longitude");
        lon = json_object_get_double(dval);
        
        printf("         type: %s\n", "v2n event alarm");
        printf("   event_code: %d\n", msgType);
        printf("       tunnel: %s\n", tunnel == 0 ? "NO" : "YES");
        printf("  distToEvent: %d\n", distToEvent);
        printf("     latitude: %f\n", lat);
        printf("    longitude: %f\n\n", lon);
    }
    
    json_object_put(rxMsg);    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);    

    return 1;
}

////////////////////////////////////////////////
// Flow #4. Publish V2N event - B
////////////////////////////////////////////////
int publishV2nEvent(MQTTClient* client, char* topic, int eventType, double lat, double lon) {

    int64_t timestamp = time(NULL);
    json_object *ovc_event = json_object_new_object();
    json_object *location = json_object_new_object();
    MQTTClient_message pubMsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token_;
    int rc;

    json_object_object_add(location, "latitude", json_object_new_double(lat));
    json_object_object_add(location, "longitude", json_object_new_double(lon));
    json_object_object_add(ovc_event, "devType", json_object_new_string(OVC_DEVICE_TYPE));
    json_object_object_add(ovc_event, "timestamp", json_object_new_int64(timestamp));
    json_object_object_add(ovc_event, "serialNo", json_object_new_string(OVC_SERIAL_NUMBER));
    json_object_object_add(ovc_event, "eventType", json_object_new_int(eventType));
    json_object_object_add(ovc_event, "location", location);

    pubMsg.payload = (void*)json_object_to_json_string(ovc_event);
    pubMsg.payloadlen = 10000;

    if ((rc = MQTTClient_publishMessage(*client, topic, &pubMsg, &token_)) 
        != MQTTCLIENT_SUCCESS) {
            printf("Failed to publish message, return code %d\n", rc);
    }

    printf("\nWaiting for up to %d seconds for publication of [%d] %s\n"
            "       on topic %s for client with ClientID: %s\n",
                (int)(MQTT_MESSAGE_TIMEOUT/1000), pubMsg.payloadlen, pubMsg.payload, topic, OVC_SERIAL_NUMBER);
    rc = MQTTClient_waitForCompletion(*client, token_, MQTT_MESSAGE_TIMEOUT);
    printf("       Message with delivery token %d delivered\n", token_);

    json_object_put(ovc_event); 

    return 1;
}


//
int main(int argc, char* argv[]) {
    
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubMsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    int seqLocation = 0;
    int szLocation = sizeof(latitudeValue) / sizeof(double);
    char topic_sub[100] = "ovs/device/";
    char topic_pub_location[100] = "ovs/location";
    char topic_pub_event[100] = "ovs/event";

    ////////////////////////////////////////////////
    // Flow #1 : Request Connection
    ////////////////////////////////////////////////
    if ((rc = MQTTClient_create(&client, MQTT_SERVER_ADDRESS, MQTT_CLIENT_ID, 
        MQTTCLIENT_PERSISTENCE_DEFAULT, NULL)) 
        != MQTTCLIENT_SUCCESS) {
            printf("Failed to create client, return code %d\n", rc);
            exit(-1);
    }

    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) 
        != MQTTCLIENT_SUCCESS) { 
            printf("Failed to set callbacks, return code %d\n", rc);
            exit(-1);
    }    

    conn_opts.keepAliveInterval = MQTT_KEEP_ALIVE_INTERVAL;
    conn_opts.cleansession = 1;
    conn_opts.username = MQTT_USERNAME;
    conn_opts.password = MQTT_PASSWORD;
    if ((rc = MQTTClient_connect(client, &conn_opts)) 
        != MQTTCLIENT_SUCCESS) {
            printf("MQTT connection fail, return code %d\n", rc);
            exit(-1);
    } 
    printf("MQTT connection success!\n");

    ////////////////////////////////////////////////
    // Flow #2. Subscribe a topic for V2N services 
    ////////////////////////////////////////////////
    strcat(topic_sub, OVC_SERIAL_NUMBER);
    if ((rc = MQTTClient_subscribe(client, topic_sub, MQTT_QOS)) 
        != MQTTCLIENT_SUCCESS) {
    	    printf("Failed to subscribe, return code %d\n", rc);
    	    exit(-1);
    }
    printf("Subscribing to topic %s\n  for client %s using QoS%d\n\n",
         topic_sub, MQTT_CLIENT_ID, MQTT_QOS);

    ////////////////////////////////////////////////
    // Flow #4. Publish V2N event - B
    ////////////////////////////////////////////////
    publishV2nEvent(&client, topic_pub_event, 0, 32.11, 126.22);

    ////////////////////////////////////////////////
    // Flow #3. Publish current location
    ////////////////////////////////////////////////
    pubMsg.qos = MQTT_QOS;
    pubMsg.retained = 0;
    while (1) {
        int64_t timestamp = time(NULL);
        json_object *ovc_location = json_object_new_object();
        json_object *location = json_object_new_object();
        
        json_object_object_add(location, "latitude", json_object_new_double(latitudeValue[seqLocation]));
        json_object_object_add(location, "longitude", json_object_new_double(longitudeValue[seqLocation]));
        json_object_object_add(ovc_location, "devType", json_object_new_string(OVC_DEVICE_TYPE));
        json_object_object_add(ovc_location, "timestamp", json_object_new_int64(timestamp));
        json_object_object_add(ovc_location, "serialNo", json_object_new_string(OVC_SERIAL_NUMBER));
        json_object_object_add(ovc_location, "speed", json_object_new_int(100));
        json_object_object_add(ovc_location, "location", location);

        pubMsg.payload = (void*)json_object_to_json_string(ovc_location);
        pubMsg.payloadlen = 10000;

        if ((rc = MQTTClient_publishMessage(client, topic_pub_location, &pubMsg, &token)) 
            != MQTTCLIENT_SUCCESS) {
                printf("Failed to publish message, return code %d\n", rc);
        }

        printf("\nWaiting for up to %d seconds for publication of [%d] %s\n"
                "on topic %s for client with ClientID: %s\n",
                    (int)(MQTT_MESSAGE_TIMEOUT/1000), pubMsg.payloadlen, pubMsg.payload, topic_pub_location, OVC_SERIAL_NUMBER);
        rc = MQTTClient_waitForCompletion(client, token, MQTT_MESSAGE_TIMEOUT);
        printf("Message with delivery token %d delivered\n", token);
                
        if (++seqLocation >= szLocation) seqLocation = 0;      
        json_object_put(ovc_location);
        
        sleep(1);
    }

    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
    	printf("Failed to disconnect, return code %d\n", rc);
    MQTTClient_destroy(&client);
   
    return 0;
}
