/*
 * Relay.h: This file contains all the configuration for the relays and their states
 * and different roles.
 *
 *  Created on: 10/04/2014
 *      Author: David Cuevas
 *      Co-Author: victor suarez
 * * This library is free software; you can redistribute it and/or
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
 * Vesion History:
 * 0.1. Initial Version 
 * 
 * Current Version: 0.1
 */
 

#include "Arduino.h"

#define MAX_RELAYS 1
#define RELAY1_PIN 2
#define RELAY2_PIN 3
#define RELAY3_PIN 4

  enum RelayState {
    RELAY_OFF=0,
    RELAY_ON,
    RELAY_WAIT
 };

typedef struct _Relay Relay;

  struct _Relay {
  byte gpio_pin;
  byte role;
  byte state;
 };
 
enum Roles{
  R_DISCONNECTED=0,
  R_IRRIGATION,
  R_HUMIDIFIER,
  R_VENTILATION,
  R_HEATING,
  R_COOLING,
  R_LIGHT,
};

void relay_on (byte relayPin)
{
  


    digitalWrite(relayPin, HIGH);

}



void relay_off (byte relayPin)
{
  
  

  digitalWrite(relayPin, LOW);

}


