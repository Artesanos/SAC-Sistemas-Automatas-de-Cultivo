/*******************************************************************************************************

  S.A.C. Project (Automatic Cropping Systems) http://sacultivo.com started originally by Adrian Navarro.

  This file contains all the functions about the sensors.

  *** Version History:

  0.1) Initial Version (added cache struct, state struct and Field Capacity Sensor Read) by Victor Suarez (suarez.garcia.victor@gmail.com) and David Cuevas (mr.cavern@gmail.com) in March 2014.
  0.2) Optimized, debugged, cleaned, and improved the code by Adrian Navarro in December 2015.
  
  *** License:

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
  
*******************************************************************************************************/

#define SM_PIN A1 // Soil Moisture PIN.
#define FC_PIN A0 // Field Capacity PIN.
#define SM_POWER_PIN 2 // Soil Moisture and Field Capacity POWER PIN (ELECTRODES).
#define DS18S20_PIN 17 // Soil Temperature PIN.
#define NoWater_PIN 3 // Water available PIN (Water Level Tank or Presostate).
#define WaterFlow_PIN 16 // Water Flow PIN.

// This structure Store the Last Cached Data Sensor.

typedef struct
{
  int cached_Irrigation_Mode;
  int cached_ST; // Last Soil Temperature.
  int cached_STmin; // Last Minimum Temperature.
  int cached_STMax; // Last Maximum Temperature.
  int cached_SM; // Last Soil Moisture Value.
  int cached_SMmin; // Last Minimum Soil Moisture.
  int cached_SMOp; // Last Soil Moisture Optimum.
  int cached_SMcalib; // Last Soil Moisture Calibration.
  int cached_FC; // Last Field Capacity.
  long cached_TICicle; // Last Total Irrigation Cicle (seconds).
  int cached_PICicle; // Last Percentaje Irrigation Cicle.
  int cached_Liters_Flow_Meter; // Last Volume Flow Meter
}

cached_sensors;

// This Structure Store all the Information for the Current State.

typedef struct
{
  int Irrigation_Mode;
  int SMOp; // Soil Moisture Optimum.
  int SMmin; // Soil Moisture minimum.
  int SM; // Soil Moisture.
  int SMcalib; // Soil Moisture Calibration.
  int FC; // Field Capacity.
  int ST; // Soil Temperature.
  int STMax; // Max Soil Temperature.
  int STmin; // Min Soil Temperature.
  long TICicle; // Total Irrigation Cicle (seconds).
  int PICicle; // Percentaje Irrigation Cicle.
  int Liters_Flow_Meter; // Volume Flow Meter
}

State;

// Read the Soil Temperature (Celsius).

OneWire ds(DS18S20_PIN);

int read_ST()
{
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr))
  {
    ds.reset_search();
    return -1000;
  }
  if ( OneWire::crc8( addr, 7) != addr[7])
  {
    return -1000;
  }
  if ( addr[0] != 0x10 && addr[0] != 0x28)
  {
    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // Start Conversion, with Parasite Power on at the End
  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad
  for (int i = 0; i < 9; i++)   // We need 9 bytes
  {
    data[i] = ds.read();
  }
  ds.reset_search();
  byte MSB = data[1];
  byte LSB = data[0];
  int tempRead = ((MSB << 8) | LSB); // Using two's compliment
  int TemperatureSum = tempRead / 16;
  return TemperatureSum;
}

// Read the Soil Moisture.

void sm_power_pin()
{
  pinMode(SM_POWER_PIN, OUTPUT);
}

int read_SM(int sm_calib)
{
  digitalWrite(SM_POWER_PIN, HIGH);
  int cached_SM = analogRead(SM_PIN);
  digitalWrite(SM_POWER_PIN, LOW);      // Introducir Pulsos a los electrodos (2 opciones: a) Continuo y b) intervalo de 1' a 240') (Cuando esta en estado relay_on() pasa siempre a pulsos electr. en modo continuo)
  cached_SM =  map(cached_SM, 0, sm_calib, 0, 100);
  if (cached_SM >= 100)
    cached_SM = 100;
  int SM = cached_SM * 100;
  return cached_SM;
}

// Read the Field Capacity.

int read_FC(int sm_calib)
{
  digitalWrite(SM_POWER_PIN, HIGH);
  int cached_FC = analogRead(FC_PIN);
  digitalWrite(SM_POWER_PIN, LOW);
  cached_FC =  map(cached_FC, 0, sm_calib, 0, 100);
  if (cached_FC >= 100)
    cached_FC = 100;
  int FC = cached_FC * 100;
  return cached_FC;
}

// Reading SM Sensor for calibration (Sample Soil Moisture 100% Saturation).

int readSMcalib()
{
  digitalWrite(SM_POWER_PIN, HIGH);
  int cached_SMcalib = analogRead(SM_PIN);
  digitalWrite(SM_POWER_PIN, LOW);
  return cached_SMcalib;
}

// Read Available Water.

/*
 Original Code Written by Limor Fried/Ladyada for Adafruit Industries.
 Source: https://github.com/adafruit/Adafruit-Flow-Meter/blob/master/Adafruit_FlowMeter.pde
*/

void No_Water()
{
  pinMode(NoWater_PIN, INPUT);
}

// Read Volume Flow Meter

volatile uint16_t pulses = 0; // count how many pulses
volatile uint8_t lastflowpinstate; // track the state of the pulse pin
volatile uint32_t lastflowratetimer = 0; // keep time of how long it is between pulses
volatile float flowrate; // calculate a flow rate

SIGNAL(TIMER0_COMPA_vect) // Interrupt is called once a millisecond, looks for any pulses from the sensor
{
  uint8_t x = digitalRead(WaterFlow_PIN);

  if (x == lastflowpinstate)
  {
    lastflowratetimer++;
    return; // nothing changed!
  }

  if (x == HIGH)
  {
    pulses++; // low to high transition
  }
  lastflowpinstate = x;
  flowrate = 1000.0;
  flowrate /= lastflowratetimer;  // in hertz
  lastflowratetimer = 0;
}

void useInterrupt(boolean v)
{
  if (v)  // Timer0 is already used for millis() - we'll just interrupt somewhere in the middle and call the "Compare A" function above
  {
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
  }

  else
  {
    TIMSK0 &= ~_BV(OCIE0A); // do not call the interrupt function COMPA anymore
  }
}

void flow_meter_config()
{
  pinMode(WaterFlow_PIN, INPUT);
  digitalWrite(WaterFlow_PIN, HIGH);
  lastflowpinstate = digitalRead(WaterFlow_PIN);
  useInterrupt(true);
}

// Update the Sensors Cache with the Current State.

void update_State(cached_sensors & last_values, int SM)
{
  last_values.cached_SM = read_SM(SM);
  last_values.cached_FC = read_FC(SM);
  last_values.cached_ST = read_ST();
  last_values.cached_Liters_Flow_Meter = (pulses / (7.5 * 60)) * 100; // Liters = Pulses / (7.5 * 60)
}

// Get the Current State from the Cached Values.

State read_sensors(cached_sensors & last_values)
{
  State current_state;
  current_state.Irrigation_Mode = last_values.cached_Irrigation_Mode;
  current_state.SM = last_values.cached_SM;
  current_state.SMOp = last_values.cached_SMOp;
  current_state.SMmin = last_values.cached_SMmin;
  current_state.SMcalib = last_values.cached_SMcalib;
  current_state.FC = last_values.cached_FC;
  current_state.ST = last_values.cached_ST;
  current_state.STMax = last_values.cached_STMax;
  current_state.STmin = last_values.cached_STmin;
  current_state.TICicle = last_values.cached_TICicle;
  current_state.PICicle = last_values.cached_PICicle;
  current_state.Liters_Flow_Meter = last_values.cached_Liters_Flow_Meter;
  return current_state;
}
