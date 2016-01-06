/*******************************************************************************************************

  S.A.C. Project (Automatic Cropping Systems) http://sacultivo.com started originally by Adrian Navarro.

  This is the main file of the software to run the funtionalities of the system (Smart Irrigation System), embebed into and Arduino Pro Mini board.

  *** Funtionality:

  *** Parameters Name:

  SM=Soil Moisture.
  SMOp=Soil Moisture Optimun.
  SMmin=Soil Moisture minimun.
  SMcalib=Soil Moisture Reading for calibrating the sensor.
  FC=Field Capacity.
  STMax=Soil Temperature Maximun.
  STmin=Soil Temperature minimun.
  TICicle=Total Irrigation Cicle (seconds).
  PICicle=Percentaje Irrigation Cicle (%).

  *** Version History:

  0.9) First prototype of the SAC System and first code aproach, by Victor Amo (@) in (-> Github).
  1.0) Second prototype with first sensors and LCD, coded by Andres Orencio (andy@orencio.org) in  (-> Github).
  1.1) First 3 channel version multirole prototype. Code written, by Øyvind Kolås (pippin@gimp.org) in June 2013  (-> Github).
  1.2 & 1.3) Some Changes for more readablity, improved all the functionality for 20x4 screen (PCB 1.3), by Victor Suarez (suarez.garcia.victor@gmail.com) and David Cuevas (mr.cavern@gmail.com) in March 2014.
  1.4) Optimized, debugged, cleaned, and improved the code to run the agronomical funtionalities implemented in PCB 1.4.1, by Adrian Navarro in May 2015.

  [All PCBs designed by Adrian Navarro]

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

// Libraries.

#include <SoftwareSerial.h>
#include <SerLCD.h>
#include <Button.h>
#include <elapsedMillis.h>
#include <EEPROM.h>
#include <OneWire.h>
#include "EEPROM.h"
#include "Languages.h"
#include "Sensors.h"

// Versions of Software and Hardware.

#define FIRMWARE "1.4"
#define HARDWARE "1.4.3"
#define UEC_ID 12345678

// Global Variables.

Configuration current_config;
cached_sensors value;
State current_state;
int current_menu;
int select_language;
int current_menu_state;
int current_screen;
boolean refresh_LCD;
boolean isEditing;
boolean SM_flag = 0;

// LCD Configuration.

#define LCD_PIN 11
#define NUM_COLS 20
#define NUM_ROWS 4
#define MAXMENUITEMS 5
#define printTitle(lcd,m){lcd.print("---");lcd.print(m);lcd.print(":");}
SoftwareSerial SSerial(0, LCD_PIN);
SerLCD mylcd(SSerial, NUM_COLS, NUM_ROWS);

// RELAY Configuration.

#define RELAY_PIN 13

void relay_pin()
{
  pinMode(RELAY_PIN, OUTPUT);
}

void relay_on()
{
  digitalWrite(RELAY_PIN, HIGH);
}

void relay_off()
{
  digitalWrite(RELAY_PIN, LOW);
  LEDpump_off();
}

// LEDs Configuration.

#define LEDPump_PIN 4
#define LEDNoWater_PIN 6
#define LEDFieldC_PIN 5
int ledState_pumping = LOW;
int ledState_pump_waiting = LOW;
elapsedMillis Led_timeElapsed;

void led_pins()
{
  pinMode(LEDPump_PIN, OUTPUT);
  pinMode(LEDNoWater_PIN, OUTPUT);
  pinMode(LEDFieldC_PIN, OUTPUT);
}

/*      Pumping - LED Flashing       */

unsigned long OnTime_LED_Pumping = 3000; //2000
unsigned long OffTime_LED_Pumping = 200;

void LEDpumping()
{
  if ((ledState_pumping == HIGH) && (Led_timeElapsed > OnTime_LED_Pumping))
  {
    ledState_pumping = LOW;
    Led_timeElapsed = 0;
    digitalWrite(LEDPump_PIN, ledState_pumping);
  }
  else if ((ledState_pumping == LOW) && (Led_timeElapsed > OffTime_LED_Pumping))
  {
    ledState_pumping = HIGH;
    Led_timeElapsed = 0;
    digitalWrite(LEDPump_PIN, ledState_pumping);
  }
}

void LEDpump_on()
{
  digitalWrite(LEDPump_PIN, HIGH);
}

void LEDpump_off()
{
  digitalWrite(LEDPump_PIN, LOW);
}

/*      Pump Waiting - LED Flashing      */

unsigned long OnTime_LED_Pump_Waiting = 75;
unsigned long OffTime_LED_Pump_Waiting = 3500; //2500

void LEDpump_waiting()
{
  if ((ledState_pump_waiting == HIGH) && (Led_timeElapsed > OnTime_LED_Pump_Waiting))
  {
    ledState_pump_waiting = LOW;
    Led_timeElapsed = 0;
    digitalWrite(LEDPump_PIN, ledState_pump_waiting);
  }
  else if ((ledState_pump_waiting == LOW) && (Led_timeElapsed > OffTime_LED_Pump_Waiting))
  {
    ledState_pump_waiting = HIGH;
    Led_timeElapsed = 0;
    digitalWrite(LEDPump_PIN, ledState_pump_waiting);
  }
}

/*      No Water LED      */

void LEDNo_Water_on()
{
  digitalWrite(LEDNoWater_PIN, HIGH);
}

void LEDNo_Water_off()
{
  digitalWrite(LEDNoWater_PIN, LOW);
}

/*      Field Capacity LED     */

void LEDFieldC_Reached()
{
  digitalWrite(LEDFieldC_PIN, HIGH);
}

void LEDFieldC_off()
{
  digitalWrite(LEDFieldC_PIN, LOW);
}

// Buttons Configuration.

#define BUTTON_UP_PIN 9
#define BUTTON_CENTER_PIN 8
#define BUTTON_DOWN_PIN 7
#define TIME_BUTTON_LONG 2500
#define PULLUP true
#define INVERT false
#define DEBOUNCE_MS 20
boolean TIMEOUT;
unsigned long TimeOut_millis = 8000;
elapsedMillis TimeOut_Elapsed;
Button btnUP(BUTTON_UP_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button btnCENTER(BUTTON_CENTER_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button btnDOWN(BUTTON_DOWN_PIN, PULLUP, INVERT, DEBOUNCE_MS);

void button_pins()
{
  pinMode(BUTTON_UP_PIN, INPUT);
  pinMode(BUTTON_DOWN_PIN, INPUT);
  pinMode(BUTTON_CENTER_PIN, INPUT);
}

void button_read()
{
  btnUP.read();
  btnCENTER.read();
  btnDOWN.read();

  // TimeOut Function

  if (TimeOut_Elapsed > TimeOut_millis)
  {
    TIMEOUT = true;
    TimeOut_Elapsed = 0;
  }

  if (btnCENTER.isPressed() || btnUP.isPressed() || btnDOWN.isPressed())
  {
    TIMEOUT = false;
    TimeOut_Elapsed = 0;
  }
}

// General Setup.

void setup()
{
  // Serial.begin(9600);
  SSerial.begin(9600);
  mylcd.begin();
  button_pins();
  led_pins();
  sm_power_pin();
  flow_meter_config();
  refresh_LCD = true;
  if (!load_Settings(current_config))
  {
    current_config = reset_Settings();
  }
  global_variables();
}

// Initialize Global Variables.

void global_variables()
{
  active_language = current_config.active_languaje;
  value.cached_Irrigation_Mode = (current_config.Irrigation_Mode != 0) ? current_config.Irrigation_Mode : 0;
  value.cached_SMOp = (current_config.SMOp != 0) ? current_config.SMOp : 70;
  value.cached_SMmin = (current_config.SMmin != 0) ? current_config.SMmin : 50;
  value.cached_SMcalib = (current_config.SMcalib != 0) ? current_config.SMcalib : 500;
  value.cached_STMax = (current_config.STMax != 0) ? current_config.STMax : 32;
  value.cached_STmin = (current_config.STmin != 0) ? current_config.STmin : 4;
  value.cached_TICicle = (current_config.TICicle != 0) ? current_config.TICicle : 20;
  value.cached_PICicle = (current_config.PICicle != 0) ? current_config.PICicle : 100;
  value.cached_Liters_Flow_Meter = (current_config.Liters_Flow_Meter != 0) ? current_config.Liters_Flow_Meter : 0;
}

// Core Logic (Irrigation System).

void core_logic_irrigation (void)
{
  switch (value.cached_Irrigation_Mode)
  {
    case 0: // Irrigation_Mode = Automatic

      {
        int No_Water = digitalRead(NoWater_PIN) == HIGH;
        int ST_Sensor = current_state.ST != -1000;
        int Dangerous_Soil_Temperature = current_state.ST > current_state.STMax || current_state.ST < current_state.STmin;
        int Field_Capacity_Reached = current_state.FC >= current_state.SMOp;
        int Dry_Soil = current_state.SM < current_state.SMmin;
        int Safety_Moisture_Range = current_state.SM >= current_state.SMmin && current_state.SM <= current_state.SMOp;
        int Wet_Soil = current_state.SM > current_state.SMOp;

        if ((ST_Sensor && Dangerous_Soil_Temperature) || Field_Capacity_Reached || No_Water || Wet_Soil)
        {
          relay_off();
        }

        else
        {
          if (Dry_Soil)
          {
            Pump_pulses();
            LEDpumping();
            SM_flag = 0;
          }

          if (Safety_Moisture_Range && SM_flag == 0)
          {
            Pump_pulses();
            LEDpumping();
          }
        }

        if (Wet_Soil)
        {
          SM_flag = 1;
        }

        /*      LEDs Alerts     */

        if (Dry_Soil && ((ST_Sensor && Dangerous_Soil_Temperature) || Field_Capacity_Reached || No_Water))
        {
          LEDpump_waiting();
        }

        if (Field_Capacity_Reached)
        {
          LEDFieldC_Reached();
        }
        else
        {
          LEDFieldC_off();
        }

        if (No_Water)
        {
          LEDNo_Water_on();
        }
        else
        {
          LEDNo_Water_off();
        }
      }
      break;

    case 1: // Irrigation_Mode = Manual_On

      {
        relay_on();
        LEDpump_on();
      }
      break;

    case 2: // Irrigation_Mode = Manual_Off

      {
        relay_off();
      }
      break;
  }
}

// Pump Pulses (Duty Cicle).

int Pump_State = LOW;
elapsedMillis Pump_timeElapsed;

void Pump_pulses()
{
  unsigned long Time_Pumping = value.cached_TICicle * value.cached_PICicle * 10;
  unsigned long Time_Waiting_Pumping = value.cached_TICicle * (1000 - value.cached_PICicle * 10);

  if ((Pump_State == HIGH) && (Pump_timeElapsed > Time_Pumping))
  {
    Pump_State = LOW;
    Pump_timeElapsed = 0;
    digitalWrite(RELAY_PIN, Pump_State);
  }

  else if ((Pump_State == LOW) && (Pump_timeElapsed > Time_Waiting_Pumping))
  {
    Pump_State = HIGH;
    Pump_timeElapsed = 0;
    digitalWrite(RELAY_PIN, Pump_State);
  }

  if ((Pump_State == LOW) && value.cached_PICicle == 100)
  {
    Pump_State = HIGH;
    Pump_timeElapsed = 0;
    digitalWrite(RELAY_PIN, Pump_State);
  }
}

/*      USER LCD INTERFACE      */

// Configuration Structure Menu.

typedef struct MenuItem
{
  int label;
  int state;
};

// Screen MODES.

enum SCREEN_MODES
{
  HOME,
  SELECT,
  CONFIG_MENU,
};

// States in SELECT Screen.

enum SELECT_SCREEN
{
  S_SMOp,
  S_SMmin,
  S_TICicle,
  S_PICicle,
  S_STMax,
  S_STmin,
};

int home_screen = S_SMOp;

// Comportamiento del boton central para acceder al menu de configuracion si la Pulsacion es >3", o en caso de pulsacion corta:

// a) Entra en Modo Seleccion desde el menu HOME.
// b) Entra en Modo Edicion si se esta en Modo Seleccion.

void handleEvent()
{
  if (btnCENTER.pressedFor(2000))
  {
    mylcd.clear();
    current_menu_state = CONFIG_MENU;
    current_menu = 0;
    refresh_LCD = true;
    mylcd.underlineCursorOff();
    mylcd.boxCursorOff();
    return;
  }
  switch (current_menu_state)
  {
    case HOME:

      if (btnCENTER.pressedFor(50))
      {
        switch (value.cached_Irrigation_Mode)
        {
          case 0:
            {
              mylcd.clear();
              current_menu_state = SELECT;
              refresh_LCD = true;
              home_screen = S_SMOp;
            }
            break;
        }
      }
      break;

    case SELECT:
      EventSELECT();
      break;

    case CONFIG_MENU:
      EventCONFIG_MENU();
      break;
  }
}

// Comportamiento del boton central en Modo Seleccion que pasa a Modo Edicion (Pantalla de Inicio).

void EventSELECT()

{
  int HomeMenu_Items;

  if (current_state.ST != -1000)
  {
    HomeMenu_Items = 5;
  }

  else
  {
    HomeMenu_Items = 3;
  }

  if (btnCENTER.isPressed())
  {
    isEditing = !isEditing;
    refresh_LCD = true;
  }

  if (TIMEOUT == true)
  {
    // Store current state settings
    {
      current_config.SMOp = value.cached_SMOp;
      current_config.SMmin = value.cached_SMmin;
      current_config.STMax = value.cached_STMax;
      current_config.STmin = value.cached_STmin;
      current_config.TICicle = value.cached_TICicle;
      current_config.PICicle = value.cached_PICicle;
      store_Settings(current_config);
    }

    home_screen = S_SMOp;
    current_menu_state = HOME;
    refresh_LCD = true;
    mylcd.clear();
    mylcd.boxCursorOff();
  }

  // Comportamiento de los botones UP y DOWN en Modo Seleccion (Pantalla de Inicio).

  if (!isEditing)
  {
    if (btnUP.isPressed())
    {
      home_screen--;
      if (home_screen < 0)
      {
        home_screen = 0;
      }
      refresh_LCD = true;
    }

    if (btnDOWN.isPressed())
    {
      home_screen++;
      if (home_screen > HomeMenu_Items)
      {
        home_screen = HomeMenu_Items;
      }
      refresh_LCD = true;
    }
  }

  // Comportamiento de los botones UP y DOWN en Modo Edicion segun los diferentes casos (Pantalla de Inicio).

  else
  {
    switch (home_screen)
    {
      case S_SMOp:
        if (btnUP.isPressed())
        {
          value.cached_SMOp++;
          if (value.cached_SMOp > 99)
          {
            value.cached_SMOp = 99;
          }
          refresh_LCD = true;
        }

        // While improving LCD speed

        if (btnUP.pressedFor(TIME_BUTTON_LONG))
        {
          value.cached_SMOp += 5;
          if (value.cached_SMOp > 99)
          {
            value.cached_SMOp = 99;
          }
          refresh_LCD = true;
        }
        // End
        if (btnDOWN.isPressed())
        {
          value.cached_SMOp--;
          if (value.cached_SMOp <= value.cached_SMmin)
          {
            value.cached_SMOp = value.cached_SMmin + 1;
          }
          refresh_LCD = true;
        }

        // While improving LCD speed

        if (btnDOWN.pressedFor(TIME_BUTTON_LONG))
        {
          value.cached_SMOp -= 5;
          if (value.cached_SMOp <= value.cached_SMmin)
          {
            value.cached_SMOp = value.cached_SMmin + 1;
          }
          refresh_LCD = true;
        }
        break;
      // End

      case S_SMmin:

        if (btnUP.isPressed())
        {
          value.cached_SMmin++;
          if (value.cached_SMmin >= value.cached_SMOp)
          {
            value.cached_SMmin = value.cached_SMOp - 1;
          }
          refresh_LCD = true;
        }

        if (btnDOWN.isPressed())
        {
          value.cached_SMmin--;
          if (value.cached_SMmin < 1)
          {
            value.cached_SMmin = 1;
          }
          refresh_LCD = true;
        }
        break;

      case S_TICicle:

        if (btnUP.isPressed())
        {
          value.cached_TICicle++;
          if (value.cached_TICicle > 300)
          {
            value.cached_TICicle = 300;
          }
          refresh_LCD = true;
        }

        if (btnDOWN.isPressed())
        {
          value.cached_TICicle--;
          if (value.cached_TICicle < 1)
          {
            value.cached_TICicle = 1;
          }
          refresh_LCD = true;
        }
        break;

      case S_PICicle:

        if (btnUP.isPressed())
        {
          value.cached_PICicle++;
          if (value.cached_PICicle > 100)
          {
            value.cached_PICicle = 100;
          }
          refresh_LCD = true;
        }

        if (btnDOWN.isPressed())
        {
          value.cached_PICicle--;
          if (value.cached_PICicle < 1)
          {
            value.cached_PICicle = 1;
          }
          refresh_LCD = true;
        }
        break;

      case S_STMax:

        if (btnUP.isPressed())
        {
          value.cached_STMax++;
          if (value.cached_STMax > 60)
          {
            value.cached_STMax = 60;
          }
          refresh_LCD = true;
        }

        if (btnDOWN.isPressed())
        {
          value.cached_STMax--;
          if (value.cached_STMax <= value.cached_STmin)
          {
            value.cached_STMax = value.cached_STmin + 1;
          }
          refresh_LCD = true;
        }
        break;

      case S_STmin:

        if (btnUP.isPressed())
        {
          value.cached_STmin++;
          if (value.cached_STmin >= value.cached_STMax)
          {
            value.cached_STmin = value.cached_STMax - 1;
          }
          refresh_LCD = true;
        }

        if (btnDOWN.isPressed())
        {
          value.cached_STmin--;
          if (value.cached_STMax <= value.cached_STmin)
          {
            value.cached_STMax = value.cached_STmin + 1;
          }
          if (value.cached_STmin < 0)
          {
            value.cached_STmin = 0;
          }
          refresh_LCD = true;
        }
        break;
    }
  }
}

/*      SCREEN VIEWS      */

// Draws Interface in Status MODE and SELECTION MODE

void DrawUI()
{
  if (refresh_LCD)
  {
    switch (current_menu_state)
    {
      case HOME:
        DrawHOME();
        break;

      case SELECT:
        DrawSELECT();
        break;

      case CONFIG_MENU:
        DrawCONFIG_MENU();
        break;
    }
    refresh_LCD = false;
  }
}

// Draws HOME SCREEN.

void DrawHOME()
{
  switch (value.cached_Irrigation_Mode)
  {
    case 0:
      {
        // Line 1.

        mylcd.setPosition(1, 0);
        mylcd.print(translate(S_SM));
        if (value.cached_SMOp < 10)
          mylcd.print("0");
        mylcd.print((int)value.cached_SMOp);
        mylcd.print(" ");
        mylcd.print(translate(S_SMm));
        if (value.cached_SMmin < 10)
          mylcd.print("0");
        mylcd.print((int)value.cached_SMmin);
        mylcd.setPosition(1, 16);
        if (current_state.SM < 10)
          mylcd.print("00");
        if (current_state.SM <= 99 && current_state.SM >= 10)
          mylcd.print("0");
        mylcd.print((int)current_state.SM);
        mylcd.print("%");

        // Line 2.

        mylcd.setPosition(2, 0);
        mylcd.print(translate(S_CICLE));
        int seconds = value.cached_TICicle;
        if (seconds < 10)
          mylcd.print("00");
        if (seconds < 100 && seconds >= 10)
          mylcd.print("0");
        mylcd.print(seconds);
        mylcd.print("\42"); //Simbolo dobles comillas
        mylcd.setPosition(2, 13);
        mylcd.print(translate(S_ON));
        if (value.cached_PICicle < 10)
          mylcd.print("00");
        if (value.cached_PICicle < 100 && value.cached_PICicle >= 10)
          mylcd.print("0");
        mylcd.print(value.cached_PICicle);
        mylcd.print("%");

        // Line 3.

        if (current_state.ST != -1000)
        {
          mylcd.setPosition(3, 0);
          mylcd.print(translate(S_STMAX));
          if (value.cached_STMax < 10)
            mylcd.print("0");
          mylcd.print((int)value.cached_STMax);
          mylcd.print(" ");
          mylcd.print(translate(S_STMIN));
          if (value.cached_STmin < 10)
            mylcd.print("0");
          mylcd.print((int)value.cached_STmin);
          mylcd.print(" ");
          if (current_state.ST >= 0)
          {
            mylcd.print("+");
          }
          else
          {
            if (current_state.ST == -1000)
            {
              mylcd.print("-");
            }
          }
          mylcd.print(current_state.ST);
          mylcd.print("\337"); // Simbolo grados
        }
        else
        {
          mylcd.print("                    ");
        }

        // Line 4.

        mylcd.setPosition(4, 0);
        mylcd.print(translate(S_CONSUMPTION));
        mylcd.setPosition(4, 8);
        mylcd.print((float)value.cached_Liters_Flow_Meter / 100);
        mylcd.setPosition(4, 19);
        mylcd.print("L");
      }
      break;

    case 1:
      {
        refresh_LCD = true;
        mylcd.setPosition(1, 0);
        mylcd.print("RIEGO    INTELIGENTE");
        mylcd.setPosition(2, 0);
        mylcd.print("--------------------");
        mylcd.setPosition(3, 0);
        mylcd.print("ENCENDIDO MANUAL (!)");
        mylcd.setPosition(4, 0);
        mylcd.print("--------------------");
      }
      break;
    case 2:
      {
        refresh_LCD = true;
        mylcd.setPosition(1, 0);
        mylcd.print("RIEGO    INTELIGENTE");
        mylcd.setPosition(2, 0);
        mylcd.print("--------------------");
        mylcd.setPosition(3, 0);
        mylcd.print("      APAGADO    ");
        mylcd.setPosition(4, 0);
        mylcd.print("--------------------");
      }
      break;
  }
}

// Draws SELECT/EDIT Mode Screen.

void DrawSELECT()
{

  // Line 1.

  mylcd.setPosition(1, 0);
  mylcd.print(translate(S_SM));
  if (value.cached_SMOp < 10)
    mylcd.print("0");
  mylcd.print((int)value.cached_SMOp);
  mylcd.print(" ");
  mylcd.print("MIN:");
  if (value.cached_SMmin < 10)
    mylcd.print("0");
  mylcd.print((int)value.cached_SMmin);

  // Line 2.

  mylcd.setPosition(2, 0);
  mylcd.print(translate(S_CICLE));
  int seconds = value.cached_TICicle;
  if (seconds < 10)
    mylcd.print("00");
  if (seconds < 100 && seconds >= 10)
    mylcd.print("0");
  mylcd.print(seconds);
  mylcd.print("\42"); // Simbolo dobles comillas
  mylcd.setPosition(2, 13);
  mylcd.print(translate(S_ON));
  if (value.cached_PICicle < 10)
    mylcd.print("00");
  if (value.cached_PICicle < 100 && value.cached_PICicle >= 10)
    mylcd.print("0");
  mylcd.print(value.cached_PICicle);
  mylcd.print("%");

  // Line 3.

  if (current_state.ST != -1000)
  {
    mylcd.setPosition(3, 0);
    mylcd.print(translate(S_STMAX));
    if (value.cached_STMax < 10)
      mylcd.print("0");
    mylcd.print((int)value.cached_STMax);
    mylcd.print(" ");
    mylcd.print(translate(S_STMIN));
    if (value.cached_STmin < 10)
      mylcd.print("0");
    mylcd.print((int)value.cached_STmin);
    mylcd.setPosition(3, 16);
    mylcd.print("\337C"); // Simbolo grados
    mylcd.boxCursorOff();
    mylcd.underlineCursorOff();
  }
  else
  {
    mylcd.print("                    ");
  }

  /*     Cursor SELECT/EDIT Mode Screen     */

  switch (home_screen)
  {
    case S_SMOp:

      if (!isEditing)
      {
        mylcd.setPosition(1, 2);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(1, 5);
        mylcd.underlineCursorOn();
      }
      break;

    case S_SMmin:

      if (!isEditing)
      {
        mylcd.setPosition(1, 9);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(1, 12);
        mylcd.underlineCursorOn();
      }
      break;

    case S_TICicle:

      if (!isEditing)
      {
        mylcd.setPosition(2, 5);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(2, 9);
        mylcd.underlineCursorOn();
      }
      break;

    case S_PICicle:

      if (!isEditing)
      {
        mylcd.setPosition(2, 14);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(2, 18);
        mylcd.underlineCursorOn();
      }
      break;

    case S_STMax:

      if (!isEditing)
      {
        mylcd.setPosition(3, 4);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(3, 7);
        mylcd.underlineCursorOn();
      }
      break;

    case S_STmin:

      if (!isEditing)
      {
        mylcd.setPosition(3, 11);
        mylcd.boxCursorOn();
      }
      else
      {
        mylcd.setPosition(3, 14);
        mylcd.underlineCursorOn();
      }
      break;
  }
}

// States in Configuration MENU MODE.

enum CONFIG_MENU
{
  HOME_Config_Menu,
  IRRIGATION_MODE,
  CALIBRACION_SAT,
  FLOW_METER,
  RESET_CONFIG,
  ABOUT,
};

MenuItem config_menu[] =
{
  {S_IRRIGATION_MODE, IRRIGATION_MODE},
  {S_SATCALIBRATION, CALIBRACION_SAT},
  {S_FLOW_METER, FLOW_METER},
  {S_RESET, RESET_CONFIG},
  {S_ABOUT, ABOUT},
};

// Draws Interface for the Configuration Menu.

void DrawCONFIG_MENU()
{
  switch (current_screen)
  {
    case HOME_Config_Menu:
      DrawMenu();
      break;

    case IRRIGATION_MODE:
      DrawIrrigation_Mode();
      break;

    case CALIBRACION_SAT:
      DrawCalibrationSat();
      break;

    case FLOW_METER:
      DrawFlow_Meter();
      break;

    case RESET_CONFIG:
      DrawReset_Config();
      break;

    case ABOUT:
      DrawAbout();
      break;
  }
}

// Comportamiento de los botones en Modo Configuracion segun los diferentes casos.

void EventCONFIG_MENU()
{
  if (TIMEOUT == true)
  {
    global_variables();
    current_screen = HOME_Config_Menu;
    current_menu_state = HOME;
    refresh_LCD = true;
    mylcd.clear();
    mylcd.boxCursorOff();
  }

  switch (current_screen)
  {
    case HOME_Config_Menu:

      if (btnCENTER.isPressed())
      {
        mylcd.clear();
        current_screen = config_menu[current_menu].state;
        select_language = 0;
        refresh_LCD = true;
        isEditing = false;
      }

      if (btnDOWN.isPressed())
      {
        mylcd.clear();
        current_menu++;
        if  (current_menu == MAXMENUITEMS)
        {
          current_menu = MAXMENUITEMS - 1;
        }
        refresh_LCD = true;
      }

      if (btnUP.isPressed())
      {
        mylcd.clear();
        current_menu--;
        if  (current_menu <= 0)
        {
          current_menu = 0;
        }
        refresh_LCD = true;
      }
      break;

    case CALIBRACION_SAT:

      if (btnCENTER.pressedFor(1500))
      {
        int calib = readSMcalib();
        current_config.SMcalib = calib;
        if (current_config.SMcalib == 0)
        {
          current_config.SMcalib = 500;
        }
        store_Settings(current_config);
        mylcd.clear();
        mylcd.setPosition(2, 1);
        mylcd.print(F("SISTEMA CALIBRADO"));
        mylcd.setPosition(4, 7);
        mylcd.print(F("["));
        mylcd.print(current_config.SMcalib);
        mylcd.print(F("]"));
        delay(3000);
        TimeOut_Elapsed = 0;
        current_screen = HOME_Config_Menu;
        mylcd.clear();
      }
      refresh_LCD = true;
      break;

    case FLOW_METER:

      if (btnCENTER.pressedFor(1500))
      {
        pulses = 0.00;
        mylcd.clear();
        mylcd.setPosition(2, 0);
        mylcd.print(F(" CONTADOR REANUDADO "));
        delay(3000);
        TimeOut_Elapsed = 0;
        current_screen = HOME_Config_Menu;
        mylcd.clear();
      }
      refresh_LCD = true;
      break;

    case RESET_CONFIG:

      if (btnCENTER.pressedFor(1500))
      {
        current_config = reset_Settings();
        global_variables();
        mylcd.clear();
        mylcd.setPosition(2, 0);
        mylcd.print(F(" SISTEMA RESTAURADO"));
        delay(3000);
        TimeOut_Elapsed = 0;
        current_screen = HOME_Config_Menu;
        mylcd.clear();
      }
      refresh_LCD = true;
      break;

    case ABOUT:

      if (btnCENTER.isPressed())
      {
        current_screen = HOME_Config_Menu;
        refresh_LCD = true;
        mylcd.clear();
      }
      break;

    case IRRIGATION_MODE:

      if (btnUP.isPressed())
      {
        value.cached_Irrigation_Mode++;
        if (value.cached_Irrigation_Mode > 2)
        {
          value.cached_Irrigation_Mode = 0;
        }
        refresh_LCD = true;
      }

      if (btnDOWN.isPressed())
      {
        value.cached_Irrigation_Mode--;

        if (value.cached_Irrigation_Mode < 0)
        {
          value.cached_Irrigation_Mode = 2;
        }
        refresh_LCD = true;
      }

      if (btnCENTER.pressedFor(1500))
      {
        current_screen = HOME_Config_Menu;
        refresh_LCD = true;
        mylcd.clear();
        current_config.Irrigation_Mode = value.cached_Irrigation_Mode;
        store_Settings(current_config);
        mylcd.clear();
        mylcd.setPosition(2, 0);
        mylcd.print(F("   MODO DE RIEGO    "));
        mylcd.setPosition(3, 0);
        mylcd.print(F("    CONFIGURADO    "));
        delay(3000);
        TimeOut_Elapsed = 0;
        current_screen = HOME_Config_Menu;
        mylcd.clear();
      }
      break;
  }
}

// Draws Configuration Submenus.

void DrawMenu()
{
  mylcd.setPosition(1, 0);
  printTitle(mylcd, translate(S_CONFIGURATION));
  int position = 2;
  for (int i = current_menu; i < (current_menu + 3) && i < MAXMENUITEMS; i++)
  {
    mylcd.setPosition(position, 0);
    if (i == current_menu)
      mylcd.print("\176"); //Simbolo seleccion o \245
    mylcd.print(translate(config_menu[i].label));
    position++;
  }
}

void DrawCalibrationSat()
{
  mylcd.setPosition(1, 0);
  printTitle(mylcd, translate(S_SATCALIBRATION));
  mylcd.setPosition(3, 0);
  mylcd.print(translate(S_CURRENTVALUE));
  mylcd.print(F(": "));
  if (readSMcalib() != 0)
    mylcd.print(readSMcalib());
  else
    mylcd.print(F("---"));
  mylcd.setPosition(4, 0);
  mylcd.print(F("Pulse 'INTRO' 2 Sec"));
  refresh_LCD = true;
}

void DrawFlow_Meter()
{
  mylcd.setPosition(1, 0);
  printTitle(mylcd, translate(S_FLOW_METER));
  mylcd.setPosition(3, 0);
  mylcd.print(F("Reanudar Contador?"));
  mylcd.setPosition(4, 0);
  mylcd.print(F("Pulse 'INTRO' 2 Sec"));
  refresh_LCD = true;
}

void DrawReset_Config()
{
  mylcd.setPosition(1, 0);
  printTitle(mylcd, translate(S_RESET));
  mylcd.setPosition(3, 0);
  mylcd.print(F("Restaurar Config.?"));
  mylcd.setPosition(4, 0);
  mylcd.print(F("Pulse 'INTRO' 2 Sec"));
  refresh_LCD = true;
}

void static DrawAbout()
{
  mylcd.setPosition(1, 0);
  mylcd.print(F("FIRMWARE: "));
  mylcd.print(FIRMWARE);
  mylcd.setPosition(2, 0);
  mylcd.print(F("HARDWARE: "));
  mylcd.print(HARDWARE);
  mylcd.setPosition(3, 0);
  mylcd.print(F("UEC_ID: "));
  mylcd.print(UEC_ID);
  mylcd.setPosition(4, 0);
  mylcd.print(F("http://sacultivo.com"));
}

void DrawIrrigation_Mode()
{
  mylcd.setPosition(1, 0);
  printTitle(mylcd, translate(S_IRRIGATION_MODE));
  mylcd.setPosition(3, 0);
  if (value.cached_Irrigation_Mode == 0)
  {
    mylcd.print(F("    [AUTOMATICO]    "));
  }
  if (value.cached_Irrigation_Mode == 1)
  {
    mylcd.print(F(" [ENCENDIDO MANUAL] "));
  }
  if (value.cached_Irrigation_Mode == 2)
  {
    mylcd.print(F("     [APAGADO]      "));
  }
  mylcd.setPosition(4, 0);
  mylcd.print(F("Pulse 'INTRO' 2 Sec"));
  refresh_LCD = true;
}

// Void Loop.

void loop()
{
  update_State(value, current_config.SMcalib);
  current_state = read_sensors(value);

  if (current_menu_state == HOME)
  {
    refresh_LCD = true;
    core_logic_irrigation();
  }
  else
  {
    relay_off();
  }

  DrawUI();
  button_read();
  handleEvent();

  //Serial.println(current_menu);
}
