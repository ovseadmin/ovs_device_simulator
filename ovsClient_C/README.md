# OVS Device Simulator
본 시뮬레이터는 SKT의 OVS(Open V2X Service) 플랫폼 프로토콜을 따르는 Smartphone, BlackBox, ADAS 등의 단말 동작을 나타내는 시뮬레이터입니다.
본 시뮬레이터는 OVS 플랫폼의 [기술문서](https://ovs-document.readthedocs.io/en/latest/index.html)를 기반으로 구성되어 있습니다. 상세한 프로토콜은 링크의 기술문서를 참고하세요.  

<br>

*****

## OVS Device Simulator Introduction

#### Device Simulator 구성
본 시뮬레이터는 `VS Code`를 통해 `C언어` 기반으로 구현되어 있으며, 기술 규격에 따라 MQTT 프로토콜에 준수하여 개발이 되어 있습니다. OVS 플랫폼 프로토콜에서 제공하는 단말 타입은 `ovc-g`, `ovc-m` 이며 해당 문서는 `ovc-g` 기준으로 설명되어 있습니다. 아래의 파일에 시뮬레이션 과정이 코딩되어 있습니다.
 * ovs_device.c

<br>

#### Device Simulator 실행 방법
본 시뮬레이터를 동작을 위해서는 기본적으로 `paho.mqtt.c`, `json-c`가 설치되어 있어야 합니다. 
`paho.mqtt.c`의 설치 방법은 **[Github - paho.mqtt.c](https://github.com/eclipse/paho.mqtt.c)**, `json-c`의 설치 방법은 **[Github - json-c](https://github.com/json-c/json-c)** 를 참고하시길 부탁 드립니다.

라이브러리 참조는 본 Repository의 `/.vscode/tasks.json` 파일을 참고하시기 바랍니다.

<br>

#### Device Simulator 설정 방법
본 시뮬레이터 동작을 위한 설정은 본 Repository의 `ovs_device.c` 파일에 정의되어 있으며, 해당 설정을 통해 각자의 상황에 맞추어 시뮬레이션을 수행할 수 있습니다.
아래 코드의 내용 중에서 수정이 필요한 사항은 다음과 같습니다.

Key                     | Description 
:-----------------------|:------------------------ 
**OVC_SERIAL_NUMBER**     | OVC 단말을 식별할 수 있는 식별자입니다. 자세한 내용은 OVS 플랫폼의 [기술문서](https://ovs-document.readthedocs.io/en/latest/procedure.html)를 참조하시기 바랍니다. 
**OVC_DEVICE_TYPE**       | 시뮬레이션을 돌리고자 하는 디바이스 타입을 명시합니다. `ovc-g`, `ovc-m` 
**MQTT_SERVER_ADDRESS**   | MQTT Broker 접속을 위한 주소입니다. 자세한 내용은 OVS 플랫폼의 [기술문서](https://ovs-document.readthedocs.io/en/latest/procedure.html)를 참조하시기 바랍니다. 
**MQTT_CLIENT_ID**        | MQTT Broker에서 OVC 단말을 식별할 수 있는 식별자입니다. 
**MQTT_USERNAME**         | MQTT Broker 접속에 필요한 Username입니다. OVS 플랫폼의 [기술문서](https://ovs-document.readthedocs.io/en/latest/procedure.html)를 참조하여 발급받습니다. 
**MQTT_PASSWORD**         | MQTT Broker 접속에 필요한 Password입니다. OVS 플랫폼의 [기술문서](https://ovs-document.readthedocs.io/en/latest/procedure.html)를 참조하여 발급받습니다. 
**MQTT_QOS**              | 단말이 OVS Platform에 전송하는 메시지 QoS를 명시합니다.<br/> 서비스 품질 보장을 위해 반드시 **1** 이상의 값이 설정되어야 합니다.

<br>

*****

## OVS Device Simulator Flow
본 시뮬레이터는 아래의 Flow를 기반으로 작성되어 있으며, 아래의 Flow는 [단말 프로시저 규격](https://ovs-document.readthedocs.io/en/latest/procedure.html)과 [단말 전송 메시지 규격](https://ovs-document.readthedocs.io/en/latest/message_format.html)을 참고 바랍니다.

 <center><img src = ./image/Flow_ovcg.png alt="drawing" width="50%"></center>

제공해드리는 단말 시뮬레이터의 코드에는 위 Flow의 각 순서와 대응되는 주석이 표기되어 있습니다. 아래의 동작 예시 설명을 통해 자세히 알아보도록 하겠습니다.

<br>

#### Device Simulator 정상 동작 예시
1. 정상적으로 설정된 Device Simulator의 동작 예시입니다. MQTT 연결에 필요한 `connetionOptions` 설정하여 MQTT object `client`를 생성합니다.

~~~ c
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
~~~

<br>

2. `client`가 `serialNo`를 이용하여 생성한 `topic`으로 MQTT Broker에 Subscribe 기능을 수행합니다.

~~~ c
////////////////////////////////////////////////
// Flow #2. Subscribe a topic for V2N services 
////////////////////////////////////////////////
char topic_sub[100] = "ovs/device/";
strcat(topic_sub, OVC_SERIAL_NUMBER);
if ((rc = MQTTClient_subscribe(client, topic_sub, MQTT_QOS)) 
    != MQTTCLIENT_SUCCESS) {
	    printf("Failed to subscribe, return code %d\n", rc);
	    exit(-1);
}
printf("Subscribing to topic %s\n  for client %s using QoS%d\n\n",
     topic_sub, MQTT_CLIENT_ID, MQTT_QOS);
~~~

<br>

3. `client`가 Publish하는 데이터는 크게 2개 종류로 구분됩니다. 첫 째, 주기적으로 전송하는 **위치데이터** 와 둘 째, V2N 이벤트가 발생했을 때 전송하는 **V2N 이벤트 데이터** 입니다. 각각은 `topic`에 따라 구분됩니다.

~~~ c
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

~~~

<br>

4. `client`가 Subscribe하고 있는 `topic`에 의해 메세지를 수신한 경우 사용자에게 알림 메세지를 전달합니다. 수신하는 메세지의 종류는 크게 2개 종류로 구분됩니다. 첫 째, 특정 단말에게 **정보성 메세지**를 전달하는 메세지와 둘 째, 긴급제동알림 서비스, 전방 낙하물 주의 등 **V2N 서비스 메세지** 입니다. 

~~~ c
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
~~~
