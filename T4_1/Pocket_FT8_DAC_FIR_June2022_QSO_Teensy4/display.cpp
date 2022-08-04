
#include "HX8357_t3n.h"
#include "display.h"
#include <TimeLib.h>

#include <SD.h>
#include <SPI.h>

char log_filename[] = "FT8_Traffic.txt";
File Log_File;
char stationData_filename[] = "Station_Data.txt";
File stationData_File;

File Date_Time_File;


extern time_t getTeensy3Time();
extern   char Station_Call[]; //six character call sign + /0
extern   char Locator[]; // four character locator  + /0

extern int log_flag, logging_on;


extern HX8357_t3n tft;


      void display_value(int x, int y, int value){
      char string[7];   // print format stuff
      sprintf(string,"%6i",value);
      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(x, y);
      tft.print(string);
      }

     
      
      
      void display_time(int x, int y){
      getTeensy3Time();
      char string[10];   // print format stuff
      sprintf(string,"%2i:%2i:%2i",hour(),minute(),second());
      tft.setTextColor(HX8357_WHITE , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(x, y);
      tft.print(string);
      }

      void display_date(int x, int y){
      getTeensy3Time();
      char string[10];   // print format stuff
      sprintf(string,"%2i %2i %4i",day(),month(),year());
      tft.setTextColor(HX8357_WHITE , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(x, y);
      tft.print(string);
      }


      
      void make_filename(void) {
      getTeensy3Time();
      sprintf((char *)log_filename,"%2i%2i%4i%2i%2i",day(),month(),year(),hour(),minute());
      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 200);
      tft.print(log_filename);
      }

    
    bool open_log_file(void) {

    if (!SD.begin(BUILTIN_SDCARD)) {
      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 200);
      tft.print("SD Card not found");
      log_flag = 0;
      return false;
       }
      else
      {
      Log_File = SD.open("FT8_Log.txt", FILE_WRITE);
      //Log_File = SD.open(log_filename, FILE_WRITE);
      
      Log_File.println(" ");
      Log_File.println("**********************");
      
      Log_File.close();
      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 200);
      tft.print("FT8_Log.txt");
      log_flag = 1;
      return true;
      }
      }


    bool open_stationData_file(void) {
    char read_buffer[64];
    char* Station_Data;
      
    if (!SD.begin(BUILTIN_SDCARD)) {
      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 200);
      tft.print("SD Card not found");
      return false;
       }
      else
      {
      stationData_File = SD.open("Station_Data.txt", FILE_READ);
      if(stationData_File)stationData_File.read(read_buffer,64);
      stationData_File.close();
      }

      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 220);
      tft.print(read_buffer);


      Station_Data = strtok(read_buffer,":");
      strcpy(Station_Call, Station_Data);
      tft.setCursor(0, 240);
      tft.print(Station_Call);
      
      Station_Data = strtok(NULL,":");
      strcpy(Locator, Station_Data);
      tft.setCursor(0, 260);
      tft.print(Locator);

      return true;
      }
        
      
    bool open_date_time_file(void) {
    char read_buffer[64];
    char* Date_Time_Data;
    int hour,minute,second,day,month,year;
    char detail_data[4];
    
      
    if (!SD.begin(BUILTIN_SDCARD)) {
      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 200);
      tft.print("SD Card not found");
      return false;
       }
      else
      {
      Date_Time_File = SD.open("Date_Time.txt", FILE_READ);
      if(Date_Time_File)Date_Time_File.read(read_buffer,64);
      Date_Time_File.close();
      }

      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 260);
      tft.print(read_buffer);


      Date_Time_Data = strtok(read_buffer,":");
      strcpy(detail_data, Date_Time_Data);
      hour = atoi(detail_data);

      Date_Time_Data = strtok(NULL,":");
      strcpy(detail_data, Date_Time_Data);
      minute = atoi(detail_data);

      Date_Time_Data = strtok(NULL,":");
      strcpy(detail_data, Date_Time_Data);
      second = atoi(detail_data);

      Date_Time_Data = strtok(NULL,":");
      strcpy(detail_data, Date_Time_Data);
      day = atoi(detail_data);

      Date_Time_Data = strtok(NULL,":");
      strcpy(detail_data, Date_Time_Data);
      month = atoi(detail_data);

      Date_Time_Data = strtok(NULL,":");
      strcpy(detail_data, Date_Time_Data);
      year = atoi(detail_data);

      setTime(hour, minute, second, day, month, year);
      Teensy3Clock.set(now()); // set the RTC

      
      
      return true;
      }



      

      void display_station_data(int x, int y){
      char string[13];   // print format stuff
      sprintf(string,"%7s %4s",Station_Call,Locator);
      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(x, y);
      tft.print(string);

      }

      
      void write_log_data(char *data){
      Log_File = SD.open("FT8_Log.txt", FILE_WRITE);
      Log_File.println(data);
      Log_File.close();
      }


      void show_degrees(uint16_t x, uint16_t y,int32_t variable) {
      char test_string[20];
      float scaled_variable, remainder;
      int units, fraction;
      scaled_variable = (float)variable/10000000;
      units = (int)scaled_variable;
      remainder = scaled_variable - units;
      fraction = (int)(remainder * 1000);
      if(fraction<0)fraction = fraction * -1;
      sprintf(test_string,"%3i.%3i",units, fraction);
      tft.setTextColor(HX8357_YELLOW , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(x, y);
      tft.print(test_string);
}
      
      
      void show_decimal(uint16_t x, uint16_t y,float variable) {
      char str[20];
      int units, fraction;
      float remainder;
      units = (int)variable;
      remainder = variable - units;
      fraction = (int)(remainder * 1000);
      if(fraction<0)fraction = fraction * -1;
      sprintf(str,"%3i.%3i",units, fraction);
      tft.setTextColor(HX8357_WHITE , HX8357_BLACK);
      tft.setTextSize(2);
      tft.setCursor(x, y);
      tft.print(str);
    }


    
