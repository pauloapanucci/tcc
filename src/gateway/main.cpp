
#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <vector>
#include <iomanip>

#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>

#include <RH_RF95.h>

#define BOARD_LORASPI

#include "./RadioHead/examples/raspi/RasPiBoards.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 1024

//AWS PARAMS
static char certDirectory[PATH_MAX + 1] = "aws/certs";
#define HOST_ADDRESS_SIZE 255
static char HostAddress[HOST_ADDRESS_SIZE] = AWS_IOT_MQTT_HOST;
static uint32_t port = AWS_IOT_MQTT_PORT;
static uint8_t numPubs = 5;


//RF95 PARAMS 
#define RF_FREQUENCY 915.00
#define RF_NODE_ID 1

// Create an instance of a driver
RH_RF95 rf95(RF_CS_PIN, RF_IRQ_PIN);

// int run = 1;
//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

unsigned long led_blink = 0;


/* Signal the end of the software */
void sigint_handler(int signal)
{
    printf("\n%s Break received, exiting!\n", __BASEFILE__);
    force_exit = true;
}

void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
                                const char *pReceivedJsonDocument, void *pContextData)
{
    IOT_UNUSED(pThingName);
    IOT_UNUSED(action);
    IOT_UNUSED(pReceivedJsonDocument);
    IOT_UNUSED(pContextData);

    if (SHADOW_ACK_TIMEOUT == status)
    {
        IOT_INFO("Update Timeout--");
    }
    else if (SHADOW_ACK_REJECTED == status)
    {
        IOT_INFO("Update RejectedXX");
    }
    else if (SHADOW_ACK_ACCEPTED == status)
    {
        IOT_INFO("Update Accepted !!");
    }
}

void parseInputArgsForConnectParams(int argc, char **argv)
{
    int opt;

    while (-1 != (opt = getopt(argc, argv, "h:p:c:n:")))
    {
        switch (opt)
        {
        case 'h':
            strncpy(HostAddress, optarg, HOST_ADDRESS_SIZE);
            IOT_DEBUG("Host %s", optarg);
            break;
        case 'p':
            port = atoi(optarg);
            IOT_DEBUG("arg %s", optarg);
            break;
        case 'c':
            strncpy(certDirectory, optarg, PATH_MAX + 1);
            IOT_DEBUG("cert root directory %s", optarg);
            break;
        case 'n':
            numPubs = atoi(optarg);
            IOT_DEBUG("num pubs %s", optarg);
            break;
        case '?':
            if (optopt == 'c')
            {
                IOT_ERROR("Option -%c requires an argument.", optopt);
            }
            else if (isprint(optopt))
            {
                IOT_WARN("Unknown option `-%c'.", optopt);
            }
            else
            {
                IOT_WARN("Unknown option character `\\x%x'.", optopt);
            }
            break;
        default:
            IOT_ERROR("ERROR in command line argument parsing");
            break;
        }
    }
}

void rfm95_setup()
{
    signal(SIGINT, sigint_handler);
    printf("%s\n", __BASEFILE__);

    if (!bcm2835_init())
    {
        fprintf(stderr, "%s bcm2835_init() Failed\n\n", __BASEFILE__);
        exit(1);
    }

    printf("RF95 CS=GPIO%d", RF_CS_PIN);

#ifdef RF_LED_PIN
    pinMode(RF_LED_PIN, OUTPUT);
    digitalWrite(RF_LED_PIN, HIGH);
#endif

#ifdef RF_IRQ_PIN
    printf(", IRQ=GPIO%d", RF_IRQ_PIN);
    // IRQ Pin input/pull down
    pinMode(RF_IRQ_PIN, INPUT);
    bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_DOWN);
    // Now we can enable Rising edge detection
    bcm2835_gpio_ren(RF_IRQ_PIN);
#endif

#ifdef RF_RST_PIN
    printf(", RST=GPIO%d", RF_RST_PIN);
    // Pulse a reset on module
    pinMode(RF_RST_PIN, OUTPUT);
    digitalWrite(RF_RST_PIN, LOW);
    bcm2835_delay(150);
    digitalWrite(RF_RST_PIN, HIGH);
    bcm2835_delay(100);
#endif

#ifdef RF_LED_PIN
    printf(", LED=GPIO%d", RF_LED_PIN);
    digitalWrite(RF_LED_PIN, LOW);
#endif

    if (!rf95.init())
    {
        fprintf(stderr, "Init failed\n");
        exit(1);
    }

    /* Tx Power is from +5 to +23 dbm */
    // rf95.setTxPower(23);
    rf95.setTxPower(14, false);
    rf95.setFrequency(RF_FREQUENCY);
    rf95.setThisAddress(RF_NODE_ID);
    rf95.setHeaderFrom(RF_NODE_ID);
    /* There are different configurations
     * you can find in lib/radiohead/RH_RF95.h 
     * at line 437 
     */

    // Be sure to grab all node packet
    // we're sniffing to display, it's a demo
    rf95.setPromiscuous(true);

    // We're ready to listen for incoming message
    rf95.setModeRx();

    printf( " OK NodeID=%d @ %3.2fMHz\n", RF_NODE_ID, RF_FREQUENCY );
    printf( "Listening packet...\n" );
}

std::string uint8_vector_to_hex_string(const std::vector<uint8_t>& v) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    std::vector<uint8_t>::const_iterator it;

    for (it = v.begin(); it != v.end(); it++) {
        ss << "\\x" << std::setw(2) << static_cast<unsigned>(*it);
    }

    return ss.str();
}

int loop()
{
    IoT_Error_t rc = FAILURE;
    int32_t i = 0;
    char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
    size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
    char *pJsonStringToUpdate;

    float temperature = 0.0;
    char latLong[32];
    uint8_t fromT = 0;
    uint8_t rssiT = 0;
    uint8_t toH = 0;
    uint8_t fromGPS = 0;
    uint8_t rssiGPS = 0;

    jsonStruct_t temperatureHandler;
    temperatureHandler.cb = NULL;
    temperatureHandler.pKey = "temperature";
    temperatureHandler.pData = &temperature;
    temperatureHandler.dataLength = sizeof(float);
    temperatureHandler.type = SHADOW_JSON_FLOAT;

    jsonStruct_t latLongHandler;
    latLongHandler.cb = NULL;
    latLongHandler.pKey = "latLong";
    latLongHandler.pData = &latLong;
    latLongHandler.dataLength = 32;
    latLongHandler.type = SHADOW_JSON_STRING;

    jsonStruct_t fromTempHandler;
    fromTempHandler.cb = NULL;
    fromTempHandler.pKey = "fromTemp";
    fromTempHandler.pData = &fromT;
    fromTempHandler.dataLength = sizeof(uint8_t);
    fromTempHandler.type = SHADOW_JSON_UINT8;

    jsonStruct_t fromGPSpHandler;
    fromGPSpHandler.cb = NULL;
    fromGPSpHandler.pKey = "fromLatLong";
    fromGPSpHandler.pData = &fromGPS;
    fromGPSpHandler.dataLength = sizeof(uint8_t);
    fromGPSpHandler.type = SHADOW_JSON_UINT8;

    jsonStruct_t toHandler;
    toHandler.cb = NULL;
    toHandler.pKey = "to";
    toHandler.pData = &toH;
    toHandler.dataLength = sizeof(uint8_t);
    toHandler.type = SHADOW_JSON_UINT8;

    jsonStruct_t rssiGPSHandler;
    rssiGPSHandler.cb = NULL;
    rssiGPSHandler.pKey = "rssiLatLong";
    rssiGPSHandler.pData = &rssiGPS;
    rssiGPSHandler.dataLength = sizeof(uint8_t);
    rssiGPSHandler.type = SHADOW_JSON_UINT8;

    jsonStruct_t rssiTempHandler;
    rssiTempHandler.cb = NULL;
    rssiTempHandler.pKey = "rssiTemp";
    rssiTempHandler.pData = &rssiT;
    rssiTempHandler.dataLength = sizeof(uint8_t);
    rssiTempHandler.type = SHADOW_JSON_UINT8;

    char rootCA[PATH_MAX + 1];
    char clientCRT[PATH_MAX + 1];
    char clientKey[PATH_MAX + 1];
    char CurrentWD[PATH_MAX + 1];

    IOT_INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    getcwd(CurrentWD, sizeof(CurrentWD));
    snprintf(rootCA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_ROOT_CA_FILENAME);
    snprintf(clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
    snprintf(clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);

    IOT_DEBUG("rootCA %s", rootCA);
    IOT_DEBUG("clientCRT %s", clientCRT);
    IOT_DEBUG("clientKey %s", clientKey);

    // parseInputArgsForConnectParams(argc, argv);

    // initialize the mqtt client
    AWS_IoT_Client mqttClient;

    ShadowInitParameters_t sp = ShadowInitParametersDefault;
    sp.pHost = "a2g4kcwiyy0rle.iot.us-west-2.amazonaws.com";
    sp.port = 443;
    sp.pClientCRT = "aws/certs/fd5f1c490d-certificate.pem.crt";
    sp.pClientKey = "aws/certs/fd5f1c490d-private.pem.key";
    sp.pRootCA = "aws/certs/VeriSign-Class 3-Public-Primary-Certification-Authority-G5.pem";
    sp.enableAutoReconnect = false;
    sp.disconnectHandler = NULL;

    IOT_INFO("Shadow Init");
    rc = aws_iot_shadow_init(&mqttClient, &sp);
    if (0 != rc)
    {
        IOT_ERROR("Shadow Connection Error");
        return rc;
    }

    ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
    scp.pMyThingName = "RPi-Gateway";
    scp.pMqttClientId = "c-sdk-client-id";
    scp.mqttClientIdLen = (uint16_t)strlen(AWS_IOT_MQTT_CLIENT_ID);

    IOT_INFO("Shadow Connect");
    rc = aws_iot_shadow_connect(&mqttClient, &scp);
    if (0 != rc)
    {
        IOT_ERROR("Shadow Connection Error");
        return rc;
    }

    /*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
    rc = aws_iot_shadow_set_autoreconnect_status(&mqttClient, true);
    if (0 != rc)
    {
        IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
        return rc;
    }

    rc = aws_iot_shadow_register_delta(&mqttClient, &temperatureHandler);
    rc = aws_iot_shadow_register_delta(&mqttClient, &latLongHandler);
    rc = aws_iot_shadow_register_delta(&mqttClient, &fromTempHandler);
    rc = aws_iot_shadow_register_delta(&mqttClient, &fromGPSpHandler);
    rc = aws_iot_shadow_register_delta(&mqttClient, &toHandler);
    rc = aws_iot_shadow_register_delta(&mqttClient, &rssiTempHandler);
    rc = aws_iot_shadow_register_delta(&mqttClient, &rssiGPSHandler);

    // if(0 != rc) {
    // 	IOT_ERROR("Shadow Register Delta Error");
    // }

    /* If we receive one message we show on the prompt
     * the address of the sender and the Rx power.
     */
    printf("Giving a time for receiving\n");
    sleep(2);
    printf("ready to enter in receiving loop\n");
    while (!force_exit)
    {
        // printf("Entrei no while para receber\n");
        rc = aws_iot_shadow_yield(&mqttClient, 1024);
        if (NETWORK_ATTEMPTING_RECONNECT == rc)
        {
            sleep(1);
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }
        // printf("passei da conexao\n");
        // if (rf95.available())
        // {
        // printf("Tem lora\n");
#ifdef RF_IRQ_PIN
        // We have a IRQ pin ,pool it instead reading
        // Modules IRQ registers from SPI in each loop

        // Rising edge fired ?
        // printf("Tem lora 2\n");
        if (bcm2835_gpio_eds(RF_IRQ_PIN))
        {
            // Now clear the eds flag by setting it to 1
            bcm2835_gpio_set_eds(RF_IRQ_PIN);
            printf("Packet Received, Rising event detect for pin GPIO%d\n", RF_IRQ_PIN);
#endif

            if (rf95.available())
            {
#ifdef RF_LED_PIN
                led_blink = millis();
                digitalWrite(RF_LED_PIN, HIGH);
#endif
                // Should be a message for us now
                uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
                uint8_t len = sizeof(buf);
                uint8_t from = rf95.headerFrom();
                uint8_t to = rf95.headerTo();
                uint8_t id = rf95.headerId();
                uint8_t flags = rf95.headerFlags();
                uint8_t rssi = rf95.lastRssi();

                union float_union{
                    uint8_t bytes[RH_RF95_MAX_MESSAGE_LEN];
                    float f;
                    char str[32];
                };

                float_union fu;

                if (rf95.recv(fu.bytes, &len))
                {
                    if(from == 2)
                        printf("Packet[%02d] #%d => #%d %ddB: %.2f\n", len, from, to, rssi, fu.f);
                    else if(from == 3){
                        printf("Packet[%02d] #%d => #%d %ddB: %s\n", len, from, to, rssi,fu.str);
                    }

                    toH = to;

                    if(from == 2){ //TEMPERATURE NODE
                        temperature = fu.f;
                        fromT = from;
                        rssiT = rssi;
                    }
                    else if(from == 3){ //GPS NODE
                        fromGPS = from;
                        rssiGPS = rssi;
                        strncpy(latLong, fu.str, 32);
                    }
                    
                    rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
                    if (0 == rc)
                    {
                        if(from == 2){ //TEMPERATURE NODE
                            rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 4, &temperatureHandler,
                                                         &fromTempHandler, &toHandler, &rssiTempHandler);
                        }
                        else if(from == 3){ //GPS NODE
                            rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 4, &latLongHandler,
                                                         &fromGPSpHandler, &toHandler, &rssiGPSHandler);
                        }
                        // rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 4, &temperatureHandler,
                        //                                  &fromHandler, &toHandler, &rssiHandler);
                        if (0 == rc)
                        {
                            rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
                            if (0 == rc)
                            {
                                printf("Update Shadow: %s\n", JsonDocumentBuffer);
                                IOT_INFO("Update Shadow: %s", JsonDocumentBuffer);
                                rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME, JsonDocumentBuffer,
                                                           ShadowUpdateStatusCallback, NULL, 4, true);
                            }
                        }
                        sleep(1);
                    }
                }
                else
                {
                    Serial.print("receive failed");
                }
                printf("\n");
            }

#ifdef RF_IRQ_PIN
        }
#endif

#ifdef RF_LED_PIN
        // Led blink timer expiration ?
        if (led_blink && millis() - led_blink > 200)
        {
            led_blink = 0;
            digitalWrite(RF_LED_PIN, LOW);
        }
#endif
        // Let OS doing other tasks
        // For timed critical appliation you can reduce or delete
        // this delay, but this will charge CPU usage, take care and monitor
        bcm2835_delay(5);
        // }
        usleep(1);
    }

    if (0 != rc)
    {
        IOT_ERROR("An error occurred in the loop %d", rc);
    }

    IOT_INFO("Disconnecting");
    rc = aws_iot_shadow_disconnect(&mqttClient);

    if (0 != rc)
    {
        IOT_ERROR("Disconnect error %d", rc);
    }

    return rc;
}

int main(int argc, char **argv)
{
    fprintf(stderr, "LoRa Gateway version mu1.0.0.0\n");
    fprintf(stderr, "Setting Up LoRa Module\n");
    rfm95_setup();
    fprintf(stderr, "Ready to receive!\n");
    // while (!force_exit)
    // {
    //     loop();
    //     usleep(1);
    // }
    loop();
    return EXIT_SUCCESS;
}
