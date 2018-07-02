#include <Arduino.h>
#include <Wire.h>
#include <SSD1306.h>
#include <OLEDDisplayUi.h>

#include <SPI.h>
#include <RH_RF95.h>

#define RF95_FREQ 915.0

#define RFM95_CS 18
#define RFM95_RST 14
#define RFM95_INT 26

#define data_pin 13

RH_RF95 rf95(RFM95_CS, RFM95_INT);

// OLED pins to ESP32 GPIOs via this connecthin:
#define OLED_ADDRESS 0x3c
#define OLED_SDA 4  // GPIO4
#define OLED_SCL 15 // GPIO15
#define OLED_RST 16 // GPIO16

#define SCK 5      // GPIO5 - SX1278's SCK
#define MISO 19    // GPIO19 - SX1278's MISO
#define MOSI 27    // GPIO27 - SX1278's MOSI
#define SS 18      // GPIO18 - SX1278's CS
#define RST 14     // GPIO14 - SX1278's RESET
#define DI0 26     // GPIO26 - SX1278's IRQ (interrupt request)
#define BAND 915E6 // 915E6

SSD1306 display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
OLEDDisplayUi ui(&display);

#define LED 2

#define node_address 2
#define gateway_address 100

typedef union
{
 float number;
 uint8_t bytes[4];
} FLOATUNION_t;

FLOATUNION_t data_union;

unsigned int counter = 0;


void setup()
{
    //while (!Serial);
    Serial.begin(115200);
    delay(100);
    // put your setup code here, to run once:
    Serial.println("setup complete.");

    pinMode(LED, OUTPUT);

    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW); // low to reset OLED
    delay(50);
    digitalWrite(OLED_RST, HIGH); // must be high to turn on OLED

    display.init();
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_10);
    
    pinMode(RFM95_RST, OUTPUT);
    pinMode(data_pin, INPUT);
    digitalWrite(RFM95_RST, HIGH);

    // manual reset
    digitalWrite(RFM95_RST, LOW);
    delay(10);
    digitalWrite(RFM95_RST, HIGH);
    delay(10);

     //  Set the addresses of this device and the gateway.
    rf95.setThisAddress(node_address);
    rf95.setHeaderFrom(node_address);
//    rf95.setHeaderTo(gateway_address);
//    rf95.setHeaderId(0); //  Byte 3 of header is the message counter.

    while (!rf95.init())
    {
        Serial.println("LoRa radio init failed");
        while (1)
            ;
    }
    Serial.println("LoRa radio init OK!");
    // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
    if (!rf95.setFrequency(RF95_FREQ))
    {
        Serial.println("setFrequency failed");
        while (1)
            ;
    }
    Serial.print("Set Freq to: ");
    Serial.println(RF95_FREQ);

    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
    // you can set transmitter powers from 5 to 23 dBm:
    rf95.setTxPower(23, false);
    data_union.number = 0.0;
}

void send_message(FLOATUNION_t data_union, int msg_size)
{
    Serial.print("Package to send: ");
    Serial.print(data_union.number);
    Serial.println();
    rf95.setHeaderFlags(msg_size, 0xff);
    rf95.send(data_union.bytes, msg_size);
    rf95.waitPacketSent();
    Serial.println("SENT!");
}

void loop()
{
    delay(5000);
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    display.drawString(0, 0, "Sending packet:");
    display.drawString(90, 0, String(counter));
    display.display();
    Serial.println("############################################");
    Serial.print("Sending package: ");
    Serial.println(counter);
    
    data_union.number = analogRead(data_pin);
    send_message(data_union, 4);
    Serial.println("############################################");
    counter++;

    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
}

