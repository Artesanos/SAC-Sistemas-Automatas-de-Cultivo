/*******************************************************************************************************

  S.A.C. Project (Automatic Cropping Systems) http://sacultivo.com started originally by Adrian Navarro.

  This file contains all the information about the functions for use the EEPROM and store the configuration.

  *** Version History:

  0.1) Initial Version by Victor Suarez (suarez.garcia.victor@gmail.com) and David Cuevas (mr.cavern@gmail.com) in March 2014.
  
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

#define CONFIG_START 25 // Initial Address in the EEPROM.

// Structure

typedef struct
{
  int active_languaje; // Active Languaje.
  int Irrigation_Mode; // Irrigation_Mode [0=Automatic, 1=Manual_ON, 2=Manual_OFF]
  int SMOp; // Soil Moisture Optimum.
  int SMmin; // Soil Moisture minimum.
  int STMax; // Maximum Soil temperature.
  int STmin; // Minimum Soil Temperature.
  int SMcalib; // SM calibration.
  long TICicle; // Total Irrigation Cicle (seconds).
  int PICicle; // Percentaje Irrigation Cicle.
  int Liters_Flow_Meter;
}

Configuration;

// Get the Default Configuration.

Configuration Default_Config()
{
  Configuration default_config;
  default_config.active_languaje = 0;
  default_config.Irrigation_Mode = 0;
  default_config.SMOp = 70;
  default_config.SMmin = 50;
  default_config.SMcalib = 500;
  default_config.STMax = 32;
  default_config.STmin = 4;
  default_config.TICicle = 20;
  default_config.PICicle = 100;
  default_config.Liters_Flow_Meter = 0;
  return default_config;
}

// Store the Settings in the EEPROM.

int store_Settings(Configuration & settings)
{
  int e = CONFIG_START;
  int i = 0;
  byte* data = (byte *)&settings;
  for (i = 1; i < sizeof(settings); i++)
  {
    byte b = data[i];
    EEPROM.write(e + i, b);
  }
  return i;
}

// Loads the Settings from the EEPROM.

int load_Settings(Configuration & settings)
{
  int e = CONFIG_START;
  int i = 0;
  byte* data = (byte *)&settings;
  for (i = 1; i < sizeof(settings); i++)
  {
    data[i] = EEPROM.read(e + i);
  }
  memcpy(&settings, data, sizeof(settings));
  return i;
}

// Loads the Default Configuration, and Store it in the EEPROM.

Configuration reset_Settings()
{
  Configuration current_config = Default_Config();
  store_Settings(current_config);
  return current_config;
}
