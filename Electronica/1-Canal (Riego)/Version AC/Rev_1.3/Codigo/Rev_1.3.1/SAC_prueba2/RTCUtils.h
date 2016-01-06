#include <DS1307RTC.h>
#include <Wire.h>
#include <Time.h>
#include <stdlib.h>

double getMinutesBetween(tmElements_t time1,tmElements_t time2);

/*
 * RTCUtils.h
 *
 *  Created on: 16/03/2014
 *      Author: dcuevas
 */
/*
 * RTCUtils.h: This file contains all the functions for reading the Real Time Clock
 *
 *  Created on: 16/03/2014
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
 * 0.2. Improved Version.
 * 
 * Current Version: 0.2
 */
 

 
int RTCread(tmElements_t & tm);
char * getTimeAndHour(tmElements_t tm);
String parseDigit(int n);
tmElements_t parseTimeAndHour(char * td);


/*
* Reads the Time and Date form the DS1307 RTC.
* Parameters:
* tm: the tmElements with the Time and Date information
* returns true if success or false otherwise.
*/
int RTCread(tmElements_t & tm){
	
	if(!RTC.read(tm)){
	  if(RTC.chipPresent()){
              return 0;
            }else{
              return -1;
            }	
	}
}
/*
* get an String from and tmElements_t with the current format:
*     HH:MM    DD/MM/YYYY
* Parameters:
* tm: Time and Date Information
* Returns: the String with time and Date with Format: HH:MM    DD/MM/YYYY.
*/
char * getTimeAndHour(tmElements_t tm)
{
  char timedate[30]="%d:%d   %d/%d/%d";
  
  
  char buffer[30];
  sprintf(buffer,timedate,tm.Hour,tm.Minute,tm.Day,tm.Month,tmYearToCalendar(tm.Year));
  return buffer;
}
/*
* Parse a digit for Show as time or date.
* (if the number is between 0 and 9 a '0' is add at the begin).
* parameters:
* n: number to parse.
* returns: the String as a digit.
*/
String parseDigit(int n)
{
   String str="";
   if(n >=0 &&  n<10){
     str+="0";
   }
   str+=n;
  
   
   return str;
}



tmElements_t parseTimeAndHour(char * td)
{
  char * buffer=new char[strlen(td)];
  String Hour;
  int i=0;
  tmElements_t tm;
  for(i=0;i<strlen(td);i++)
  {
    if(buffer[i]!=':')
      Hour+=buffer[i];
    else
      break;  
    
  }
   char  buff[2];
   Hour.toCharArray(buff,2);
   tm.Hour=atoi(buff);  
  for(;i<strlen(td);i++)
  {
    
  }
  return tm;
}
long time_between(tmElements_t time1, tmElements_t time2)
{
  long nminutes1=  numberOfMinutes(makeTime(time1));
  long nminutes2=  numberOfMinutes(makeTime(time2));
  return (nminutes2-nminutes1);
}

void setHour(tmElements_t currentTime)
{
   RTC.write(currentTime); 
}
