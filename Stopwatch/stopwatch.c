/*----------------------------------------------------------------------------
 * Name:    Blinky.c
 * Purpose: LED Flasher
 * Note(s): possible defines set in "options for target - C/C++ - Define"
 *            __USE_LCD   - enable Output on LCD
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2008-2011 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include "LPC17xx.H"                         /* LPC17xx definitions           */
#include "GLCD.h"
#include "Serial.h"
#include "LED.h"
#include "ADC.h"
#include "mcb1700_joystick.h"
#include <stdbool.h>

#include "cmsis_os.h"
#include "LPC17xx.h"
//#include "Board_LED.h"

#define __FI        1                        /* Font index 16x24               */
                                                                         
char text[10];
static unsigned long ticks = 0;
uint8_t  clock_2s;                           /* Flag activated each second    */

/* Import external variables from IRQ.c file                                  */
extern uint8_t  clock_1s;

uint8_t sec=00;
uint8_t min=00;
uint8_t hour=00;
uint8_t state=0;

osThreadId tid_LCD_UPDATE;                  /* Thread id of task: phase_a */
osThreadId tid_JOYSTICK;                  	/* Thread id of task: phase_b */
osThreadId tid_clock;                  		  /* Thread id of task: clock   */
osThreadId tid_debounce;  

osSemaphoreId LCD;                         	// Semaphore ID
osSemaphoreDef(LCD);                       	// Semaphore definition

osSemaphoreId joystick;                     // Semaphore ID
osSemaphoreDef(joystick);                   // Semaphore definition

osSemaphoreId debouncer;                    // Semaphore ID
osSemaphoreDef(debouncer);                  // Semaphore definition


unsigned char hex2bcd (unsigned char x)
{
    unsigned char y;
    y = (x / 10) << 4;
    y = y | (x % 10);
    return (y);
}

/*----------------------------------------------------------------------------
 *      Thread 1 'phaseA': Phase A output
 *---------------------------------------------------------------------------*/
void LCD_UPDATE (void const *argument) {
  for (;;) {
//		int seconds[2] = "00";
//		uint8_t minutes[2] = "00";
//		uint8_t hours[2] = "00";
		
		osSemaphoreWait (LCD, osWaitForever);
		
		if(state==0){
			
			sec++;
			sprintf(text, "%02X", hex2bcd(sec));       /* format text for print out     */
      GLCD_DisplayString(5,  12, __FI,  (unsigned char *)text);
			
						
		  if (sec==60)
			  {	
			  sec=0;
			  min++;
			 //GLCD_DisplayChar (5, 9, __FI, hex2bcd(min));
			sprintf(text, "%02X", hex2bcd(min));       /* format text for print out     */
      GLCD_DisplayString(5,  9, __FI,  (unsigned char *)text);

			if (min==60)
				{
				min=0;
				hour ++;
				//GLCD_DisplayChar (5, 6, __FI, hex2bcd(min));
			sprintf(text, "%02X", hex2bcd(hour));       /* format text for print out     */
      GLCD_DisplayString(5,  6, __FI,  (unsigned char *)text);
				}
			}
		}
		osSemaphoreRelease (debouncer);
  }
}

/*----------------------------------------------------------------------------
 *      Thread 2 'phaseB': Phase B output
 *---------------------------------------------------------------------------*/
void JOYSTICK (void const *argument) {
	#define JOY_POS_UP     (1<<3)
  #define JOY_POS_DOWN	 (1<<5)
	#define JOY_POS_CENTER (1<<0)
  for (;;) {
			
			osSemaphoreWait (joystick, osWaitForever);

			clock_2s = 0;
			if(JoyPosGet() == JOY_POS_UP)
			{
				 if (state==0)
				 {
							state=1;
							osSemaphoreRelease (debouncer);
						
							GLCD_DisplayString (8, 8, __FI, "Pause"); //sec
				 }
			 
				 else if (state==1)
				 {
							state=0;
						osSemaphoreRelease (debouncer);
					 	GLCD_DisplayString (8, 8, __FI, "         "); //sec
							GLCD_DisplayString (8, 8, __FI, "Run"); //sec
				 }   	
			}
			
			if(JoyPosGet() == JOY_POS_DOWN)
			{	
				sec=00;
				min=00;
				hour=00;
			GLCD_DisplayString (5, 6, __FI, "00"); //hour
			GLCD_DisplayString (5, 8, __FI, ":");
			GLCD_DisplayString (5, 9, __FI, "00"); //min 
			GLCD_DisplayString (5, 11, __FI, ":");
			GLCD_DisplayString (5, 12, __FI, "00"); //sec
				GLCD_DisplayString (8,8 , __FI, "Reset"); //sec
				osSemaphoreRelease (LCD);
				state=1;
				
			}
			osSemaphoreRelease (debouncer);
	}
}

/*----------------------------------------------------------------------------
 *      Thread 5 'clock': Signal Clock
 *---------------------------------------------------------------------------*/
void clock (void  const *argument) {
  for (;;) {
					osDelay(10000); 
			osSemaphoreRelease (LCD);
		  }
}
/*----------------------------------------------------------------------------
 *      Thread 5 'clock': Signal Clock
 *---------------------------------------------------------------------------*/
void debounce (void  const *argument) {
  for (;;) {
			osSemaphoreWait (debouncer, osWaitForever);
			osDelay(1500); 
			osSemaphoreRelease (joystick);
	  }
}

osThreadDef(LCD_UPDATE, osPriorityNormal, 1, 0);
osThreadDef(JOYSTICK, osPriorityNormal, 1, 0);
osThreadDef(clock,  osPriorityNormal, 1, 0);
osThreadDef(debounce,  osPriorityNormal, 1, 0);

/*----------------------------------------------------------------------------
 *      Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/
int main (void) {
	
	#ifdef __USE_LCD
  GLCD_Init();                               /* Initialize graphical LCD      */

  GLCD_Clear(White);                         /* Clear graphical LCD display   */
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(White);
  GLCD_DisplayString(0, 0, __FI, "       MCB1700      ");
	GLCD_DisplayString(1, 0, __FI, "     Stop Watch     ");
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Blue);
	GLCD_DisplayString (5, 6, __FI, "00"); //hour
	GLCD_DisplayString (5, 8, __FI, ":");
	GLCD_DisplayString (5, 9, __FI, "00"); //min 
	GLCD_DisplayString (5, 11, __FI, ":");
	GLCD_DisplayString (5, 12, __FI, "00"); //sec
	GLCD_DisplayString (8, 8, __FI, "Run"); //sec
#endif
                          
	LCD = osSemaphoreCreate(osSemaphore(LCD), 0);
	joystick = osSemaphoreCreate(osSemaphore(joystick), 0);
	debouncer = osSemaphoreCreate(osSemaphore(debouncer), 0);
	
  tid_LCD_UPDATE = osThreadCreate(osThread(LCD_UPDATE), NULL);
  tid_JOYSTICK = osThreadCreate(osThread(JOYSTICK), NULL);
  tid_clock  = osThreadCreate(osThread(clock),  NULL);
	tid_debounce  = osThreadCreate(osThread(debounce),  NULL);

  osDelay(osWaitForever);
  while(1);
}

