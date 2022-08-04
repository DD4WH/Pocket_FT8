
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include "HX8357_t3.h"
#include "TouchScreen_I2C.h"
#include "MCP342x.h"
#include <TimeLib.h>
#include <TinyGPS.h>

#include "Process_DSP.h"
#include "decode_ft8.h"
#include "WF_Table.h"
#include "arm_math.h"
#include "display.h"
#include "button.h"
#include "locator.h"
#include "traffic_manager.h"

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"
#include "filters.h"

#include "constants.h"

#include "maidenhead.h"

#define AM_FUNCTION 1
#define RESET_PIN 20
#define PTT_Pin 13
#define USB 2


// These are the four touchscreen analog pins
#define YP 38 // must be an analog pin, use "An" notation!
#define XM 37  // must be an analog pin, use "An" notation!
#define YM 36   // can be a digital pin
#define XP 39   // can be a digital pin

HX8357_t3 tft = HX8357_t3(10, 9, 8, 11, 14,12);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 313);

TinyGPS gps; 

AudioSynthWaveformSine   sine1;          //xy=185,364
AudioInputAnalog         adc1;           //xy=191,257
AudioAmplifier           amp1;           //xy=342,255
AudioAmplifier           amp2;           //xy=348,362
AudioRecordQueue         queue1;         //xy=505,250
AudioOutputAnalog        dac1;           //xy=510,365

AudioConnection          patchCord1(sine1, amp2);

AudioConnection          patchCord3(amp1, queue1);
AudioConnection          patchCord4(amp2, dac1);

AudioFilterFIR           Hilbert45_I;
AudioConnection          c1(adc1, 0, Hilbert45_I, 0);
AudioConnection          c3(Hilbert45_I, 0, amp1, 0); 


q15_t dsp_buffer[3*input_gulp_size] __attribute__ ((aligned (4)));
q15_t dsp_output[FFT_SIZE *2] __attribute__ ((aligned (4)));
q15_t  input_gulp[input_gulp_size * 5] __attribute__ ((aligned (4)));

char Station_Call[11]; //six character call sign + /0
char home_Locator[11];; // four character locator  + /0
char Locator[11]; // four character locator  + /0

uint16_t currentFrequency;

uint32_t current_time, start_time, ft8_time;
uint32_t days_fraction, hours_fraction, minute_fraction;

uint8_t  ft8_hours, ft8_minutes, ft8_seconds;
int     ft8_flag, FT_8_counter, ft8_marker, decode_flag;
int WF_counter;
int num_decoded_msg;
int xmit_flag, ft8_xmit_counter, Transmit_Armned;
int DSP_Flag;
int master_decoded;

uint16_t cursor_freq; 
uint16_t cursor_line;
int offset_freq;
int tune_flag;

float xmit_level = 0.8;

int log_flag, logging_on;

float raw_lon;
float raw_lat; 

int auto_location;
float flat, flon;
int32_t gps_latitude;
int32_t gps_longitude;
int8_t  gps_hour;
int8_t  gps_second;
unsigned long gps_age;

int SD_Year;
byte SD_Month, SD_Day, SD_Hour, SD_Minute, SD_Second, SD_Flag;

int altitude;
int8_t  gps_minute;

const int offset = -6;  //CST

  void gps_display_time(int x, int y){

  char string[10];   // print format stuff
  sprintf(string,"%2i:%2i:%2i:%4i",gps_hour, gps_minute, gps_second, gps_second);
  tft.setTextColor(HX8357_WHITE , HX8357_BLACK);
  tft.setTextSize(2);
  tft.setCursor(x, y);
  tft.print(string);
  }


void setup(void) {
  Serial.begin(9600);
  Serial1.begin(9600);

  setSyncProvider(getTeensy3Time);
  delay(100);
  
  tft.begin(HX8357D);
  tft.fillScreen(HX8357_BLACK);
  tft.setTextColor(HX8357_YELLOW);
  tft.setRotation(2);
  tft.setTextSize(2);

  Serial.println(" ");
  Serial.println("To change Transmit Frequency Offset OR Set RTC Touch Tu button, then: ");
  Serial.println("Use keyboard u to raise, d to lower, & s to save  Frequency Offset");
  Serial.println(" ");
  Serial.println("OR");
  Serial.println(" ");
  Serial.println("Use keyboard t to set RTC");
  
  pinMode(PTT_Pin, OUTPUT);
  digitalWrite(PTT_Pin, LOW);
  
  init_DSP();
  initalize_constants();

 AudioMemory(100);
 queue1.begin();

 sine1.amplitude(xmit_level);
 sine1.frequency(cursor_freq);
 //amp2.gain(0.1);
 amp2.gain(1.0);
 amp1.gain(0.1);
 
 Hilbert45_I.begin(RX_hilbert45,HILBERT_COEFFS);

 start_time = millis();

  set_startup_freq();
  receive_sequence();

  update_synchronization();

  open_stationData_file();
  
  for(int i = 0; i<11; i++)  home_Locator[i] = Locator[i] ;
  set_Station_Coordinates(Locator);
  
  display_all_buttons();
  open_log_file();

  tft.setTextColor(HX8357_GREEN , HX8357_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 280);
  tft.print("Pocket-FT8 DAC_FIR_QSO");

  auto_sync_FT8();

  tune_flag = 1; //for allowing serial comms for testing

}



void loop()
{
  
if(decode_flag == 0) process_data();

    if(DSP_Flag == 1) {
     process_FT8_FFT();

    if(xmit_flag == 1 ){
        int offset_index = 0;

        if(ft8_xmit_counter >= offset_index && ft8_xmit_counter < 79 + offset_index ){
        set_FT8_Tone(tones[ft8_xmit_counter - offset_index ]);
        }

        ft8_xmit_counter++;

        if(ft8_xmit_counter == 80  + offset_index){

        xmit_flag = 0;
        receive_sequence();
        terminate_transmit_armed();

        }

      }
      
     DSP_Flag = 0;

     display_time(0,100);
     /*
     if(auto_location ==1) {
     gps_display_time( 0, 100);
     show_decimal(100, 100,raw_lat);
     show_decimal(210, 100,raw_lon);
    }

    else

    {display_date(0,100);
     display_time(140,100);
    }

    */
    }
    

    
    if(decode_flag == 1) {

      num_decoded_msg = ft8_decode();
      master_decoded = num_decoded_msg;
      //display_value(200,100,num_decoded_msg);
      if (num_decoded_msg > 0) display_details(num_decoded_msg);
      decode_flag = 0;
      }

      update_synchronization();

      process_touch();

      delay(10);
      if ( tune_flag == 1) process_serial();

      if ( auto_location == 1 && Serial1.available())   {
      parse_NEMA();
      gps_display_time( 140, 100);
      }
            
    
    }
  
time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

 void process_data() {

  if (queue1.available() >= num_que_blocks) {
    
    for (int i = 0; i<num_que_blocks; i++) {
    copy_to_fft_buffer(input_gulp + block_size *i, queue1.readBuffer());
    queue1.freeBuffer();
    }

    for (int i = 0; i< input_gulp_size; i++) {
      dsp_buffer[i] = dsp_buffer[i + input_gulp_size];
      dsp_buffer[i + input_gulp_size] = dsp_buffer[i + 2*input_gulp_size];
      dsp_buffer[i + 2*input_gulp_size] = input_gulp[i * 5];
    }
    
    DSP_Flag = 1;
    

    }
    
}


static void copy_to_fft_buffer(void *destination, const void *source)
{
  const uint16_t *src = (const uint16_t *)source;
  uint16_t *dst = (uint16_t *)destination;

  for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
    *dst++ = *src++;  // real sample plus a zero for imaginary
  }
}


void rtc_synchronization() {
    getTeensy3Time();


  if(ft8_flag == 0 && second() % 15 == 0)    {

    ft8_flag = 1;
    FT_8_counter = 0;
    ft8_marker = 1;
    WF_counter = 0;
    }
}

void update_synchronization() {
    current_time = millis();
    ft8_time = current_time  - start_time;
    ft8_hours =  (int8_t)(ft8_time/3600000);
    hours_fraction = ft8_time % 3600000;
    ft8_minutes = (int8_t)  (hours_fraction/60000);
    ft8_seconds = (int8_t)((hours_fraction % 60000)/1000);

  if(ft8_flag == 0 && ft8_time % 15000 <= 200)    {
    ft8_flag = 1;
    FT_8_counter = 0;
    ft8_marker = 1;
    WF_counter = 0;
    }
}

void auto_sync_FT8(void) {
      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 240);
      tft.print("Synchronzing With RTC");

       while (second()%15 != 0){
       }

    start_time =millis();
    ft8_flag = 1;
    FT_8_counter = 0;
    ft8_marker = 1;
    WF_counter = 0; 
    tft.setCursor(0, 260);
    tft.print("FT8 Synched With World");
}

void sync_FT8(void) {
    
    //setSyncProvider(getTeensy3Time);
    start_time =millis();
    ft8_flag = 1;
    FT_8_counter = 0;
    ft8_marker = 1;
    WF_counter = 0; 
}

void parse_NEMA(void){
    while (Serial1.available()) {
    if (gps.encode(Serial1.read())) { // process gps messages
      // when TinyGPS reports new data...
      unsigned long age;
      int Year;
      byte Month, Day, Hour, Minute, Second;
      gps.crack_datetime(&Year, &Month, &Day, &Hour, &Minute, &Second, NULL, &age);
      gps.f_get_position(&flat, &flon, &age);
      
      if (age < 500) {
        // set the Time to the latest GPS reading
        setTime(Hour, Minute, Second, Day, Month, Year);
        Teensy3Clock.set(now()); // set the RTC
        //adjustTime(offset * SECS_PER_HOUR);
        gps_hour = Hour;
        gps_minute = Minute;
        gps_second = Second;
        gps_age = age;

        //if(auto_location == 1) {
        char* locator = get_mh(flat, flon,4);
        for(int i = 0; i<11; i++) Locator[i] = locator[i];
        set_Station_Coordinates(Locator);
        //}
      }
    }
  }
}

void set_SD_DateTime(void) {
  setTime(SD_Hour, SD_Minute, SD_Second, SD_Day, SD_Month, SD_Year);
  Teensy3Clock.set(now()); // set the RTC
}
