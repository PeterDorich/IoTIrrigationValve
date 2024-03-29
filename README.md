
# IoT Irrigaton Valve System: 

![project_overview](/project_documents/Valve_System.png)


The project was split into three equal parts. 
1.The Valve Logic.
2:The Communications Hub.
3:The user interface.

This repo will only contain my portion of the project, The Communications Hub.



*******************************************************************************
Original README from project. Details how to setup the Hub.
*******************************************************************************
README for the Communication Hub


Author: Peter Dorich

Purpose: Access point for Valve to connect to via Long Range Radio signals.
         The intention is to have 1 hub for every farm. One hub can operate many different valves. There hasn't been testing to determine how many yet.
         The hub will send instructions to the valve and publish valve data to Adafruit.io
         
Required Hardware: Feather M0 board with Long range radio
                    https://www.adafruit.com/product/3178
                   Ethernet Featherwing
                    https://www.adafruit.com/product/3201
                    
Required Software: Arduino IDE (Latest Version)

Required Arduino Libraries: Ethernet2 
                            OSCBundle Library (Belongs to OPEnS)
                            RH_RF95 
                            Adafruit_MQTT
                            RadioHead 
                            
HOW TO SETUP HUB FOR YOUR SYSTEM:

  1. First you need to retrieve your Username and AIO key from your associated Adafruit.io account.
  2. These numbers will be inputed on Line 103 and 104, respectively. Each value will be put in between quotation marks.
            NOTE: You do not need to change the AIO_SERVER or the AIO_SERVERPORT
  3. Next, input a valid MAC address for your device to connect to Ethernet on line 99.
            NOTE: Format: "{0x98, 0x98,.. 0x98};
            
  4. Setup your feeds. This is done starting on line 114. You can add as many feeds as you'd like. 
            NOTE: Must match exact naming scheme on Adafruit.io
                  DO NOT edit the txtbox subsciption.
            Only edit the feeds if you're trying to setup an entirely new system. All feeds related to the Irrigation valve are already setup.
            
      
CODE DOCUMENTATION: 
         The first thing of note for the code is the difference between the 32u4 and the M0 boards.
         Depending on which on you are using, you need to set different definitions for ports.
         
         
        Next the Struct named "soil_data" is defined and declared globally for easy access. This struct is where incoming soil data is stored.
        The same can be said for the "inst_data" structure, which holds instruction data from Adafruit.io.
        
        In the setup() method, initial pre-processing instructions are executed to start the micro-controller board. 
                  These includes setting up the Radio frequency, done in "setTXPower(), which sets the transmitter powerlevel from 5 to 23.
                  The Ethernet.begin function is also called using the defined mac address in order to execute the connection. 
                  A new_instructions flag is also set to false initially.
                  
        In the "loop()" function is where the real logic happens. Everything discussed will be repeated.
                  The first thing done is to attempt to connect to MQTT, which retries to connect every 5 seconds if failed. 
                  Next, the Hub checks to see if there are any new instructions from the subscription "txtbox".
                   If there is, the data is read in by the hub and the function "get_inst_data" is called from OSC_interpret.ino.
                     This method uses the knowledge of the formatted string to assign each value to its own variable.
                        Next, the new instruction flag is set to true indicating that a new message needs to be sent to the hub.
                  After that, the hub checks to see if the valve is trying to connect to send data. 
                      If it is, essentially the same steps are performed as I just went over. 
                          The difference is the addition of the OSC library for this step.
                          The incoming soil data OSC bundle is broken down using the "get_OSC_bundle" method, 
                                             This method breaks down the values from the string using intentional delimiters such as "f" for a float value, and so on.
                                             Once each value is removed from the string, it is added to the soil_data structure.
                   Once the soil_data structure is filled, each value is published using our defined feeds from the beginning of the program.
                           Each feed is associated with 1 value, so the VWC has its own feed, and so on.
                   Then, the loop executes again starting with the MQTT connection.
FILES:
         Hub_V0.1.ino: Actual hub code, with the setup methods and loop that I just went over.
         OSC_interpret.ino: All of the code that sifts through the OSC bundles and assigns values to the structs. Includes the get_osc_bundle method and the get_inst_data method 
         mqtt_utils.ino: File with the MQTT_Connect() method that returns if the connection is still alive and attempts to reconnect every 5 seconds if not.
         
