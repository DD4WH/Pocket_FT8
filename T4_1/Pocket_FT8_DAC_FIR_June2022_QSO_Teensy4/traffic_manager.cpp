
#include "traffic_manager.h"
#include "display.h"
#include "decode_ft8.h"
#include "gen_ft8.h"
#include "button.h"
#define PTT_Pin 14
//#include "SI4735.h"
#include <Audio.h>

#define FT8_TONE_SPACING        625

extern uint16_t currentFrequency;
extern int xmit_flag, ft8_xmit_counter, Transmit_Armned;

int max_QSO_calls = 5;

extern uint16_t cursor_freq;
extern uint16_t cursor_line;

extern int CQ_Flag;
extern int QSO_Flag;
extern int Target_Flag;
extern int Beacon_State;
extern int num_decoded_msg;

extern int offset_freq;

extern AudioSynthWaveformSine   sine1; 
extern float xmit_level;


uint64_t F_Long, F_FT8, F_Offset;

   void transmit_sequence(void){

      sine1.frequency( (float)cursor_freq);
      sine1.amplitude(xmit_level);
      pinMode(PTT_Pin, OUTPUT); 
      digitalWrite(PTT_Pin, HIGH);
      set_xmit_button(true);  
   }

      void receive_sequence(void){
       sine1.amplitude(0.0);
       pinMode(PTT_Pin, OUTPUT);
       digitalWrite(PTT_Pin, LOW);  
       set_xmit_button(false);
   }

   
   void tune_On_sequence(void){
      sine1.frequency( (float)cursor_freq);
      sine1.amplitude(xmit_level);
      pinMode(PTT_Pin, OUTPUT); 
      digitalWrite(PTT_Pin, HIGH); 
      set_xmit_button(true);
   }

      void tune_Off_sequence(void){ 
       pinMode(PTT_Pin, OUTPUT);
       digitalWrite(PTT_Pin, LOW); 
       sine1.amplitude(0.0);
       set_xmit_button(false);
   }


    void set_FT8_Tone( uint8_t ft8_tone) {
       sine1.frequency( (float)cursor_freq + (float)ft8_tone * 6.25);
    }


    void setup_to_transmit_on_next_DSP_Flag(void){
        ft8_xmit_counter = 0;
        transmit_sequence();
        xmit_flag = 1;

    }


    void service_CQ (void) {
      int receive_index;

      switch (Beacon_State) {

      case 0: Beacon_State = 1;//Listen 
      break;

      case 1: receive_index = Check_Calling_Stations(num_decoded_msg);
      
              if(receive_index >= 0) {
              display_calling_station(receive_index);
              set_message(1); // send Location
              }
              else
              set_message(0); // send CQ
              
              Transmit_Armned = 1;
              Beacon_State = 2;
      break;

      case 2: receive_index = Check_Calling_Stations(num_decoded_msg);
      
              if(receive_index >= 0) {
              display_calling_station(receive_index);
              set_message(2); // send RSL
              Transmit_Armned = 1;
              }
              Beacon_State = 3;
      break;

      
      case 3: receive_index = Check_Calling_Stations(num_decoded_msg);
      
              if(receive_index >= 0) {
              display_calling_station(receive_index);
              set_message(3); // send 73
              Transmit_Armned = 1;
              }
              Beacon_State = 1;
      break;
      }// End of Switch
    } //End of Service CQ

    void service_QSO (void) {
      int receive_index;

      switch (Beacon_State) {

      case 0: receive_index = Check_Calling_Stations(num_decoded_msg);
              if(receive_index >= 0) {
              display_calling_station(receive_index);
              set_message(1); // send Location
              Transmit_Armned = 1;
              Beacon_State = 2;
              }
              else {
              Beacon_State = 1;
              }
      break;

      case 1: receive_index = Check_Calling_Stations(num_decoded_msg);
              if(receive_index >= 0) {
              display_calling_station(receive_index);
              set_message(1); // send Location
              Transmit_Armned = 1;
              Beacon_State = 2;
              }
              else 
              {
              if(Target_Flag > 0 && Target_Flag <= max_QSO_calls) {
              set_message(1); // send Location
              Target_Flag ++;
              Transmit_Armned = 1;
              Beacon_State = 0;
              }

              }

              if (Target_Flag > max_QSO_calls) Target_Flag = 0; //turn off sending location
              //display_value(180,395,Target_Flag);

              
      break;

      case 2: receive_index = Check_Calling_Stations(num_decoded_msg);
              Target_Flag = 0;
              if(receive_index >= 0) {
              display_calling_station(receive_index);
              set_message(2); // send RSL
              Transmit_Armned = 1;
              }
              Beacon_State = 3;
      break;

      
      case 3: receive_index = Check_Calling_Stations(num_decoded_msg);
              Target_Flag = 0;
              if(receive_index >= 0) {
              display_calling_station(receive_index);
              set_message(3); // send 73
              Transmit_Armned = 1;
              }
              Beacon_State = 0;
      break;
      
    } // End of Switch
      } //End of Service QSO
       
