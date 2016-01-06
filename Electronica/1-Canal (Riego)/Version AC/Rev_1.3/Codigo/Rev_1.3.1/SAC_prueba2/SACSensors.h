
#include <Time.h>
#include <OneWire.h>

/****************************************************************
SACSensors: this file contains all the functions for use the sensors and use the sensor cache. 
 for the SAC Project:
  http://saccultivo.com
   
   In this file we have the functions for use de cache sensors
  
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
 * 0.2. Added cache struct and state struct.
 * 0.3. Added Field Capacity Sensor Read.
 * Current Version: 0.3.
*************************************************************************/

#define DS19S20_PIN 13

/*
 * MOISURE POWER PIN
 */
#define SOIL_MOISTURE_POWER_PIN 5

/*
 * MOISURE PIN
 */
#define MOISTURE_PIN A3

/*
 * WATER TANK PIN
 */
#define WTS_PIN A7
#define WFL_PIN 6
#define FC_PIN A2
#define AHT_DATAPIN A0
#define AHT_CLOCKPIN A1

#define MOISTURE_SMOOTHING 85
#define MOISTURE_CALIB 500



volatile int NbTopsFan;
int Calc;
/**
* Read the field capacity in function of the previous calibration.
* Parameters:
* fcapacity_calib: field capacity calibration; the maximum value where the field is saturated.
* returns true if the field is saturated or false otherwise
*/
boolean readFieldCapacity(int fcapacity_calib){
         
         digitalWrite(SOIL_MOISTURE_POWER_PIN, HIGH);
  
  int cached_fc = analogRead(FC_PIN);
  digitalWrite(SOIL_MOISTURE_POWER_PIN, LOW);
          int fcapacity=map(cached_fc,0,fcapacity_calib,0,100);
   return fcapacity>=100;
}

void npm();
/*
* this struct store the last cached data sensor.
*/
typedef struct {
  float cached_temperature;//Last Temperature
  float cached_humidity;//Last Air Humidity
  int cached_waterlevel;//Last WaterLevel
  float cached_flowvolume; //Water Flow
  int cached_tempmin;//Last Minimum Temperature
  int cached_tempmax;//Last Maximum Temperature
  int cached_moisture;//Last Soil Moisture
  int cached_minmoisture;//Last Minimum Temperature
  int cached_maxmoisture;//Last Maximum Temperature
  int cached_waterFlowdiameter;//Water Flow Diameter
  boolean cached_fieldCapacity;//last result for the field capacity
  int cached_cicle_length;
  tmElements_t cached_lastWaterEvent;//Last Time and Date for Pumping.
  int cached_pump_percent;
  int cached_pump_cicle_seconds;
  

} cached_sensors;
/*
 * This Struct store all the information for the current state.
 */
typedef struct {

	/**
	 * the moisture target
	 */
	float moisture_target;
	/**
	 * current moisture_MIN
	 */
	float moisture_MIN;
	/**
	 * current moisture_MIN
	 */
	float moisture_MAX;
	/**
	 * current moisture
	 */
	float current_moisture;
	/*
	 * Max Soil Temperature
	 */
	float temps_max;
	/*
	 * Min Soil Temperature
	 */
	float temps_min;
	/*
	 * Current Soil Temperature
	 */
	float current_temps;
	/*
	 * Current Water Consumption in m3
	 */
	float consumption;
        int cicle_length_seconds;
	boolean field_capacity;
        

}State;


enum Interval_state{
  I_CONTINOUS=0,
  I_INTERVAL
};
/**
 *
 *read the Soilt Temperature
 * returns: the Soil Temperature in Celsius.
*/ 
float read_SoilTemp()
{
  OneWire ds(DS19S20_PIN);  // on pin A1
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;
}
 /*
 *
 * Read the Soil Moisture.
 * returns Soil Moisture in Percent.
 */
 float read_moisture(int hs_calib){
   
  digitalWrite(SOIL_MOISTURE_POWER_PIN, HIGH);
  
  float cached_moisture = analogRead(MOISTURE_PIN);
  digitalWrite(SOIL_MOISTURE_POWER_PIN, LOW);
  cached_moisture=  map(cached_moisture,0,hs_calib,0,99);
  if(cached_moisture>=100)
    cached_moisture=99;
   static float kept=0;
   float soil_moisture= cached_moisture *100/ MOISTURE_CALIB;
   kept=(kept* MOISTURE_SMOOTHING/100.0);
   kept=kept+soil_moisture*(100-MOISTURE_SMOOTHING)/100.0;
   
  return cached_moisture;
 }
 
 


 
 
 /*
 *
 * Read the WaterLevel Tank
 * return true if tank have water or false if not.
 */
 boolean getWaterLevel(){

	int WaterLevel=analogRead(WTS_PIN);
	int WaterUp=map(WaterLevel,0,602,0,1);
	//Note When We Want analog Water results, change map function
	return WaterUp>0;
}
 void setupFlowRate()
 {
   NbTopsFan=0;
   pinMode(WFL_PIN, INPUT);
   attachInterrupt(0,npm,RISING);
 }
 
 void npm()
{
  NbTopsFan++;
}
float getWaterFlowRate ()
{
    
  sei();      //Enables interrupts
  delay (1000);   //Wait 1 second
  cli();      //Disable interrupts
  Calc = (NbTopsFan * 7 / 60); //(Pulse frequency x 60) / 7Q, = flow rate in L/min
  return Calc;
  /*  Serial.print (Calc, DEC); //Prints the number calculated above*/
}




/*
 *
 * update the sensors cache with the current state.
 * Parameters:
 * last_values : last sensors values.
 * tmElements: current Time.
 * fcCapacityCalib: field Capacity calibration.
 */

void update_State(cached_sensors & last_values,tmElements_t current_event,int FCapacityCalib,byte mode,long lastInterval, boolean& cerrojo_intervalo, long& IntervalTime)
{
            float curr_moisture;
            
            if( mode==I_INTERVAL){         
            long time_aux=millis();

            if(cerrojo_intervalo==HIGH ){
            curr_moisture=read_moisture(FCapacityCalib);
            last_values.cached_moisture=curr_moisture;
            cerrojo_intervalo=LOW;
            IntervalTime=millis();
            }
            if(cerrojo_intervalo==LOW && ( time_aux - IntervalTime>=15000*60)){
            cerrojo_intervalo=HIGH;
            } 
            }          
            if(mode==I_CONTINOUS){
            curr_moisture=read_moisture(FCapacityCalib);
            last_values.cached_moisture=curr_moisture;
            }
            
	    float curr_temps= read_SoilTemp();
            
            
	    float curr_flowrate=0.0;//getWaterFlowRate();
    
    
            
	    last_values.cached_temperature=curr_temps;
	    last_values.cached_waterlevel = 0.0;//getWaterLevel(); // Boolean indicates if we have water or not.
	    last_values.cached_flowvolume+=curr_flowrate/60000;//FlowRate(L/m) to FlowRate(m3/s).
	    last_values.cached_fieldCapacity=readFieldCapacity(FCapacityCalib);

}
/*
*
* get the current state from the cached values.
* Parameters:
* last_values: cache for sensors values.
* returns: current state.
*/
State read_sensors(cached_sensors & last_values)
{
 State current_state;


	/* set the state */
	current_state.moisture_MAX=last_values.cached_maxmoisture;
	current_state.moisture_MIN=last_values.cached_minmoisture;

        current_state.current_moisture=last_values.cached_moisture;
	current_state.consumption=last_values.cached_flowvolume;
	current_state.current_temps=last_values.cached_temperature;
	current_state.moisture_target=last_values.cached_maxmoisture;
	current_state.temps_max=last_values.cached_tempmax;
	current_state.temps_min=last_values.cached_tempmin;
	current_state.field_capacity=last_values.cached_fieldCapacity;
        current_state.cicle_length_seconds=last_values.cached_cicle_length*60;
        
        
       
	return current_state; 
  
}

cached_sensors initSensorsCache(){
  cached_sensors current_sensors;
   current_sensors.cached_moisture=0.0;
   current_sensors.cached_temperature=0.0;
   current_sensors.cached_waterlevel = 0.0;//getWaterLevel(); // Boolean indicates if we have water or not.
   current_sensors.cached_flowvolume+=0.0;//FlowRate(L/m) to FlowRate(m3/s).
   current_sensors.cached_fieldCapacity=true;
   current_sensors.cached_humidity=0;
   current_sensors.cached_tempmin=0;
   current_sensors.cached_tempmax=0;
   current_sensors.cached_minmoisture=0;
   current_sensors.cached_maxmoisture=0;
   

   return current_sensors;
}
int readFCapacityValue()
{
  digitalWrite(SOIL_MOISTURE_POWER_PIN, HIGH);
  
  int cached_fc = analogRead(MOISTURE_PIN);
  digitalWrite(SOIL_MOISTURE_POWER_PIN, LOW);
  
  return cached_fc;
}
boolean state_changed(State state1,State state2)
{
   if(state1.current_moisture!= state2.current_moisture ) return true;
   if(state1.current_temps!= state2.current_temps ) return true;
   if(state1.field_capacity!= state2.field_capacity ) return true;
 
   return false;
}


