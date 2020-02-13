/***********************************
 *  Author: Peter Dorich
 *  Hub code for data transfer between Adafruit.io and  
 *  the valve controller. 
 *  Data rate limit is due to Adafruit.io Free account.
 *   * OSC code based off of OPEnS lab resources. 
 ************************************/
#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>
#include <OSCBundle.h>

/* for feather32u4
  #define RFM95_CS 8
  #define RFM95_RST 4
  #define RFM95_INT 7
*/

/* for M0 */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define LED 13


#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "Ethernet2.h"    //Required for ethernet, Ethernet.h will not work

#include <Dns.h>
#include <Dhcp.h>

#define HUB_ADDRESS 30
#define RELAY_ADDRESS 31

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

//Size of message for LoRa
#define MSG_SIZE RH_RF95_MAX_MESSAGE_LEN

//IDString constructor
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x) //to concatenate a predefined number to a string literal, use STR(x)

#define FAMILY "/LOOM"
#define THIS_DEVICE "/Hub/"
#define RELAY_DEVICE "/Relay/"
#define INSTANCE_NUM 0  // Unique instance number for this device, useful when using more than one of the same device type in same space

//#define IDString FAMILY DEVICE STR(INSTANCE_NUM) // C interprets subsequent string literals as concatenation: "/Loom" "/Ishield" "0" becomes "/Loom/Ishield0"
#define MYIDString FAMILY THIS_DEVICE STR(INSTANCE_NUM) 
#define RelayIDString FAMILY RELAY_DEVICE STR(INSTANCE_NUM) 

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

RHReliableDatagram manager(rf95, HUB_ADDRESS);
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];

uint32_t x = 0;

//Struct to hold the incoming soil data
struct soil_data
{
  float VWC;
  float TEMP;
  uint32_t ELEC_COND;
  float VBAT;

};

//Struct to hold the instruction parameters
struct inst_data
{
      float inst_VWC_low = 0;
      float inst_VWC_high = 0;
      int32_t inst_start = 0;
      int32_t inst_dur = 0;
      int32_t inst_sleep = 0;
      int32_t inst_mode = 0;

};
    soil_data s_dat;
    inst_data i_dat;

int flag = 0;
OSCBundle inst_bndl;

Adafruit_MQTT_Subscribe *subscription;

/********************Ethernet Client Setup******************************/
/* This section will change depending on the user's mac address and 
 *  AIO account information. Each acccount is associated with a unique ID
 */


byte mac[] = {0x98, 0x76, 0xB6, 0x10, 0x61, 0xD6};

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "OPEnS"
#define AIO_KEY         "c3b8ceca3231410ab47418540810c1fe"

EthernetClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

#define halt(s) { Serial.println(F( s )); while(1);  }


//In order to publish data, the adafruit MQTT API can establish connections to our AIO feeds
Adafruit_MQTT_Publish Elec_Cond = Adafruit_MQTT_Publish(&mqtt,  AIO_USERNAME "/feeds/soil-data.elec-cond");
Adafruit_MQTT_Publish Temperature = Adafruit_MQTT_Publish(&mqtt,  AIO_USERNAME "/feeds/soil-data.temp");
Adafruit_MQTT_Publish VWC = Adafruit_MQTT_Publish(&mqtt,  AIO_USERNAME "/feeds/soil-data.vwc");
Adafruit_MQTT_Subscribe txtbox = Adafruit_MQTT_Subscribe(&mqtt,  AIO_USERNAME "/feeds/soil-data.txtbox");
Adafruit_MQTT_Publish VBAT = Adafruit_MQTT_Publish(&mqtt,  AIO_USERNAME "/feeds/soil-data.vbat");

boolean new_instructions;

void setup() {
  Serial.begin(9600);

  //Data rate limit delay
  delay(5000);

  Serial.println("Here.");

  //manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!manager.init())
    Serial.println("init failed");

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }

    rf95.setTxPower(23, false);

    Ethernet.begin(mac);
  
    mqtt.subscribe(&txtbox);

    new_instructions = false;
}

/*MAIN LOOP:
 * This is the recurring loop for the hub. 
 * The hub will check for new incoming instruction
 *      Won't send stale ones.
 * Uses Open Sound Control Bundles to 
 * communicate to the valve. 
 * 
 */
void loop() {
  
  MQTT_connect();

  unsigned long lora_timer = millis();
  int x = 0;
  while (!manager.available() && (millis() - lora_timer < 10000)) 
  {
    x++;
    while ((subscription = mqtt.readSubscription(250))) 
    {
      if (subscription == &txtbox)
      {
        Serial.print(F("Got: "));
        Serial.println((char *)txtbox.lastread);
        get_inst_data((char *)txtbox.lastread);
        new_instructions = true;
      }
    }
  }
  
  if (manager.available()) 
  {
    uint8_t len = sizeof(buf);
    uint8_t from;
    memset(buf, '\0', RH_RF95_MAX_MESSAGE_LEN);
    if (manager.recvfromAck(buf, &len, &from)) 
    {
      OSCBundle bndl;
      get_OSC_bundle((char*)buf, &bndl);
      Serial.println((char*)buf);
      String str;

      char inst_mess[MSG_SIZE];
      memset(inst_mess, '\0', MSG_SIZE);

      /* TODO Test values. */
         str = (char*) txtbox.lastread;
         Serial.println(str.toInt());
        // inst_timer = str.toInt();

      //----------------------------------------
      //------ Pack and send instructions ------
      //----------------------------------------
      inst_bndl.empty();

      // Add desired instructions to bundle. Remember to handle on receiving end. /
      //Build Instruction Bundle for the Relay
      //OSC LIBRARY:
      inst_bndl.add(MYIDString "/mode_inst").add((int32_t) i_dat.inst_mode);
      inst_bndl.add(MYIDString "/vwc_low_inst").add((float)i_dat.inst_VWC_low);
      inst_bndl.add(MYIDString "/vwc_high_inst").add((float)i_dat.inst_VWC_high);
      inst_bndl.add(MYIDString "/start_inst").add((int32_t) i_dat.inst_start);
      inst_bndl.add(MYIDString "/dur_inst").add((int32_t) i_dat.inst_dur);
      inst_bndl.add(MYIDString "/sleep_inst").add((int32_t) i_dat.inst_sleep);

      get_OSC_string(&inst_bndl, inst_mess);
      
      if(new_instructions){
        if (manager.sendtoWait((uint8_t*)inst_mess, strlen(inst_mess), RELAY_ADDRESS)) {
          Serial.println("Instructions sent.");
          new_instructions = false;
        } else {
          Serial.println("Instruction sending failed -- TODO/Resend?");
        }
      } else {
        Serial.println("No new instructions to pass.");
      }
      //----------------------------------------
      //------ End of instruction passing ------
      //----------------------------------------
      
      Serial.print("[Publishing data] - ");
      
      //Info from soil struct gets published to Adafruit.io
      Serial.print("Elec_Cond: ");
      if (! Elec_Cond.publish(s_dat.ELEC_COND)) {
        Serial.print(F("Failed "));
      } else {
        Serial.print(F("OK "));
      }

      Serial.print("Temp: ");
      if (! Temperature.publish((char *) String(s_dat.TEMP).c_str())) {
        Serial.print(F("Failed "));
      } else {
        Serial.print(F("OK "));
      }

      Serial.print("VWC: ");
      if (! VWC.publish((char *) String(s_dat.VWC).c_str())) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK"));
      }

      Serial.print("VBAT: ");
      if (! VBAT.publish((char *) String(s_dat.VBAT).c_str())) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK"));
      }

      Serial.println("\n\n");
    }
  }

}
