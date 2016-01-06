#include <SoftwareSerial.h>
#include <SerLCD.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DS1307RTC.h>
#include <Wire.h>
#include <Time.h>



/****************************************************************
 * SAC: This is the Main File of SAC Project:
 * http://saccultivo.com
 * 
 * IN this file we can see all the functions for the correct functionality of the SAC Unit for 3 Output Channel
 * 
 * Author: Victor Suarez Garcia<suarez.garcia.victor@gmail.com>
 * Co-Author: David Cuevas Lopez<mr.cavern@gmail.com>
 * 
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
 * 1.1. Initial Version. Written by Øyvind Kolås pippin@gimp.org in 2013.
 * 1.2. Some Changes for more readablity. written by victor suarez suarez.garcia.victor@gmail.com and David Cuevas mr.cavern@gmail.com in March 2014.
 * 1.3. Improved all the Functionality for 20x4 Screen and improved for 1.3 PCB Version. written by victor suarez suarez.garcia.victor@gmail.com and David Cuevas mr.cavern@gmail.com in April 2014
 * 
 * 
 * Current Version: 1.3.
 *************************************************************************/



#include "RTCUtils.h"
#include "EEPROMUtils.h"
#include "languages.h"
#include "SACSensors.h"
#include "Relay.h"
/*
*LCD CONFIG & PINS
 */
#define LCD_PIN 11
#define NUM_COLS 20
#define NUM_ROWS 4
#define MAXMENUITEMS 6
#define magia(lcd,x) {  lcd.setPosition(1,0); lcd.print(x);delay(5000);}
#define printTitle(lcd,m){lcd.print("***");lcd.print(m);lcd.print("***");}
/*
 * BUTTONS PINS
 */
#define BUTTON_UP_PIN 9
#define BUTTON_CENTER_PIN 8
#define BUTTON_DOWN_PIN 7

#define BUTTONUP 0
#define BUTTONDOWN 1
#define BUTTONCENTER 2
#define BUTTONCENTERLONG 3
#define BUTTONUPLONG 4
#define BUTTONDOWLONG 5
#define TIMEOUT 6

#define IDLE -1

#define TOTALTIMEOUTTIME 10000

#define VERSION 1.3


/*
 * DIFFERENT STATES TO MOVE THROUGH MENU
 */
enum States
{
  INICIO,
  CALIB_SAT,
  ESTADO,
  EDICION,
  SELECCION,

};
/*
 * DIFFERENT STATES IN SELECTION MODE
 */
enum Seleccion_States
{
  MENU,
  FECHA,
  HORA,
  CALIBRACION_SAT,
  RESET_CONFIG,
  ABOUT,
  END_SELECTION
};
/*
 * DIFFERENT STATES IN SELECTION MODE FOR DATE MENU
 */
enum selectionDateStates
{
  DAY,
  MONTH,
  YEAR,
  SAVE,
  BACK
};
/*
 * DIFFERENT STATES IN SELECTION MODE FOR TIME MENU
 */
enum s_selectionTimeStates
{
  S_SELECTHOURS,
  S_SELECTMINUTES,
  S_SAVETIME,
  S_BACKTIME 
};
enum s_selectStatus
{
  S_HSO=0,
  S_HSMIN,
  S_PCICLE,
  S_PCICLESECONDS,
  S_PINTERVAL,
  S_TSMAX,
  
};

/*
 * MENU STRUCTURE 
 */
typedef struct MenuItem
{
  int label;
  int  state; 
};
MenuItem main_menu[] ={
  {
    S_DATE,FECHA          }
  ,
  {
    S_HOUR,HORA          }
  ,{
    S_SATCALIBRATION,CALIBRACION_SAT          }
  ,
  {
    S_RESET,RESET_CONFIG          }
  ,{
    S_ABOUT,ABOUT      }
  ,{
    S_RETURN_TO,END_SELECTION          }

};

int current_menu;

int select_language;
/*
 * LCD PIN SETUP & CONFIG
 */
SoftwareSerial SSerial(0,LCD_PIN);
SerLCD mylcd(SSerial,NUM_COLS,NUM_ROWS);
/*
 * VALUES, STATES & CONFIG
 */
boolean actualizar_pantalla;
Configuration current_config;
tmElements_t tm;
cached_sensors current_sensorsvalues;
State current_state;
State previous_state;

/*
 * RELAY CONFIG
 *EACH RELAY USES A ROLE TO CONTROL IT'S FUNCTIONS
 */


Relay relay[MAX_RELAYS]={
  {
    RELAY1_PIN,R_IRRIGATION,RELAY_OFF       }
};
/*
 * GLOBAL VARIABLES
 */
tmElements_t lastUpdate;
boolean irrigating;
byte current_mstate;
byte current_selectionstate;
byte current_selectionDateState;
byte cTime=S_SELECTHOURS;
byte button_up_state=LOW;
byte button_down_state=LOW;
byte button_center_state=LOW;
byte center_pressed_state=0;
byte up_pressed_state=0;
int selectionStatus=S_HSO;
byte editHours;
byte editMinutes;
byte editDays;
byte editMonths;
byte interval_mode=I_INTERVAL;
int current_rele=0;
int editYears;
boolean isEditing;
long time1;
long time2;

long time1I;
long time2I;
long time3I;
long IntervalTime;
long lastEvent;



boolean cerrojo_up=1;
boolean cerrojo_center=1;
boolean cerrojo_down=1;

boolean cerrojo_intervalo=HIGH;
//--------------------------------------------------------------------------
//SACLCD saclcd(mylcd);
void setup_pins();
void static drawSelectStatus(State & state);

void setup()
{
  Serial.begin(9600);
  SSerial.begin(9600);
  mylcd.begin();
  setup_pins();
  actualizar_pantalla=true;
  // setupFlowRate();

  irrigating=false;
  current_selectionstate=MENU;
  if(!load_Settings(current_config)){
    current_config=reset_Settings();
    current_mstate=SELECCION;
  }
  else{
    current_mstate=ESTADO; 
  }
  initializeGlobalVars();
}
/*Initialize Global Variables. */
void initializeGlobalVars(){
  active_language =current_config.active_languaje;
  current_sensorsvalues.cached_tempmax=(current_config.temps_max!=0)?current_config.temps_max:35;
  current_sensorsvalues.cached_tempmin=(current_config.temps_min!=0)?current_config.temps_min:8;
  current_sensorsvalues.cached_minmoisture=(current_config.moisture_min!=0)?current_config.moisture_min:30;
  current_sensorsvalues.cached_maxmoisture=(current_config.moisture_target!=0)?current_config.moisture_target:70;
  current_sensorsvalues.cached_cicle_length=(current_config.pump_cicle_length!=0)?current_config.pump_cicle_length:15;
  current_sensorsvalues.cached_pump_percent=(current_config.pump_percent!=0)?current_config.pump_percent:100;
  current_sensorsvalues.cached_pump_cicle_seconds=(current_config.pump_cicle_seconds!=0)?current_config.pump_cicle_seconds:0;
  time1=millis();
}
/**
 * setup SAC Pins.
 */
void setup_pins(){
  pinMode(BUTTON_UP_PIN,INPUT);
  pinMode(BUTTON_DOWN_PIN,INPUT);
  pinMode(BUTTON_CENTER_PIN,INPUT);

  pinMode(SOIL_MOISTURE_POWER_PIN, OUTPUT);
  for(int i=0;i<MAX_RELAYS;i++)
  {
    pinMode(relay[i].gpio_pin,OUTPUT);

  }
  current_config.interval_time=15;
  isEditing=false;
}
void loop(){
  RTCread(tm);

  //ALWAYS UPDATE SCREEN WHEN STATE CHANGES
  update_State(current_sensorsvalues,tm,current_config.calib_FCapacity,interval_mode, current_config.interval_time, cerrojo_intervalo, IntervalTime);
  current_state=read_sensors(current_sensorsvalues);
  if(current_mstate==ESTADO){
    if(state_changed(current_state,previous_state) || time_between(lastUpdate,tm)>1){
      actualizar_pantalla=true; 
      previous_state=current_state;
      lastUpdate=tm;
    }
    
  }


  drawUI(current_state);
  int event=get_event();



  handleEvent(event);
  update_relay_state();
  

}

/*Gets the current Button Event
 * returns: the button that is pressed.
 *  0: button UP is Pressed.
 *  1: button DOWN is Pressed.
 *  2: button CENTER is Pressed minor than 3 seconds.
 *  3: button CENTER is Pressed more than 3 seconds.
 */
int get_event(){
  int event=IDLE;
  event=button_up_pressed();

  if(event!=IDLE){  
  //  Serial.println(event); 
    return event;
  }
  event=button_down_pressed();
  //Serial.println(event);
  if(event!=IDLE){ 
   // Serial.println(event); 
    return event;
  }
  time2=millis();
  event=button_center_pressed();
  if(event>=0){
    time1=millis(); 
  }
  if((time2-time1)>=TOTALTIMEOUTTIME){
    time1=millis();
    event= TIMEOUT;
  }
  Serial.println(event);
  return event;

}
/*
* Check if button up is pressed. The Event only ocurrs when the button is released; not
 * when button is pressed.
 returns HIGH when button Up is released or LOW otherwise.
 */

int button_up_pressed()
{
  byte current_state=digitalRead(BUTTON_UP_PIN);

  if(current_state==HIGH && cerrojo_up==1)
  {  
    cerrojo_up=0;
    time1I=millis();
    return BUTTONUP;
  }
  if(current_state==LOW && cerrojo_up==0)
  {
    cerrojo_up=1;
    time1I=millis();
  }

  if(current_state==HIGH && cerrojo_up==0){
    long TIME_UP=millis();
    if( (TIME_UP- time1I) >= 3000){

      return BUTTONUPLONG;
    }
  }
  return IDLE;

}


/*
* check if button down is pressed. The Event only ocurrs when the Button is released. NOt when
 * the button is Pressed.
 * returns HIGH when the Button is released or LOW otherwise.
 */
int button_down_pressed()
{
  boolean current_state=digitalRead(BUTTON_DOWN_PIN);


  if(current_state==HIGH && cerrojo_down==1){
    time3I=millis();
    cerrojo_down=0;
    return BUTTONDOWN;
  }

  if(current_state==LOW && cerrojo_down==0 ){
    cerrojo_down=1;   
  }

  if(current_state==HIGH && cerrojo_down==0){
    long TIME_UP=millis();
    if( (TIME_UP- time3I) >= 3000){

      return BUTTONDOWLONG;
    }
  }
  return IDLE;
}
/*
* check if button center is pressed with two states. When is pressed minor than 3 secods or if its pressed
 more than 3 seconds. The event only ocurr when the button is released. not when is pressed.
 returns: -1 if is not released, 2 if button is pressed minor than three seconds or 3 if  button is pressed
 more than 3 seconds.
 */
int button_center_pressed()
{
  boolean current_state=digitalRead(BUTTON_CENTER_PIN);

  if(current_state==HIGH && cerrojo_center ==1)
  {  
    cerrojo_center=0;
    time2I=millis();
    return BUTTONCENTER;
  }
  if(current_state==LOW && cerrojo_center==0)
  {
    cerrojo_center=1;
    time2I=millis();

  }

  if(current_state==HIGH && cerrojo_center==0){
    long TIME_center=millis();
    if(TIME_center-time2I>= 3000){
      time2I=millis();
      return BUTTONCENTERLONG;
    }
  }
  return IDLE;

}

/*
 * HANDLE EVENTS FOR STATE SCREEN & SELECT MODE
 * CASE ESTADO: STATUS SCREEN, DEFAULT VIEW.
 * CASE SELECCION: SELECTION STATE, CALLS THE EVENT HANDLER FOR THIS STATE
 */
void handleEvent(int event)
{
  if(event==BUTTONCENTERLONG){
    //Clear LCD Screen
    mylcd.clear();
    current_mstate=SELECCION;
    current_menu=0; 
    actualizar_pantalla=true;
    mylcd.underlineCursorOff();
    mylcd.boxCursorOff();
    return;
  }
  switch(current_mstate)
  {
  case ESTADO:

    if(event==BUTTONCENTER)
    {
      mylcd.clear();
      current_mstate=EDICION;
      actualizar_pantalla=true;
      selectionStatus=S_HSO;
    }

    break; 
  case SELECCION:

    handleEventSelection(event);

    break;
  case EDICION:
    handleEventRoleEditionStatus(event);
    break;


  }
}

void handleEventRoleEditionStatus(int event)
{
  switch(relay[current_rele].role)
  {
  case R_IRRIGATION:
    handleEventSelectStatus(event);
    break;


  } 
}
void handleEventSelectStatus(int event)
{
  if(event==BUTTONCENTER)
  {
    isEditing=!isEditing;
    actualizar_pantalla=true; 
  }
  if(!isEditing){
    if(event==BUTTONDOWN)
    {
      selectionStatus--;
      if(selectionStatus<0){
        selectionStatus=5;
      }
      actualizar_pantalla=true;
    } 
    if(event==BUTTONUP)
    {
      selectionStatus++;
      if(selectionStatus>5){
        selectionStatus=0;
      }
      actualizar_pantalla=true;
    }

    if(event==TIMEOUT)
    {

      selectionStatus=S_HSO;
      current_mstate=ESTADO;
      actualizar_pantalla=true;
      mylcd.clear();
      mylcd.boxCursorOff(); 
      //Store current state settings
      current_config.moisture_target=current_sensorsvalues.cached_maxmoisture;
      current_config.moisture_min=current_sensorsvalues.cached_minmoisture;
      current_config.temps_max=current_sensorsvalues.cached_tempmax;
      current_config.temps_min=current_sensorsvalues.cached_tempmin;
      current_config.pump_cicle_length=current_sensorsvalues.cached_cicle_length;
      current_config.pump_percent=current_sensorsvalues.cached_pump_percent;
      current_config.pump_cicle_seconds=current_sensorsvalues.cached_pump_cicle_seconds;
      store_Settings(current_config);
    }

  }
  else{

    switch(selectionStatus)
    {
    case S_HSO:
      if(event==BUTTONDOWN){
        current_sensorsvalues.cached_maxmoisture--;
        if(current_sensorsvalues.cached_maxmoisture<current_sensorsvalues.cached_minmoisture){
          current_sensorsvalues.cached_maxmoisture=99;
        }



        actualizar_pantalla=true;
      }
      if(event==BUTTONUP){
        current_sensorsvalues.cached_maxmoisture++;
        if(current_sensorsvalues.cached_maxmoisture>99){
          current_sensorsvalues.cached_maxmoisture=current_sensorsvalues.cached_minmoisture;
        }


        actualizar_pantalla=true;
      }
      if(event==BUTTONUPLONG){
        current_sensorsvalues.cached_maxmoisture+=10;
        if(current_sensorsvalues.cached_maxmoisture>99){
          current_sensorsvalues.cached_maxmoisture=current_sensorsvalues.cached_minmoisture;
        }
        actualizar_pantalla=true;
      }
      if(event==BUTTONDOWLONG){
        current_sensorsvalues.cached_maxmoisture-=10;
        if(current_sensorsvalues.cached_maxmoisture<00){
          current_sensorsvalues.cached_maxmoisture=99;
        }


        actualizar_pantalla=true;
      }
      break;
    case S_HSMIN:
      if(event==BUTTONDOWN){
        current_sensorsvalues.cached_minmoisture--;
        if(current_sensorsvalues.cached_minmoisture<0){
          current_sensorsvalues.cached_minmoisture=100;
        }
        actualizar_pantalla=true;
      }
      if(event==BUTTONDOWLONG){
        current_sensorsvalues.cached_minmoisture-=10;
        if(current_sensorsvalues.cached_minmoisture<0){
          current_sensorsvalues.cached_minmoisture=100;
        }
        actualizar_pantalla=true;
      }
      if(event==BUTTONUP){
        current_sensorsvalues.cached_minmoisture++;
        if(current_sensorsvalues.cached_minmoisture>99){
          current_sensorsvalues.cached_minmoisture=0;
        }       
        actualizar_pantalla=true;
      }
      if(event==BUTTONUPLONG){
        current_sensorsvalues.cached_minmoisture+=10;
        if(current_sensorsvalues.cached_minmoisture>99){
          current_sensorsvalues.cached_minmoisture=0;
        }       
        actualizar_pantalla=true;
      }
      break;
    case S_PCICLE:
      if(event==BUTTONDOWN)
      {
        current_sensorsvalues.cached_cicle_length--;
        if(current_sensorsvalues.cached_cicle_length<0){
          current_sensorsvalues.cached_cicle_length=120;
        }
        actualizar_pantalla=true;
      }
      if(event==BUTTONUP)
      {
        current_sensorsvalues.cached_cicle_length++;
        if(current_sensorsvalues.cached_cicle_length>120){
          current_sensorsvalues.cached_cicle_length=0;
        }
        actualizar_pantalla=true;  
      }  
      if(event==BUTTONDOWLONG){
        current_sensorsvalues.cached_cicle_length-=10;
        if(current_sensorsvalues.cached_cicle_length<0){
          current_sensorsvalues.cached_cicle_length=120;
        }   

        actualizar_pantalla=true;
      }  

      break;
    case S_PINTERVAL:
      if(event==BUTTONDOWN)
      {
        current_sensorsvalues.cached_pump_percent--;
        if(current_sensorsvalues.cached_pump_percent<0){
          current_sensorsvalues.cached_pump_percent=100;
        }   
        actualizar_pantalla=true;
      }
      if(event==BUTTONUP)
      {
        current_sensorsvalues.cached_pump_percent++;
        if(current_sensorsvalues.cached_pump_percent>100){
          current_sensorsvalues.cached_pump_percent=0;
        }   
        actualizar_pantalla=true;  
      }
      if(event==BUTTONUPLONG){
        current_sensorsvalues.cached_pump_percent+=10;
        if(current_sensorsvalues.cached_pump_percent>100){
          current_sensorsvalues.cached_pump_percent=0;
        }       
        actualizar_pantalla=true;
      }
      if(event==BUTTONDOWLONG){
        current_sensorsvalues.cached_pump_percent-=10;
        if(current_sensorsvalues.cached_pump_percent<0){
          current_sensorsvalues.cached_pump_percent=100;
        }       
        actualizar_pantalla=true;
      }
      break;
      
    case S_TSMAX:
      if(event==BUTTONUP){
        current_sensorsvalues.cached_tempmax++;
        if(current_sensorsvalues.cached_tempmax>55){
          current_sensorsvalues.cached_tempmax=0;
        } 
        actualizar_pantalla=true;
      }
      if(event==BUTTONDOWN){
        current_sensorsvalues.cached_tempmax--;
        if(current_sensorsvalues.cached_tempmax<0){
          current_sensorsvalues.cached_tempmax=55;
        } 
        actualizar_pantalla=true;
      }
      if(event==BUTTONUPLONG){
        current_sensorsvalues.cached_tempmax+=10;
        if(current_sensorsvalues.cached_tempmax>55){
          current_sensorsvalues.cached_tempmax=0;
        }           
        actualizar_pantalla=true;
      }
      if(event==BUTTONDOWLONG){
        current_sensorsvalues.cached_tempmax-=10;
        if(current_sensorsvalues.cached_tempmax<0){
          current_sensorsvalues.cached_tempmax=50;
        }           
        actualizar_pantalla=true;
      }      

      break;
   case S_PCICLESECONDS:
  if(event==BUTTONUP){
        current_sensorsvalues.cached_pump_cicle_seconds++;
        if(current_sensorsvalues.cached_pump_cicle_seconds>59){
          current_sensorsvalues.cached_pump_cicle_seconds=0;
        } 
        actualizar_pantalla=true;
      }
      if(event==BUTTONDOWN){
        current_sensorsvalues.cached_pump_cicle_seconds--;
        if(current_sensorsvalues.cached_pump_cicle_seconds<0){
          current_sensorsvalues.cached_pump_cicle_seconds=59;
        } 
        actualizar_pantalla=true;
      }
      if(event==BUTTONUPLONG){
        current_sensorsvalues.cached_pump_cicle_seconds+=10;
        if(current_sensorsvalues.cached_pump_cicle_seconds>59){
          current_sensorsvalues.cached_pump_cicle_seconds=0;
        }           
        actualizar_pantalla=true;
      }
      if(event==BUTTONDOWLONG){
        current_sensorsvalues.cached_pump_cicle_seconds-=10;
        if(current_sensorsvalues.cached_pump_cicle_seconds<0){
          current_sensorsvalues.cached_pump_cicle_seconds=59;
        }           
        actualizar_pantalla=true;
      }
    
  break;
    }
  }

}

/*
 * EVENT HANDLER FOR EACH SELECTION STATE
 * MENU: MOVE & INTERACT WITH MENU
 * IDIOMA: LANGUAGE SELECTION MENU
 * FECHA: DATE CONFIG MENU
 * HORA: TIME CONFIG MENU
 * END SELECTION: EXITS SELECTION STATE
 */
void handleEventSelection(int event)
{
  switch(current_selectionstate)
  {
  case MENU:

    if(event == BUTTONCENTER)
    {
      mylcd.clear();
      current_selectionstate=main_menu[current_menu].state;
      select_language=0;
      current_selectionDateState=0;
      actualizar_pantalla=true;
      editHours=tm.Hour;
      editMinutes=tm.Minute;
      editDays=tm.Day;
      editMonths=tm.Month;
      editYears=tmYearToCalendar(tm.Year);
      isEditing=false;
      // current_selectTimeState=SELECTHOURS;


    }
    if(event==BUTTONDOWN)
    {
      mylcd.clear();
      current_menu++;
      current_menu= current_menu%MAXMENUITEMS;
      actualizar_pantalla=true;
    }
    if(event==BUTTONUP)
    {
      mylcd.clear();
      current_menu=(current_menu==0)? MAXMENUITEMS-1: current_menu-1;
      current_menu= current_menu%MAXMENUITEMS;
      actualizar_pantalla=true;
    }
    break;


  case FECHA:
    if(isEditing)
      handleEventEditingDate(event);
    else
      handleEventSelectionDate(event);

    break;
  case HORA:
    if(isEditing) 
      handleEventEditingHour(event);
    else
      handleEventSelectionHour(event);
    break;
  case CALIBRACION_SAT:
    if(event==BUTTONCENTER)
    {
      int calib=readFCapacityValue();
      current_config.calib_FCapacity=calib;
      store_Settings(current_config);
      current_selectionstate=MENU;
      interval_mode=I_INTERVAL;
      mylcd.clear();
    }else{
     interval_mode=I_CONTINOUS;
    }
    
    actualizar_pantalla=true;
    break;
  case RESET_CONFIG:

    current_config=reset_Settings();
    initializeGlobalVars();
    current_mstate=SELECCION;
    current_selectionstate=MENU;
    actualizar_pantalla=true;
    break;
  case ABOUT:
    if(event==BUTTONCENTER)
    {

      current_selectionstate=MENU;
      actualizar_pantalla=true;
      mylcd.clear();
    }
    break;
  case END_SELECTION:
    current_mstate=ESTADO;
    current_selectionstate=MENU;
    mylcd.clear();
    actualizar_pantalla=true;
    break;
  }
}
void handleEventSelectionDate(int event)
{
  switch(current_selectionDateState)
  {


  default:

    if(event==BUTTONUP)
    {
      current_selectionDateState--;
      current_selectionDateState%=5;
      actualizar_pantalla=true;
    } 
    if(event==BUTTONDOWN)
    {
      current_selectionDateState++;
      current_selectionDateState%=5;
      actualizar_pantalla=true;
    }
    if(event==BUTTONCENTER)
    {
      isEditing=true;
      actualizar_pantalla=true;
      mylcd.clear(); 
    }
    if(current_selectionDateState==SAVE && event==BUTTONCENTER)
    {
      current_selectionstate=MENU;
      actualizar_pantalla=true;
      current_menu=0;
      mylcd.clear();
      tm.Day=editDays;
      tm.Month=editMonths;
      tm.Year=CalendarYrToTm(editYears);
      setHour(tm);
    }
    if(current_selectionDateState==BACK && event==BUTTONCENTER)
    {
      current_selectionstate=MENU;
      actualizar_pantalla=true;
      current_menu=0;
      mylcd.clear();
    }
    break;
  }


}
void handleEventSelectionHour(int event)
{
  switch(cTime)
  {
  case S_SAVETIME:
    if(event==BUTTONCENTER)
    {
      tm.Hour=editHours;
      tm.Minute=editMinutes;
      setHour(tm);
      current_selectionstate=MENU;
      actualizar_pantalla=true;
      current_menu=0;
      mylcd.clear();
      cTime=S_SELECTHOURS;
      break;
    }
  case S_BACKTIME:
    if(event==BUTTONCENTER){
      current_selectionstate=MENU;
      actualizar_pantalla=true;
      current_menu=0;
      mylcd.clear();
      cTime=S_SELECTHOURS;
      break;
    }

  default:
    if(event==BUTTONDOWN)
    {
      actualizar_pantalla=true;
      cTime++;
      cTime%= 4; 

    }
    if(event==BUTTONUP)
    {
      actualizar_pantalla=true;
      cTime--;
      cTime= (cTime<0)?4:cTime%4; 

    }
    if(event==BUTTONCENTER)
    {
      actualizar_pantalla=true;
      isEditing=true;
      mylcd.clear();
    }
    break;
  } 
}
void handleEventEditingDate(int event)
{
  switch(current_selectionDateState)
  {
  case DAY:
    if(event==BUTTONDOWN){
      editDays++;
      editDays%=getDaysofMonth(editMonths);
      editDays=(getDaysofMonth(editMonths)>31)?editDays=1:editDays;
      actualizar_pantalla=true;
    }
    if(event==BUTTONUP){
      editDays--;
      editDays%=getDaysofMonth(editMonths);
      editDays=(getDaysofMonth(editMonths)<1)?editDays=getDaysofMonth(editMonths):editDays;
      actualizar_pantalla=true;
    }
    if(event==BUTTONCENTER){
      isEditing=false;
      actualizar_pantalla=true;
      mylcd.clear();
    }
    break;
  case MONTH:
    if(event==BUTTONDOWN){
      editMonths++;
      editMonths=(editMonths%12)+1;
      editMonths=(editMonths>12)?editMonths=1:editMonths;
      actualizar_pantalla=true;
    }
    if(event==BUTTONUP){
      editMonths--;
      editMonths=(editMonths%12)+1;
      editMonths=(editMonths<1)?editMonths=12:editMonths;
      actualizar_pantalla=true;
    }
    if(event==BUTTONCENTER){
      isEditing=false;
      actualizar_pantalla=true;
      mylcd.clear();
    }
    break;
  case YEAR:
    if(event==BUTTONDOWN){
      editYears++;
      editYears=(editYears>2032)?editYears=2000:editYears;  
      actualizar_pantalla=true;
    }
    if(event==BUTTONUP){
      editYears--;
      editYears=(editYears<2000)?editYears=2032:editYears;
      actualizar_pantalla=true;
    }
    if(event==BUTTONCENTER){
      isEditing=false;
      actualizar_pantalla=true;
      mylcd.clear();
    }
    break;
  default:

    break; 
  }

}
void handleEventEditingHour(int event)
{
  switch(cTime)
  {
  case S_SELECTHOURS:
    if(event==BUTTONDOWN){
      editHours++;
      editHours%= 24;
      actualizar_pantalla=true;
    }
    if(event==BUTTONUP){
      editHours--;
      editHours=(editHours<0)?23:editHours%24;
      actualizar_pantalla=true;
    }
  case S_SELECTMINUTES:
    if(event==BUTTONDOWN){
      editMinutes++;
      editMinutes%= 60;
      actualizar_pantalla=true;
    }
    if(event==BUTTONUP){
      editMinutes--;
      editMinutes=(editMinutes<0)?259:editMinutes%60;
      actualizar_pantalla=true;
    }
  default:
    if(event==BUTTONCENTER){
      isEditing=false;
      actualizar_pantalla=true;
      mylcd.clear();
    }
    break; 
  }

}

/*
 * DRAWS  INTERFACE IN STATUS MODE & SELECTION MODE
 */
void drawUI(State & state){
  if(actualizar_pantalla){
    switch(current_mstate)
    {
    case SELECCION:
      drawSeleccion();
      break;
    case  ESTADO:

      drawState(state);
      break;
    case EDICION:
      drawSelectStatus(state);
      break;

    }
    actualizar_pantalla=false;
  }

}
void drawState(State & state)
{
  switch(current_rele)
  {
  case 0:
    drawIrrigationState(state);
    break;
  }
}



/*
 * DRAWS INTERFACE FOR THE DIFFERENT MENUS
 */
void drawSeleccion()
{

  switch(current_selectionstate)
  {
  case MENU:
    drawMenu();
    break;
  case FECHA:
    if(!isEditing)
      drawDate();
    else
      drawEditingDate(current_selectionDateState);
    break;
  case HORA:
    if(!isEditing)
      drawTime();
    else
      drawEditingTime(cTime);
    break;
  case CALIBRACION_SAT:
    drawCalibrationSat();
    break;
  case ABOUT:
    drawAbout();
    break;
  }

}


/*
 * DRAWS MENU
 */
void drawMenu()
{
  mylcd.setPosition(1,0);
  printTitle(mylcd,translate(S_MAIN_MENU));
  int position=2;

  for(int i=current_menu; i<(current_menu+3) && i<MAXMENUITEMS;i++)
  {

    mylcd.setPosition(position,0);
    if(i==current_menu)
      mylcd.print("*");
    mylcd.print(translate(main_menu[i].label)); 
    position++;
    //  Serial.println(i);
  }
}

/**
 * draws the current state at LCD Screen
 * state: current state of sensors.
 */
void drawIrrigationState(State & state)
{
  //Line1
  mylcd.setPosition(1,0);
  if(tm.Hour<10) mylcd.print("0");
  mylcd.print(tm.Hour);
  mylcd.print(":");
  if(tm.Minute<10) mylcd.print("0");
  mylcd.print(tm.Minute);
  mylcd.print("  ");
  if(tm.Day<10) mylcd.print("0");
  mylcd.print(tm.Day);
  mylcd.print("/");
  if(tm.Month<10) mylcd.print("0");
  mylcd.print(tm.Month);
  mylcd.print("/");
  mylcd.print(tmYearToCalendar(tm.Year));
  if(state.field_capacity)
  { 
    mylcd.print(" ");
    mylcd.print(translate(S_FC));
  }
  else
  {
    mylcd.print("   "); 
  }
  //Line2
  mylcd.setPosition(2,0);
  mylcd.print(translate(S_S));
  if(state.moisture_MAX<10)
    mylcd.print("0");
  mylcd.print((int)state.moisture_MAX);
  mylcd.print(" ");
  mylcd.print(translate(S_MIN));
  if(state.moisture_MIN<10)
    mylcd.print("0");
  mylcd.print((int)state.moisture_MIN);
  mylcd.print(" ");
  mylcd.print("[");
  if(state.current_moisture<10)
    mylcd.print("0");
  mylcd.print((int)state.current_moisture);
  mylcd.print("");
  mylcd.print("]%");
  if(state.current_moisture<=99)
    mylcd.print(" ");



  //line3
  mylcd.setPosition(3,0);
  mylcd.print(translate(CICLO));
  int minutes=state.cicle_length_seconds/60;
  if(minutes<100)
    mylcd.print("0");
  if(minutes<10)
    mylcd.print("0");
  mylcd.print(minutes);
  mylcd.print("'");
byte seconds= current_sensorsvalues.cached_pump_cicle_seconds;
  if(seconds<10)
    mylcd.print("0");
  mylcd.print(seconds);
  mylcd.print("''");
  mylcd.print("ON");
  if(current_sensorsvalues.cached_pump_percent<100)
    mylcd.print("0");
  if(current_sensorsvalues.cached_pump_percent<10)
    mylcd.print("0");
  mylcd.print(current_sensorsvalues.cached_pump_percent);
  mylcd.print("%");

  //line4
  mylcd.setPosition(4,0);
  mylcd.print(translate(ST_MAX));
  if(state.temps_max<10)
    mylcd.print("0");
  mylcd.print((int)state.temps_max);
  mylcd.print(" ");
  int currenttemp=state.current_temps;
  if(currenttemp>=0){
    mylcd.print("+"); 
  }
  else{
    if(currenttemp==-1000){
      mylcd.print("-");
    } 
  }
  if(currenttemp!=-1000){
    mylcd.print(currenttemp);
  }
  else{
    mylcd.print("--"); 
  }
  mylcd.print("C");
  //Serial.println(line1);

  //Serial.println(total);
  //LCD_Message(&mylcd,line1,line2,line3,"bb");
}


/*
 * ALWAYS LISTENS TO RELAYS STATES IN EACH ROLE
 */
void update_relay_state (void)
{
  byte i;
  boolean relaystate=false;
  for (i=0; i < MAX_RELAYS; i++){


    Relay rele = relay[i];
    switch (rele.role){
    case R_IRRIGATION:
      relaystate=false;
      if(!current_state.field_capacity){

        if (current_state.current_temps==-1000 ||( current_state.current_temps < current_state.temps_max))
        {
          if (rele.state==RELAY_OFF && current_state.current_moisture <= current_state.moisture_MIN )
          {

            rele.state=RELAY_ON;
            relaystate=true;
            //Serial.print("PRUEBA");
            lastEvent=millis();
            interval_mode=I_CONTINOUS;
          }
          else{
            if ((rele.state==RELAY_ON || rele.state==RELAY_WAIT) && (current_state.current_moisture <= current_state.moisture_MAX) )
            {
               if(!checkPumpCicle(relaystate,lastEvent)){
                  if(rele.state==RELAY_ON){
                  relaystate=false;
                  rele.state=RELAY_WAIT;

                  }else{
                    relaystate=true;
                  rele.state=RELAY_ON;
                  }
                  lastEvent=millis();
               }else{
               if(rele.state==RELAY_ON){
                  rele.state=RELAY_ON;
                  relaystate=true; 
               }else{
                 relaystate=false;
                  rele.state=RELAY_WAIT;
                  
               }
               }
            }
          }
        }
        if(current_state.current_moisture >= current_state.moisture_MAX){
          interval_mode=I_INTERVAL;
        }
        

      }
      else{
        rele.state=RELAY_OFF;
        relaystate=false; 
        interval_mode=I_INTERVAL;
      }
      if(relaystate)
      {
        //rele.state=RELAY_ON;
        relay_on(relay[i].gpio_pin);
      }
      else
      {
       // rele.state=RELAY_OFF;
        relay_off(rele.gpio_pin);

      }
      relay[i]=rele;
      break; 
        
    }
  }
}


/*
 * CHECK IRRIGATION CICLE DURATION.
 * IRRIGATION IS CONFIGURABLE TO BE ON CONTINOUS MODE OR AT INTERVALS
 * BY INDICATING PUMP PERCENTAGE.
 * EXAMPLE: PUMP DURING FOUR MINUTES AT 50%. PUMPING WOULD BE ONE MINUTES ON & ONE MINUTES OFF, UNTIL COMPLETION OF THE CICLE OF FOUR MINUTES;
 */
boolean checkPumpCicle(boolean irrigating,long lastEvent){
  double totalSeconds=((current_sensorsvalues.cached_cicle_length * 60))+current_sensorsvalues.cached_pump_cicle_seconds;
  totalSeconds= (totalSeconds * current_sensorsvalues.cached_pump_percent)/100;
  if((millis()-lastEvent)<totalSeconds*1000) return true;
  else return false;
  
} 
/*
 * DRAW TIME SELECTION MENU
 */
void drawTime()
{
  mylcd.setPosition(1,0);
  printTitle(mylcd,translate(S_HOUR));
  mylcd.setPosition(2,0);
  if(editHours<10) mylcd.print("0");
  mylcd.print(editHours);
  mylcd.print(":");
  if(editMinutes<10) mylcd.print("0");
  mylcd.print(editMinutes);
  mylcd.setPosition(3,0);
  if(cTime==S_SAVETIME)
    mylcd.print("*");
  mylcd.print(translate(S_SAVE));
  mylcd.print(F(" "));
  mylcd.setPosition(4,0);
  if(cTime==S_BACKTIME)
    mylcd.print(F("*"));
  mylcd.print(translate(S_RETURN_TO));
  switch(cTime)
  {
  case S_SELECTHOURS:
    mylcd.setPosition(2,1);
    mylcd.boxCursorOn(); 
    break;
  case S_SELECTMINUTES:
    mylcd.setPosition(2,4);
    mylcd.boxCursorOn();
    break;
  default:
    mylcd.boxCursorOff();
    break;
  }
}
/*
 * DRAW DATE SELECTION MENU
 */
void drawDate()
{


  mylcd.setPosition(1,0);
  printTitle(mylcd,translate(S_DATE));
  mylcd.setPosition(2,0);
  if(tm.Day<10) mylcd.print("0");
  mylcd.print(editDays);
  mylcd.print("/");
  if(tm.Month<10) mylcd.print("0");
  mylcd.print(editMonths);
  mylcd.print("/");
  mylcd.print(editYears);
  mylcd.setPosition(3,0);

  if(current_selectionDateState==SAVE)
  {
    mylcd.print("*");
  }
  mylcd.print(translate(S_SAVE));
  mylcd.print(" ");
  mylcd.setPosition(4,0);

  if(current_selectionDateState==BACK)
  {
    mylcd.print("*");
  }
  mylcd.print(translate(S_RETURN_TO));
  mylcd.print(" ");
  switch(current_selectionDateState)
  {
  case DAY:
    mylcd.setPosition(2,1);
    mylcd.boxCursorOn();
    break;
  case MONTH:
    mylcd.setPosition(2,4);
    mylcd.boxCursorOn();
    break;
  case YEAR:
    mylcd.setPosition(2,9);
    mylcd.boxCursorOn();
    break;
  default:
    mylcd.boxCursorOff();
    break;

  }


}
void drawEditingTime(byte currentTimeState)
{
  switch(currentTimeState)
  {

  case S_SELECTHOURS:
    mylcd.setPosition(1,0);
    printTitle(mylcd,translate(S_EDITHOUR));
    mylcd.setPosition(2,0);
    if(editHours<10)
      mylcd.print(F("0"));
    mylcd.print(editHours);
    mylcd.boxCursorOff();
    mylcd.setPosition(2,1);
    mylcd.boxCursorOn();
    break;
  case S_SELECTMINUTES:
    mylcd.setPosition(1,0);
    printTitle(mylcd,translate(S_EDITMINUTES));
    mylcd.setPosition(2,0);
    if(editMinutes<10)
      mylcd.print(F("0"));
    mylcd.print(editMinutes);
    mylcd.boxCursorOff();
    mylcd.setPosition(2,1);
    mylcd.boxCursorOn();
    break;

  }
}
void drawCalibrationSat()
{
  mylcd.setPosition(1,0);
  printTitle(mylcd,translate(S_SATCALIBRATION));
  mylcd.setPosition(2,0);
  mylcd.print(translate(S_CURRENTVALUE));
  mylcd.print(F(": "));
  mylcd.print(readFCapacityValue());
  actualizar_pantalla=true;

}
byte getDaysofMonth(byte month) {  

  switch(month){ 
  case 1:
  case 3:
  case 5:
  case 7:
  case 8:
  case 10:
  case 12:
    return 31;
    break;
  case 2:
    return 2;
  case 4:
  case 6:
  case 9:
  case 11:
    return 30;
  default:
    return -1;
  }
}
void drawEditingDate(byte currentDateState)
{
  byte title;
  int data;
  switch(currentDateState)
  {

  case DAY:
    title=S_EDITDAY;
    data=editDays;
    break; 
  case MONTH:
    title=S_EDITMONTH;
    data=editMonths;
    break;
  case YEAR:
    title=S_EDITYEAR;
    data=editYears;
    break;
  }
  mylcd.boxCursorOff();
  mylcd.setPosition(1,0);
  printTitle(mylcd,translate(title));
  mylcd.setPosition(2,0);
  if(data<10)
    mylcd.print(F("0"));
  mylcd.print(data);
  if(data>1000)
    mylcd.setPosition(2,3);
  else
    mylcd.setPosition(2,1);
  mylcd.boxCursorOn();
}

void static drawAbout()
{
  mylcd.setPosition(1,0);
  printTitle(mylcd,translate(S_ABOUT));
  mylcd.setPosition(2,0);
  mylcd.print(translate(S_SAC));
  mylcd.setPosition(3,0);
  mylcd.print(F("http://sacultivo.com"));
  mylcd.setPosition(4,0);
  mylcd.print(F("VERSION "));
  mylcd.print(VERSION);
} 

void static drawSelectStatus(State & state)
{
  mylcd.setPosition(1,0);
  if(tm.Hour<10) mylcd.print("0");
  mylcd.print(tm.Hour);
  mylcd.print(":");
  if(tm.Minute<10) mylcd.print("0");
  mylcd.print(tm.Minute);
  mylcd.print("  ");
  if(tm.Day<10) mylcd.print("0");
  mylcd.print(tm.Day);
  mylcd.print("/");
  if(tm.Month<10) mylcd.print("0");
  mylcd.print(tm.Month);
  mylcd.print("/");
  mylcd.print(tmYearToCalendar(tm.Year));
  if(state.field_capacity)
  { 
    mylcd.print(" ");
    mylcd.print(translate(S_FC));
  }
  else
  {
    mylcd.print("   "); 
  }
  //Line2
  mylcd.setPosition(2,0);
  mylcd.print(translate(S_S));
  if(state.moisture_MAX<10)
    mylcd.print("0");
  mylcd.print((int)state.moisture_MAX);
  mylcd.print(" ");
  mylcd.print("MIN:");
  if(state.moisture_MIN<10)
    mylcd.print("0");
  mylcd.print((int)state.moisture_MIN);
  mylcd.print(" ");
  mylcd.print("[");
  if(state.current_moisture<10)
    mylcd.print("0");
  mylcd.print((int)state.current_moisture);
  mylcd.print("");
  mylcd.print("]%");
  if(state.moisture_target<=99)
    mylcd.print(" ");



  //line3
  mylcd.setPosition(3,0);
  mylcd.print(translate(CICLO));
  int minutes=state.cicle_length_seconds/60;
  if(minutes<100)
    mylcd.print("0");
  if(minutes<10)
    mylcd.print("0");
  mylcd.print(minutes);
  mylcd.print("'");
  byte seconds= current_sensorsvalues.cached_pump_cicle_seconds;
  if(seconds<10)
    mylcd.print("0");
  mylcd.print(seconds);
  mylcd.print("''");
  mylcd.print("ON");
  if(current_sensorsvalues.cached_pump_percent<100)
    mylcd.print("0");
  if(current_sensorsvalues.cached_pump_percent<10)
    mylcd.print("0");
  mylcd.print(current_sensorsvalues.cached_pump_percent);
  mylcd.print("%");

  //line4
  mylcd.setPosition(4,0);
  mylcd.print(translate(ST_MAX));
  if(state.temps_max<10)
    mylcd.print("0");
  mylcd.print((int)state.temps_max);
  mylcd.print(" ");

  mylcd.print(" ");
  int currenttemp=state.current_temps;
  if(currenttemp>=0){
    mylcd.print("+"); 
  }
  else{
    if(currenttemp==-1000)
      mylcd.print("-"); 
  }
  if(currenttemp!=-1000){
    mylcd.print(currenttemp);
  }
  else{
    mylcd.print("--"); 
  }
  mylcd.print("C");
  mylcd.boxCursorOff();
  mylcd.underlineCursorOff();
  switch(selectionStatus)
  {
  case S_HSO:

    if(!isEditing){
      mylcd.setPosition(2,2);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(2,5);
      mylcd.underlineCursorOn(); 
    }
    break;
  case S_HSMIN:
    if(!isEditing){
      mylcd.setPosition(2,9);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(2,12);
      mylcd.underlineCursorOn(); 
    }
    break;
  case S_TSMAX:
    if(!isEditing){
      mylcd.setPosition(4,4);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(4,7);
      mylcd.underlineCursorOn(); 
    }
    break;

  case S_PCICLE:
    if(!isEditing){
      mylcd.setPosition(3,4);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(3,9);
      mylcd.underlineCursorOn(); 
    }
    break;
  case S_PINTERVAL:
    if(!isEditing){
      mylcd.setPosition(3,15);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(3,19);
      mylcd.underlineCursorOn(); 
    }
    break;
    case S_PCICLESECONDS:
  
    if(!isEditing){
      mylcd.setPosition(3,10);
      mylcd.boxCursorOn(); 
    }
    else{
      mylcd.setPosition(3,11);
      mylcd.underlineCursorOn(); 
    }
  break;
  }
  
}

