/*
 * gen_ft8.h
 *
 *  Created on: Oct 30, 2019
 *      Author: user
 */

#ifndef GEN_FT8_H_
#define GEN_FT8_H_

#include "arm_math.h"


void clear_reply_message_box(void);

void  set_reply(uint16_t index);
void  set_cq(void);
void  set_CQ_reply(void);

void Open_Station_File( void );
void Write_Station_File( void );
void Read_Station_File( void );
void Station_Data_Initialize(void);

void  set_message(uint16_t index);
void clear_FT8_message(void);

#endif /* GEN_FT8_H_ */
