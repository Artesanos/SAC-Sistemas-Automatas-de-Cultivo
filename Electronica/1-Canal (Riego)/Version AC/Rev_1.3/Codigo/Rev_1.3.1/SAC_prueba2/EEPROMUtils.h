#include <EEPROM.h>
/****************************************************************
EEPROMUtils: this file contains all the information about the Configuration
 for the SAC Project:
  http://saccultivo.com
  
  IN this file there is the functions for use the EEPROM for store the 
  configuration.
  
  Author: Victor Suarez Garcia<suarez.garcia.victor@gmail.com>
  Co-Author: David Cuevas Lopez<mr.cavern@gmail.com>
  
  * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * Version History:
 * 0.1. Initial Version
 * 0.2. Change the Configuration System by using an struct for store the configuration
 in the EEPROM.
 * Current Version: 0.2.
*************************************************************************/
  
 #define CONFIG_START 25 //Initial Address in the EEPROM.
/*Struct with the configuration */
typedef struct{
  int rel1;//Relay 1 Role
  int rel2;//Relay 2 Role
  int rel3;//Relay 3 Role
  float moisture_target; //the moisture target
  float moisture_min;//Minimum Moisture target
  float temps_max;//maximum Soil temperature
  float temps_min;//Minimum Soil Temperature
  int pump_cicle_length;//Pumping Cicle Length in minutes.
  int pump_percent;//Pumping cicle irrigation percent.
  float moisture_calib;//moisture calibration
  int check_interval; //Interval Check for update the Sensors
  int flow_diameter;//Flow Diameter for Flow Size Sensor
  int active_languaje;//Active Languaje
  int calib_FCapacity;//Field capacity calibration.
  int interval_time;//IntervalTime
  int pump_cicle_seconds;
  
}Configuration;

/*
 get the default configuration.
  returns: Default Configuration.
 */
Configuration getDefaultConfig(){
 Configuration default_config;
  default_config.rel1=0;
  default_config.rel1=1;
  default_config.rel1=2;
  default_config.moisture_target=60;
  default_config.moisture_min=35;
  default_config.temps_max=35.0;
  default_config.temps_min=8.0;
  default_config.pump_cicle_length=15;
  default_config.check_interval=15;
  default_config.moisture_calib=0.0;
  default_config.flow_diameter=16;
  default_config.active_languaje=0;
  default_config.calib_FCapacity=0;

  return default_config;
}
/*
 * store the Settings in the EEPROM
 * Parameters:
 * Settings: The Settings to Store.
 * Returns: The number of bytes writed*/  
int store_Settings(Configuration & settings)
{
   int e=CONFIG_START;
   int i=0;
   byte* data=(byte *)&settings;
   for(i=1;i<sizeof(settings);i++){
     byte b=data[i];
     EEPROM.write(e+i,b);
   }
   return i;
}
/*
* loads the Settins form The EEPROM.
* Parameters
* settings: The settings struct for load the information from the EEPROM.
* returns: number of bytes readed.*/
int load_Settings(Configuration & settings)
{
  int e=CONFIG_START;
   int i=0;
   byte* data=(byte *)&settings;
   for(i=1;i<sizeof(settings);i++){
     data[i]=EEPROM.read(e+i);
   }
   memcpy(&settings,data,sizeof(settings));
   return i;
}
/**
* loads the default configuration, and store it in the EERPROM
* Returns: default configuration settings.
*/
Configuration reset_Settings(){
  Configuration current_config=getDefaultConfig();
  store_Settings(current_config);
  return current_config;
}
