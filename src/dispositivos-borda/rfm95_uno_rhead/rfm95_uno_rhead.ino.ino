#include <TinyGPS++.h>
#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <RH_RF95.h>

TinyGPSPlus gpsplus;
SoftwareSerial ss(8, 7);

#define RF95_FREQ 915.0

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

#define node_address 3
#define gateway_address 100

typedef union
{
 char str[32];
 uint8_t bytes[4];
} FLOATUNION_t;

FLOATUNION_t data_union;

RH_RF95 rf95(RFM95_CS, RFM95_INT);

//const char *gpsStream =
//"$GPGGA,195853.580,2324.998,S,05155.693,W,1,12,1.0,0.0,M,0.0,M,,*68\r\n"
//"$GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.0,1.0,1.0*30\r\n"
//"$GPRMC,195853.580,A,2324.998,S,05155.693,W,225.5,108.6,290618,000.0,W*77\r\n"
//"$GPGGA,195854.580,2325.020,S,05155.629,W,1,12,1.0,0.0,M,0.0,M,,*65\r\n"
//"$GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.0,1.0,1.0*30\r\n"
//"$GPRMC,195854.580,A,2325.020,S,05155.629,W,162.4,109.9,290618,000.0,W*75\r\n"
//"$GPGGA,195855.580,2325.036,S,05155.583,W,1,12,1.0,0.0,M,0.0,M,,*60\r\n"
//"$GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.0,1.0,1.0*30\r\n"
//"$GPRMC,195855.580,A,2325.036,S,05155.583,W,137.5,110.2,290618,000.0,W*72\r\n"
//"$GPGGA,195856.580,2325.050,S,05155.545,W,1,12,1.0,0.0,M,0.0,M,,*69\r\n"
//"$GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.0,1.0,1.0*30\r\n"
//"$GPRMC,195856.580,A,2325.050,S,05155.545,W,137.5,110.2,290618,000.0,W*7B\r\n";

unsigned int counter = 0, valid = 0;

void setup()
{
    while (!Serial) ; // espera serial estar pronta
    Serial.begin(115200);
    ss.begin(9600);
    delay(100);
    
    Serial.println("setup complete.");
    
    pinMode(RFM95_RST, OUTPUT);
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
        while (1);
    }
    Serial.print("Set Freq to: ");
    Serial.println(RF95_FREQ);

    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
    // you can set transmitter powers from 5 to 23 dBm:
     rf95.setTxPower(14, true);
}

void send_message(FLOATUNION_t data_union, int msg_size)
{
    Serial.print("Package to send: ");
    Serial.print((char*)data_union.bytes);
    Serial.println();
    rf95.setHeaderFlags(4, 0xff);  //  Byte 4 of header is the message length. 0xff will clear all bits and replace by buffer_len.
    rf95.send(data_union.bytes, msg_size);
    rf95.waitPacketSent();
    Serial.println("SENT!");
}


String create_package()
{
  String latt, longg;
  if (gpsplus.location.isValid())
  {
    latt = String(gpsplus.location.lat(), 6);
    longg = String(gpsplus.location.lng(), 6);
  }
  else
  {
    latt = "INVALID";
    longg = "INVALID";
  }
  String result = latt + "|" + longg ;
  return result;
}

void loop()
{
    
    delay(5000);
    while (ss.available() > 0){
      if(gpsplus.encode(ss.read())){
          String message = create_package();
          message.toCharArray(data_union.str, sizeof(data_union.str));   
          Serial.println("############################################");
          Serial.print("Sending package: ");
          Serial.println(counter);
          send_message(data_union, 32);
          Serial.println("############################################");
          counter++;
      }
    }
     
  
}
