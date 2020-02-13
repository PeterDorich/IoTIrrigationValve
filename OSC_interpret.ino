/***********************************
 * Author: Peter Dorich 
 * Back-end for send/ recieve data between hub/ valve
 * This file deals with the behind the scenes parsing of
 * bundles and strings. 
 * OSC code based off of OPEnS lab resources. 
 ************************************/

#include <string.h>

union data_vals {
  int32_t i;
  float f;
  uint32_t u;
};

void get_OSC_string(OSCBundle *bndl, char *string) {
  char buf[MSG_SIZE];
  char type;
  int n = 0;
  data_vals value;
  OSCMessage* msg = bndl->getOSCMessage(n);
  
  while(msg != NULL) {
    msg->getAddress(buf, 0);
    type = msg->getType(0);
    
    strcat(string, buf);

    if (type == 'f') {
      value.f = msg->getFloat(0);
  
      snprintf(buf, MSG_SIZE, " f%lu", value.u);
      strcat(string, buf);
    }
    else if (type == 'i') {
      value.i = msg->getInt(0);

      snprintf(buf, MSG_SIZE, " i%lu", value.u);
      strcat(string, buf);
    }
    n++;
    msg = bndl->getOSCMessage(n);
    if (msg != NULL) strcat(string, " ");
  }
}


/*get_inst_data() wil parse out the instruction string from
 * the adafruit.io subscription.
 * 
 * OSC bundles were too much for MQTT and Adafruit to handle, 
 * so this string isn't parsed out like a bundle. 
 * Format is: //"mode/low/high/start_dur/inst_dur"
 */
struct inst_data get_inst_data(char *string)
{
//"mode/low/high/start_dur/inst_dur"

  int x;

  Serial.println((char*) string);
  char buf[MSG_SIZE];
  char *p = buf;
  char *addr = NULL, *value = NULL;
  strcpy(buf, string);
  int count = 0;
  addr = strtok(string, "/");
  while(addr != NULL){
    //Serial.println(addr);
    if(count == 0){
        i_dat.inst_mode = atoi(addr);
    }
    if(count == 1){
          i_dat.inst_VWC_low = atof(addr);
    }
    if(count == 2){
          i_dat.inst_VWC_high = atof(addr);
    }
    if(count == 3){
        i_dat.inst_start = atoi(addr);
    }
    if(count == 4){
        i_dat.inst_dur = atoi(addr);
    }
    if(count == 5){
        i_dat.inst_sleep = atoi(addr);
    }
    addr = strtok(NULL, "/");
    count ++;
  }


}

/* get_OSC_bundle will parse out the OSC bundle that is recieved. 
 *  Incoming soil data order is known: VWC, Temp, VBat, Elec_cond.
 *  Adds each value to the soil struct (s_dat)
 *  get_OSC_bundle() returns this struct. 
 */
struct soil_data get_OSC_bundle(char *string, OSCBundle* bndl) {
  bndl->empty();
  data_vals value_union;
  char buf[MSG_SIZE];
  char *p = buf;
  char *addr = NULL, *value = NULL;
  strcpy(buf, string);
  addr = strtok_r(p, " ", &p);
  int count = 0;
  int elec_cond = 0;
  float vwc = 0;
  float temp = 0;
  float vbat = 0;
  while (addr != NULL & strlen(addr) > 0) {
    value = strtok_r(NULL, " ", &p);
    value_union.u = strtoul(&value[1], NULL, 0);
    if (value[0] == 'f') {
      bndl->add(addr).add(value_union.f);
      if(count == 0){
         vwc = value_union.f;
         Serial.print("The VWC is: ");
         Serial.println(vwc);
         s_dat.VWC = vwc;
      //publish to MQTT/adafruit
      }
      if(count == 1){
         temp = value_union.f;
         Serial.print("The Temperature in C is: ");
         Serial.println(temp);
         s_dat.TEMP = temp;
      }
      if(count == 3){
        vbat = value_union.f;
        Serial.print("The VBattery in C is: ");
        Serial.println(vbat);
        s_dat.VBAT = vbat;
      }
    }
    else if (value[0] == 'i') {
      bndl->add(addr).add(value_union.i);

      elec_cond = value_union.i;
      Serial.print("The electrical conductivity is: ");
      Serial.println(elec_cond);
      s_dat.ELEC_COND = elec_cond;
    }

    addr = strtok_r(p, " ", &p);
    count++;
    
  }
  return s_dat;
}

