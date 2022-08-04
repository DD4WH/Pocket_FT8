/*
 * gen_ft8.c
 *
 *  Created on: Oct 24, 2019
 *      Author: user
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <TimeLib.h>
#include "pack.h"
#include "encode.h"
#include "constants.h"
#include "gen_ft8.h"
#include <stdio.h>
#include "HX8357_t3n.h"
extern HX8357_t3n tft;

#include "arm_math.h"
#include <string.h>
#include "decode_ft8.h"
#include "display.h"
#include "locator.h"
#include "traffic_manager.h"

char Target_Call[7]; //six character call sign + /0
char Target_Locator[5]; // four character locator  + /0
int Target_RSL; // four character RSL  + /0
char CQ_Target_Call[7];

char message[18];
extern char blank[];

extern int log_flag, logging_on;
extern time_t getTeensy3Time();
extern char Station_Call[];
extern char Locator[];
extern int Target_Flag;

char ft8_time_string[] = "15:44:15";


void  set_message(uint16_t index) {

	char big_gulp[60];
    uint8_t packed[K_BYTES];
    char seventy_three[] = "RR73";
    char message_blank[] = "                  ";

      getTeensy3Time();
      char rtc_string[30];   // print format stuff
      sprintf(rtc_string,"%2i/%2i/%4i %2i:%2i:%2i",day(),month(),year(),hour(),minute(),second());

      char message_time_string[10];   // print format stuff
      sprintf(message_time_string,"%2i:%2i:%2i", hour(),minute(),second());

    strcpy(message, message_blank);

    switch ( index ){

  case 0:   sprintf(message,"%s %s %s", "CQ",Station_Call,Locator);

  break;

	case 1:   sprintf(message,"%s %s %s", Target_Call,Station_Call,Locator);

	break;

	case 2:  sprintf(message,"%s %s %3i", Target_Call,Station_Call,Target_RSL);
 
	break;

	case 3:   sprintf(message,"%s %s %3s", Target_Call,Station_Call,seventy_three);

	break;

    }

      sprintf(message_time_string,"%s %s", message_time_string,message);
      tft.setTextColor(HX8357_WHITE , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 395);
      tft.print(blank);
      tft.setCursor(0, 395);
      tft.print(message_time_string);
    
    pack77_1(message, packed);
    genft8(packed, tones);

    if (index != 0) {

    sprintf(big_gulp,"%s %s", rtc_string, message);
    if (logging_on == 1) write_log_data(big_gulp);
    
    }

}


void clear_FT8_message(void) {
    
    tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 395);
    tft.print(blank);
    Target_Flag = 0;
}
