/*
 * decode_ft8.c
 *
 *  Created on: Sep 16, 2019
 *      Author: user
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include "gen_ft8.h"

#include "unpack.h"
#include "ldpc.h"
#include "decode.h"
#include "constants.h"
#include "encode.h"
//#include "button.h"
#include <TimeLib.h>

#include "Process_DSP.h"
#include "display.h"
#include "decode_ft8.h"

#include "HX8357_t3n.h"
extern HX8357_t3n tft;


char blank[] = "                          ";

const int kLDPC_iterations = 10;
const int kMax_candidates = 20;
const int kMax_decoded_messages = 10;  //chhh 27 feb
//const int kMax_decoded_messages = 4;  //chhh 27 feb
const int kMax_message_length = 20;


const int kMin_score = 40;		// Minimum sync score threshold for candidates

int validate_locator(char locator[]);
int strindex(char s[],char t[]);

extern uint32_t ft8_time;
extern uint8_t export_fft_power[ft8_msg_samples*ft8_buffer*4] ;

extern int ND;
extern int NS;

extern int NN ;
// Define the LDPC sizes
extern int N;
extern int K;

extern int M;

extern int K_BYTES;

extern void write_log_data(char *data);

Decode new_decoded[20];

Calling_Station Answer_CQ[100];
CQ_Station Calling_CQ[8];

int 	num_calls;  // number of unique calling stations
int num_call_checks;
int num_CQ_calls;
int num_calls_to_CQ_station;
int max_displayed_CQ = 6;
int message_limit = 10;

int max_Calling_Stations = 6;
int num_Calling_Stations;

extern char Station_Call[];


extern float Station_Latitude, Station_Longitude;

extern float Target_Latitude, Target_Longitude;

extern float Target_Distance(char target[]);

extern int CQ_State;
extern int Beacon_State;
extern int Target_Flag;

extern char Target_Call[7];
extern int Target_RSL; // four character RSL  + /0

extern time_t getTeensy3Time();
extern int log_flag, logging_on;

int ft8_decode(void) {

    // Find top candidates by Costas sync score and localize them in time and frequency
    Candidate candidate_list[kMax_candidates];

    int num_candidates = find_sync(export_fft_power, ft8_msg_samples, ft8_buffer, kCostas_map, kMax_candidates, candidate_list, kMin_score);
    char    decoded[kMax_decoded_messages][kMax_message_length];

    const float fsk_dev = 6.25f;    // tone deviation in Hz and symbol rate

    // Go over candidates and attempt to decode messages
    int num_decoded = 0;

    for (int idx = 0; idx < num_candidates; ++idx) {
        Candidate cand = candidate_list[idx];
        float freq_hz  = (cand.freq_offset + cand.freq_sub / 2.0f) * fsk_dev;

        float   log174[N];
        extract_likelihood(export_fft_power, ft8_buffer, cand, kGray_map, log174);

        // bp_decode() produces better decodes, uses way less memory
        uint8_t plain[N];
        int     n_errors = 0;
        bp_decode(log174, kLDPC_iterations, plain, &n_errors);
        
        if (n_errors > 0)    continue;

        // Extract payload + CRC (first K bits)
        uint8_t a91[K_BYTES];
        pack_bits(plain, K, a91);
        
        // Extract CRC and check it
        uint16_t chksum = ((a91[9] & 0x07) << 11) | (a91[10] << 3) | (a91[11] >> 5);
        a91[9] &= 0xF8;
        a91[10] = 0;
        a91[11] = 0;
        uint16_t chksum2 = crc(a91, 96 - 14);
        if (chksum != chksum2)   continue;
        
        char message[kMax_message_length];

        char field1[14];
        char field2[14];
        char field3[7];
        int rc = unpack77_fields(a91, field1, field2, field3);
        if (rc < 0) continue;
        
        sprintf(message,"%s %s %s ",field1, field2, field3);
        
        // Check for duplicate messages (TODO: use hashing)
        bool found = false;
        for (int i = 0; i < num_decoded; ++i) {
            if (0 == strcmp(decoded[i], message)) {
                found = true;
                break;
            }
        }

		int raw_RSL;
		int display_RSL;
		float distance;

      getTeensy3Time();
      char rtc_string[10];   // print format stuff
      sprintf(rtc_string,"%2i:%2i:%2i",hour(),minute(),second());

        if (!found && num_decoded < kMax_decoded_messages) {
        	if(strlen(message) < kMax_message_length) {
            strcpy(decoded[num_decoded], message);

            new_decoded[num_decoded].sync_score = cand.score;
            new_decoded[num_decoded].freq_hz = (int)freq_hz;
            strcpy(new_decoded[num_decoded].field1, field1);
            strcpy(new_decoded[num_decoded].field2, field2);
            strcpy(new_decoded[num_decoded].field3, field3);
            strcpy(new_decoded[num_decoded].decode_time, rtc_string);
            
			raw_RSL = new_decoded[num_decoded].sync_score;
			if (raw_RSL > 160) raw_RSL = 160;
			display_RSL = (raw_RSL - 160 ) / 6;
			new_decoded[num_decoded].snr = display_RSL;

			char Target_Locator[] = "    ";

			strcpy(Target_Locator, new_decoded[num_decoded].field3);

			if (validate_locator(Target_Locator)  == 1) {
				distance = Target_Distance(Target_Locator);
				new_decoded[num_decoded].distance = (int)distance;
			}
			else
			new_decoded[num_decoded].distance = 0;
      
            ++num_decoded;
            
        	}

        }
    }  //End of big decode loop


    return num_decoded;

}

    
void display_messages(int decoded_messages){

	    char message[kMax_message_length];
      char big_gulp[60];

      //tft.fillRect(0, 100, 240, 140, HX8357_BLACK);
      tft.fillRect(0, 120, 320, 195, HX8357_BLACK);
  
  		for (int i = 0; i<decoded_messages && i<message_limit; i++ ){
  		sprintf(message,"%s %s %s",new_decoded[i].field1, new_decoded[i].field2, new_decoded[i].field3);

      sprintf(big_gulp,"%s %s", new_decoded[i].decode_time, message);
      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      //tft.setCursor(0, 100 + i *25 );
      tft.setCursor(0, 120 + i *25 );
      tft.print(message);

      //if (logging_on == 1) write_log_data(big_gulp);
      
		}
    
}


void display_calling_station(int index){
  
      char selected_station[18];
      strcpy(Target_Call, new_decoded[index].field2);
      Target_RSL = new_decoded[index].snr;

      sprintf(selected_station,"%7s %3i",Target_Call, Target_RSL);
      tft.setTextColor(HX8357_WHITE , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 395 );
      tft.print(blank);
      tft.setCursor(0, 395 );
      tft.print(selected_station);
}

void display_selected_call(int index){
  
      char selected_station[18];
      strcpy(Target_Call, new_decoded[index].field2);
      Target_RSL = new_decoded[index].snr;

      sprintf(selected_station,"%7s %3i",Target_Call, Target_RSL);
      tft.setTextColor(HX8357_WHITE , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 395 );
      tft.print(blank);
      tft.setCursor(0, 395 );
      tft.print(selected_station);
      Target_Flag = 1;
}


void display_details(int decoded_messages){

	  char message[48];
    
  // tft.fillRect(0, 100, 500, 320, RA8875_BLACK);
    tft.fillRect(0, 120, 320, 245, HX8357_BLACK);
    
		for (int i = 0; i<decoded_messages && i<message_limit; i++ ){
		//sprintf(message,"%7s %7s %4s %4i %3i %4i",new_decoded[i].field1, new_decoded[i].field2, new_decoded[i].field3,new_decoded[i].freq_hz, new_decoded[i].snr, new_decoded[i].distance );
    sprintf(message,"%7s %7s %4s %4i",new_decoded[i].field1, new_decoded[i].field2, new_decoded[i].field3, new_decoded[i].distance );
    tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
    tft.setTextSize(2);
    //tft.setTextSize(1);
    //tft.setCursor(0, 100 + i *25 );
    tft.setCursor(0, 120 + i *25 );
    tft.print(message);
		}

}

int validate_locator(char locator[]) {

  uint8_t A1, A2, N1, N2;
  uint8_t test = 0;

  A1 = locator[0] - 65;
  A2 = locator[1] - 65;
  N1 = locator[2] - 48;
  N2= locator [3] - 48;

  if (A1 >= 0 && A1 <= 17) test++;
  if (A2 > 0 && A2 < 17) test++; //block RR73 Artic and Anartica
  if (N1 >= 0 && N1 <= 9) test++;
  if (N2 >= 0 && N2 <= 9) test++;

  if (test == 4) return 1;
  else
  return 0;
}




int strindex(char s[],char t[])
{
    int i,j,k, result;

    result = -1;

    for(i=0;s[i]!='\0';i++)
    {
        for(j=i,k=0;t[k]!='\0' && s[j]==t[k];j++,k++)
            ;
        if(k>0 && t[k] == '\0')
            result = i;
    }
    return result;
}



int Check_Calling_Stations(int num_decoded ) {
      char big_gulp[60];
      char message[kMax_message_length + 10];  // add space for time stamp
      int  message_test = 0;
      
      for (int i = 0; i<num_decoded; i++ ){
      if(strindex(new_decoded[i].field1, Station_Call)  >= 0 )  {
      sprintf(message,"%s %s %s %s",new_decoded[i].decode_time, new_decoded[i].field1, new_decoded[i].field2, new_decoded[i].field3);

      getTeensy3Time();
      //sprintf(big_gulp,"%2i/%2i/%4i %s %s", day(),month(),year(), new_decoded[i].decode_time, message);
      //sprintf(big_gulp,"%2i/%2i/%4i %s %s %4i", day(),month(),year(), new_decoded[i].decode_time, message,new_decoded[i].distance );
      sprintf(big_gulp,"%2i/%2i/%4i %s %4i", day(),month(),year(), message, new_decoded[i].distance );
      tft.setTextColor(HX8357_GREEN , HX8357_BLACK);
      tft.setTextSize(2);
      //tft.setCursor(160, 150 + i *25 );
      tft.setCursor(0, 375 );
      tft.print(blank);
      tft.setCursor(0, 375 );
      tft.print(message);

      if (logging_on == 1) write_log_data(big_gulp);
      
      //num_Calling_Stations ++;
      message_test = i + 100;
      }

      }

      if (message_test > 100) return message_test - 100;
      else
      return -1;

}
