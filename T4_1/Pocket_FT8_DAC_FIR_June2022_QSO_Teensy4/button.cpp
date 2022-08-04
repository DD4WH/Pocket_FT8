#include "HX8357_t3n.h"
#include "button.h"
#include "display.h"
#include "gen_ft8.h"
#include "traffic_manager.h"
#include "decode_ft8.h"
#include "Process_DSP.h"
#include <EEPROM.h>
#include "TouchScreen_I2C.h"
#include <Wire.h>
#include "locator.h"
#include <TimeLib.h>

// This is calibration data for the raw touch data to the screen coordinates

#define TS_MINX 150
#define TS_MINY 160
#define TS_MAXX 1475
#define TS_MAXY 1400

#define MINPRESSURE 120
#define PENRADIUS 3

extern HX8357_t3n tft;
extern TouchScreen ts;

extern int Transmit_Armned;

//extern SI4735 si4735;
extern uint16_t currentFrequency;
#define USB 2

uint16_t draw_x, draw_y, touch_x, touch_y;
int test;

extern int master_decoded;
extern void sync_FT8(void);
extern uint16_t cursor_freq; 
extern int tune_flag;
extern uint16_t cursor_line;
extern int offset_freq;
#define ft8_shift  6.25
int start_up_offset_freq;
extern int log_flag, logging_on;

extern int auto_location;
extern char home_Locator[];
extern char Locator[];


int CQ_Flag;
int QSO_Flag;
int Beacon_State;
int Target_Flag;

#define numButtons 10
#define button_height 30
#define button_line1 415
#define button_line2 450
#define button_width 60

long positionRight  = 500;
uint32_t last_touch;
TSPoint pi, pw;//

typedef struct
{
  const char* text;

        bool       state;
        const uint16_t x;
        const uint16_t y;
        const uint16_t w;
        const uint16_t h;
        bool  active_state;       
} ButtonStruct;




static ButtonStruct sButtonData[] = {
  {
    /*text*/ " CQ ",  //button 0
    /*state*/ false,
    /*x*/ 0,
    /*y*/ button_line1,
    /*w*/ button_width,
    /*h*/ button_height
  },

  {
    /*text*/ "CALL",  //button 1
    /*state*/ false,
    /*x*/ 86,
    /*y*/ button_line1,
    /*w*/ button_width,
    /*h*/ button_height
  },

  {
    /*text*/ "RS",  //button 2
    /*state*/ false,
    /*x*/ 136,
    /*y*/ button_line1,
    /*w*/ button_width,
    /*h*/ button_height
  },

  {
    /*text*/ "73", //button 3
    /*state*/ false,
    /*x*/ 204,
    /*y*/ button_line1,
    /*w*/ button_width,
    /*h*/ button_height
  },

  {
    /*text*/ "CLR ",  //button 4
    /*state*/ false,
    /*x*/ 172,  //was 272
    /*y*/ button_line1,
    /*w*/ button_width,
    /*h*/ button_height
  },

  {
    /*text*/ "TUNE",  //button 5
    /*state*/ false,
    /*x*/ 0,
    /*y*/ button_line2,
    /*w*/ button_width,
    /*h*/ button_height
  },

  {
    /*text*/ " TX ",  //button 6
    /*state*/ false,
    /*x*/ 86,
    /*y*/ button_line2,
    /*w*/ button_width,
    /*h*/ button_height
  },

    {
    /*text*/ "LOG ", //button 7
    /*state*/ false,
    /*x*/ 172,
    /*y*/ button_line2,
    /*w*/ button_width,
    /*h*/ button_height
  },

    {
    /*text*/ "SYNC", //button 8
    /*state*/ false,
    /*x*/ 258,
    /*y*/ button_line2,  //was line1
    /*w*/ button_width,
    /*h*/ button_height
    },

        {
    /*text*/ "GPS ", //button 9
    /*state*/ false,
    /*x*/ 258, //was 272
    /*y*/ button_line1,  //was line 2
    /*w*/ button_width,
    /*h*/ button_height
    }
    

        };  // end of button definition


    void drawButton(uint16_t i) {
    uint16_t color;
    if (sButtonData[i].state)
      color = HX8357_RED;
      else
      color = HX8357_GREEN;
    
    tft.drawRect(sButtonData[i].x, sButtonData[i].y, sButtonData[i].w, sButtonData[i].h,color);
    tft.setCursor(sButtonData[i].x+7, sButtonData[i].y+7);
    tft.setTextColor(HX8357_WHITE);
    tft.print(sButtonData[i].text);
    }


    
void display_all_buttons(void) {
    drawButton(0);
    drawButton(1);
    //drawButton(2);
    //drawButton(3);
    drawButton(4); 
    drawButton(5);
    drawButton(6);
    drawButton(7);
    drawButton(8);
    drawButton(9);

    for (int i = 0; i<numButtons;i++) sButtonData[i].active_state = true;
    sButtonData[2].active_state = false;
    sButtonData[3].active_state = false;
    sButtonData[6].active_state = false;

    
}


void checkButton(void)  {

      for (uint8_t i=0;i<numButtons;i++){
        if (testButton( i) == 1 && sButtonData[i].active_state )  {
        sButtonData[i].state =! sButtonData[i].state;
        drawButton(i);
        executeButton (i);
        }
  }
  }

int button_delay = 100;

void executeButton (uint16_t index){
//   int Idx =0;
   switch (index) {
   
   case 0:   
              if (sButtonData[0].state)
              {set_QSO_button(false);
              CQ_Flag = 1;
              Beacon_State = 1;
              }
               else
             {CQ_Flag = 0;
             delay(button_delay );
             }
              break; 
   
   case 1:  
              if (sButtonData[1].state)
              {set_CQ_button(false);            
              QSO_Flag = 1;
              Beacon_State = 1;
              }
               else
             {QSO_Flag = 0;
             delay(button_delay );
             }
            break;  
   /*    
   case 2:  set_message(2);
            sButtonData[2].state = true;
            drawButton(2);
            delay(button_delay );
            sButtonData[2].state = false;
            drawButton(2);
            break;  
           
  case 3:   set_message(3);
            sButtonData[3].state = true;
            drawButton(3);
            delay(button_delay );
            sButtonData[3].state = false;
            drawButton(3);
            break;  
  */
    case 4: clear_FT8_message();
            sButtonData[4].state = true;
            drawButton(4);
            delay(button_delay );
            sButtonData[4].state = false;
            drawButton(4);
            break;
            
   case 5:  if (sButtonData[5].state)
           { tune_On_sequence();
            tune_flag = 1;
            //delay(button_delay);
           }
            else
            {tune_Off_sequence();
            tune_flag = 0;
            //delay(button_delay );
            }
            break;     

   case 6:  /*if (sButtonData[6].state)
            
              {Transmit_Armned = 1;
              delay(button_delay );
              }
               else
              {Transmit_Armned = 0;
              delay(button_delay );
              }
              */
              break;  
              
      case 7: if (log_flag == 1 && sButtonData[7].state ){
      
             logging_on = 1;
             sButtonData[7].state = true;
             drawButton(7);
             delay(button_delay );

             }
            else {
            logging_on = 0;
            sButtonData[7].state = false;
            drawButton(7);
            delay(button_delay );
            }
            break;   

       case 8:  sButtonData[8].state = true;
            drawButton(8);
            delay(button_delay );
            sync_FT8();
            sButtonData[8].state = false;
            drawButton(8);
            delay(button_delay );
            break;                    
            

       case 9: if  (sButtonData[9].state ){
           auto_location = 1;
           sButtonData[9].state = true;
           drawButton(9);
           delay(button_delay );
           tft.fillRect(0, 100, 320, 20, HX8357_BLACK);
           }
          else {
          auto_location = 0;
          /*
          for(int i = 0; i<11; i++) Locator[i] = home_Locator[i];
          set_Station_Coordinates(Locator);
          */
          sButtonData[9].state = false;
          drawButton(9);
          delay(button_delay );
          tft.fillRect(0, 100, 320, 20, HX8357_BLACK);
          }
          break;

          }
}


        void set_xmit_button(bool state){
        sButtonData[6].state = state;
        drawButton(6);
        delay(button_delay );
        }

        void set_CQ_button(bool state){
        sButtonData[0].state = state;
        CQ_Flag = 0;
        drawButton(0);
        delay(button_delay );
        }    

        
        void set_QSO_button(bool state){
        sButtonData[1].state = state;
        QSO_Flag = 0;
        drawButton(1);
        delay(button_delay );
        } 



void terminate_transmit_armed(void) {

            Transmit_Armned = 0;
            receive_sequence();
            sButtonData[6].state = false;
            drawButton(6);
            }

   int  testButton(uint8_t index) {

    if  ((draw_x  > sButtonData[index].x ) && ( draw_x < sButtonData[index].x+sButtonData[index].w)  && (draw_y  > sButtonData[index].y ) && ( draw_y < sButtonData[index].y+sButtonData[index].h) )   return  1;

    else
    
   return 0;
   
  }

  void check_FT8_Touch(void) {

   int FT_8_TouchIndex;
   int y_test;


  if  (draw_x < 320  && (draw_y > 120 && draw_y  <365)){
    y_test = draw_y - 120;
    
    FT_8_TouchIndex = y_test / 25;
    if(FT_8_TouchIndex < master_decoded) display_selected_call(FT_8_TouchIndex);
    }

    }

  void check_WF_Touch(void){
  if  (draw_x < 320  && draw_y < 90) {

  cursor_line = draw_x;
  cursor_freq = (uint16_t) ( (float) (cursor_line + ft8_min_bin) * ft8_shift);
  //set_Xmit_Freq();
    
  }
  }


      void set_startup_freq(void){
      cursor_line = 100;
     // if (EEPROMReadInt(10) <= 0) 
   //   start_up_offset_freq = EEPROMReadInt(10);
   //   else
    //  start_up_offset_freq = -83;
      
      cursor_freq = (uint16_t) ( (float) (cursor_line + ft8_min_bin) * ft8_shift);
   //   offset_freq = start_up_offset_freq;
    }


   
   void process_touch(void){

    pi = ts.getPoint();
    
    if (pi.z > MINPRESSURE ) {
   // Serial.print("Value0 = "); Serial.println(pi.z);
   // Serial.print("Value1 = "); Serial.println(pi.y);
   // Serial.print("Value2 = "); Serial.println(pi.x);
    pw.y = map(pi.x, TS_MINX , TS_MAXX, 0, 480);
    pw.x = map(pi.y, TS_MINY, TS_MAXY , 320, 0);

   
    Serial.print("Value X = "); Serial.println(pw.x);
    Serial.print("Value Y = "); Serial.println(pw.y);
    Serial.println(" ");

    //tft.fillCircle(pw.x, pw.y, PENRADIUS, HX8357_YELLOW);

    draw_x = pw.x;
    draw_y = pw.y ;

    checkButton();
    check_FT8_Touch();
    check_WF_Touch();
   }

   }


 
   void process_serial(void) {

      int incoming_byte;
      if (Serial.available() > 0){
      incoming_byte = Serial.read();
      //display_value(240,200,incoming_byte);
      
      if(incoming_byte == 117) offset_freq = offset_freq + 10;
      if(incoming_byte == 100) offset_freq = offset_freq - 10 ;
      //display_value(240, 220, ( int ) offset_freq);
      Serial.println(offset_freq);
      //set_Xmit_Freq();
      
      if(incoming_byte == 115) {  // s for store
        store_encoders() ;
        Serial.println("offset_freq stored");
      }
      
      if(incoming_byte == 116) { // t for time update
      open_date_time_file();
      Serial.println("date time set");
      }
      
      }
   }
   


  void store_encoders(void){

    EEPROMWriteInt(10, offset_freq);
    delay(button_delay );
    test = EEPROMReadInt(10);
  }



void EEPROMWriteInt(int address, int value)
{
  uint16_t   internal_value = 32768 + value;
  
  byte byte1 = internal_value >> 8;
  byte byte2 = internal_value & 0xFF;
  EEPROM.write(address, byte1);
  EEPROM.write(address + 1, byte2);
}

int EEPROMReadInt(int address)
{
  uint16_t byte1 = EEPROM.read(address);
  uint16_t byte2 = EEPROM.read(address + 1);
  uint16_t   internal_value = (byte1 << 8 |  byte2);
  
  return (int)internal_value  - 32768;
}
