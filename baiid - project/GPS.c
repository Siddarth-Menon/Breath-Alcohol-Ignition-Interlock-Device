#include <lpc214x.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


void LcdInit (void);
void DisplayRow (int row, char *str);
void Send_Command(unsigned char cmd);
void Send_Data(unsigned char dat);
void LcdCmd(unsigned char cmd);
void LcdDat(unsigned char dat);

void SmallDelay(void);
void Delay250(void);
void DelayMs(int n);
void clear_lcd(void)
{
       LcdCmd(0x01);
       LcdCmd(0x80);   //clear screen n start from fist line first column
       LcdCmd(0x06);     //incremental cursor
}

char Latitude_Buffer[15],Longitude_Buffer[15],Time_Buffer[15],Altitude_Buffer[8];
char iir_val[10];
char GGA_String[150];
uint8_t GGA_Comma_Pointers[20];
char GGA[3];
volatile uint16_t GGA_Index, CommaCounter;
bool	IsItGGAString	= false;



__irq void UART0_Interrupt(void);




void delay_ms(uint16_t j)
{
    uint16_t x,i;
	for(i=0;i<j;i++)
	{
    for(x=0; x<6000; x++);    
	}
}




void UART0_init(void)
{
	PINSEL0 = PINSEL0 | 0x00000005;	/* Enable UART0 Rx0 and Tx0 pins of UART0 */
	U0LCR = 0x83;	/* DLAB = 1, 1 stop bit, 8-bit character length */
	U0DLM = 0x00;	/* For baud rate of 9600 with Pclk = 15MHz */
	U0DLL = 0x61;	/* We get these values of U0DLL and U0DLM from formula */
	U0LCR = 0x03; /* DLAB = 0 */
	U0IER = 0x00000001; /* Enable RDA interrupts */
}




void UART0_TxChar(char ch) /* A function to send a byte on UART0 */
{
	U0IER = 0x00000000; /* Disable RDA interrupts */
	U0THR = ch;
	while( (U0LSR & 0x40) == 0 );	/* Wait till THRE bit becomes 1 which tells that transmission is completed */
	U0IER = 0x00000001; /* Enable RDA interrupts */
}




void UART0_SendString(char* str) /* A function to send string on UART0 */
{
	U0IER = 0x00000000; /* Disable RDA interrupts */
	uint8_t i = 0;
	while( str[i] != '\0' )
	{
		UART0_TxChar(str[i]);
		i++;
	}
	U0IER = 0x00000001; /* Enable RDA interrupts */
}




__irq void UART0_Interrupt(void)
{
	int iir_value;
	char received_char;
	iir_value = U0IIR;
		received_char = U0RBR;
			if( received_char == '$' )
			{
				GGA_Index = 0;
				CommaCounter = 0;
				IsItGGAString = false;
			}
			else if( IsItGGAString == true )	/* If $GPGGA string */
			{
				if ( received_char == ',' )	
				{
					GGA_Comma_Pointers[CommaCounter++] = GGA_Index;	/* Store locations of commas in the string in a buffer */
				}
				GGA_String[GGA_Index++] = received_char;	/* Store the $GPGGA string in a buffer */
			}
			else if( ( GGA[0] == 'G' ) && ( GGA[1] == 'G' ) && ( GGA[2] == 'A' ) )	/* If GGA string received */
			{
				IsItGGAString = true;
				GGA[0] = GGA[1] = GGA[2] = 0;
			}
			else	/* Store received character */
			{
				GGA[0] = GGA[1];
				GGA[1] = GGA[2];
				GGA[2] = received_char;
			}
	VICVectAddr = 0x00;
}




void get_Time(void)
{
	U0IER = 0x00000000; /* Disable RDA interrupts */
	
	uint8_t time_index=0;
	uint8_t index;
	uint16_t hour, min, sec;
	uint32_t Time_value;

	/* parse Time in GGA string stored in buffer */
	for(index = 0; GGA_String[index]!=','; index++)
	{		
		Time_Buffer[time_index] = GGA_String[index];
		time_index++;
	}	
	Time_value = atol(Time_Buffer);               /* convert string to integer */
	hour = (Time_value / 10000);                  /* extract hour from integer */
	min = (Time_value % 10000) / 100;             /* extract minute from integer */
	sec = (Time_value % 10000) % 100;             /* extract second from integer*/

	sprintf(Time_Buffer, "%d:%d:%d", hour,min,sec);
	
	U0IER = 0x00000001; /* Enable RDA interrupts */
}




void get_Latitude(uint16_t Latitude_Pointer)
{
	U0IER = 0x00000000; /* Disable RDA interrupts */
	
	uint8_t lat_index = 0;
	uint8_t index = (Latitude_Pointer+1);
	
	/* parse Latitude in GGA string stored in buffer */
	for(;GGA_String[index]!=',';index++)
	{
		Latitude_Buffer[lat_index]= GGA_String[index];
		lat_index++;
	}
	
	float lat_decimal_value, lat_degrees_value;
	int32_t lat_degrees;
	lat_decimal_value = atof(Latitude_Buffer);	/* Latitude in ddmm.mmmm */       
	
	/* convert raw latitude into degree format */
	lat_decimal_value = (lat_decimal_value/100);	/* Latitude in dd.mmmmmm */
	lat_degrees = (int)(lat_decimal_value);	/* dd of latitude */
	lat_decimal_value = (lat_decimal_value - lat_degrees)/0.6;	/* .mmmm/0.6 (Converting minutes to eequivalent degrees) */ 
	lat_degrees_value = (float)(lat_degrees + lat_decimal_value);	/* Latitude in dd.dddd format */
	
	sprintf(Latitude_Buffer, "%f", lat_degrees_value);
	
	U0IER = 0x00000001; /* Enable RDA interrupts */
}




void get_Longitude(uint16_t Longitude_Pointer)
{
	U0IER = 0x00000000; /* Disable RDA interrupts */
	
	uint8_t long_index = 0;
	uint8_t index = (Longitude_Pointer+1);
	
	/* parse Longitude in GGA string stored in buffer */
	for(;GGA_String[index]!=',';index++)
	{
		Longitude_Buffer[long_index]= GGA_String[index];
		long_index++;
	}
	
	float long_decimal_value, long_degrees_value;
	int32_t long_degrees;
	long_decimal_value = atof(Longitude_Buffer);	/* Longitude in dddmm.mmmm */
	
	/* convert raw longitude into degree format */
	long_decimal_value = (long_decimal_value/100);	/* Longitude in ddd.mmmmmm */
	long_degrees = (int)(long_decimal_value);	/* ddd of Longitude */
	long_decimal_value = (long_decimal_value - long_degrees)/0.6;	/* .mmmmmm/0.6 (Converting minutes to eequivalent degrees) */
	long_degrees_value = (float)(long_degrees + long_decimal_value);	/* Longitude in dd.dddd format */
	
	sprintf(Longitude_Buffer, "%f", long_degrees_value);
	
	U0IER = 0x00000001; /* Enable RDA interrupts */
}




void get_Altitude(uint16_t Altitude_Pointer)
{
	U0IER = 0x00000000; /* Disable RDA interrupts */
	
	uint8_t alt_index = 0;
	uint8_t index = (Altitude_Pointer+1);
	
	/* parse Altitude in GGA string stored in buffer */
	for(;GGA_String[index]!=',';index++)
	{
		Altitude_Buffer[alt_index]= GGA_String[index];
		alt_index++;
	}
	
	U0IER = 0x00000001; /* Enable RDA interrupts */
}




int main(void)
{
	GGA_Index = 0;
	memset(GGA_String, 0 , 150);
	memset(Latitude_Buffer, 0 , 15);
	memset(Longitude_Buffer, 0 , 15);
	memset(Time_Buffer, 0 , 15);
	memset(Altitude_Buffer, 0 , 8);
	LcdInit();	// Initialize LCD
	                          //Port 0 is now acting as a input pin
	delay_ms(1000);
	VICVectAddr0 = (unsigned) UART0_Interrupt;	/* UART0 ISR Address */
	VICVectCntl0 = 0x00000026;	/* Enable UART0 IRQ slot */
	VICIntEnable = 0x00000040;	/* Enable UART0 interrupt */
	VICIntSelect = 0x00000000;	/* UART0 configured as IRQ */
	UART0_init();
  clear_lcd();
  delay_ms(4000);
	//clear_lcd();
	  UART0_SendString("ATE0\r\n");		/* send ATE0 to check module is ready or not */
		UART0_SendString("AT\r\n");		/* send ATE0 to check module is ready or not */
		delay_ms(500);
		UART0_SendString("AT+CMGF=1\r\n");	/* select message format as text */
	  delay_ms(1000);
		 LcdCmd(0xc0) ;  // Force cursor to beginning of 1st line
	   DisplayRow (2,"Alcohol Test");	// Display on 2nd row
		delay_ms(5000);
	 clear_lcd();
	 
	while(1)
	{  
		if((IO0PIN & (1<<16)) ==0)     //Checking 16th pin of Port 1
		{
			IOSET1 |=0X01000000;   //Port 0's all pins are high now (LED is glowing)
		//UART0_SendString("Alcohol detected\r\n");	
			//UART0_SendString(GGA_String);
		//UART0_SendString("\r\n");
			UART0_SendString("AT+CMGS=\r\n");
		UART0_TxChar('"');
		UART0_SendString("9242838716");
		UART0_TxChar('"');
		UART0_SendString("\r\n");
		delay_ms(1000);
		UART0_SendString("UTC Time : ");
		get_Time();
		UART0_SendString(Time_Buffer);
		UART0_SendString("\r\n");
		UART0_SendString("Latitude : ");
		get_Latitude(GGA_Comma_Pointers[0]);
		UART0_SendString(Latitude_Buffer);
		UART0_SendString("\r\n");
		UART0_SendString("Longitude : ");
		get_Longitude(GGA_Comma_Pointers[2]);
		UART0_SendString(Longitude_Buffer);
		UART0_SendString("\r\n");
		UART0_SendString("Altitude : ");
		get_Altitude(GGA_Comma_Pointers[7]);
		UART0_SendString(Altitude_Buffer);
		UART0_SendString("\r\n");		
		UART0_SendString("\r\n");
		memset(GGA_String, 0 , 150);
		memset(Latitude_Buffer, 0 , 15);
		memset(Longitude_Buffer, 0 , 15);
		memset(Time_Buffer, 0 , 15);
		memset(Altitude_Buffer, 0 , 8);
		delay_ms(1000);
		UART0_TxChar(0X1A);
		 clear_lcd();
			LcdCmd(0x80) ;  // Force cursor to beginning of 1st line
		  DisplayRow (1,"Ur Drunk");	// Display on 1st row	
		delay_ms(5000);
		
		 clear_lcd();
		}
    else
		{
			IOCLR1 |=0X01000000;   //Port 0's all pins are low now
			delay_ms(2000);
			 clear_lcd();
			delay_ms(1000);
			LcdCmd(0x80) ;  // Force cursor to beginning of 1st line
		  DisplayRow (1,"Passed");	// Display on 1st row	
		delay_ms(3000);
    
			 clear_lcd();
		}
	
	
		
	}
}

void LcdInit(void)
{
	IO1DIR = 0x003F0000;		// Configure P1.24(EN) and P1.25(RS) as Output
	IO1CLR = 0x003F0000;		// CLEAR(0) P1.24(EN) and P1.25(RS)
  IO1DIR |=0X01000000;                   //Port 1 is now acting as a output pin
  IO0DIR &= 0XFFFEFFFF;

	DelayMs(6);
  Send_Command(0x03);
  
  DelayMs(6);
  Send_Command(0x03);
  Delay250();
  Send_Command(0x03);
  Delay250();
  Send_Command(0x02);
  Delay250();
  LcdCmd(0x28); //Function Set: 4-bit, 2 Line, 5x7 Dots 
  LcdCmd(0x08); //Display Off, cursor Off
  LcdCmd(0x0c); //Display On, cursor Off
  LcdCmd(0x06); //Entry Mode
}

/* send command on on data lines (D4 to D7) */
void Send_Command(unsigned char cmd)
{
	if (cmd & 0x01)  
		IO1SET = (1<<18);
	else
		IO1CLR = (1<<18);	

	if (cmd & 0x02)	
		IO1SET = (1<<19);
	else
		IO1CLR = (1<<19);
		
	if (cmd & 0x04) 
		IO1SET = (1<<20);
	else
		IO0CLR = (1<<20);	

	if (cmd & 0x08) 
		IO1SET = (1<<21);
	else
		IO1CLR = (1<<21);
		
	IO1CLR = 0x00010000;	// RS is LOW, for Instruction Register and EN is LOW 
	SmallDelay();
	IO1SET = 0x00020000;	// SET(1) EN 
	SmallDelay();
	IO1CLR = 0x00020000;	// CLEAR(0) EN 
	SmallDelay();
}

/* send data on on data lines (D4 to D7) */
void Send_Data(unsigned char dat)
{
	if (dat & 0x01)
		IO1SET = (1<<18);
	else
		IO1CLR = (1<<18);
		
	if (dat & 0x02)
		IO1SET = (1<<19);
	else
		IO1CLR = (1<<19);
		
	if (dat & 0x04)
		IO1SET = (1<<20);
	else
		IO1CLR = (1<<20);

	if (dat & 0x08)
		IO1SET = (1<<21);
	else
		IO1CLR = (1<<21);
		
 	
	IO1SET = 0x00010000;	// RS is LOW, for Instruction Register and EN is LOW 
	SmallDelay();
	IO1SET = 0x00020000;	// SET(1) EN 
	SmallDelay();
	IO1CLR = 0x00020000;	// CLEAR(0) EN 
	SmallDelay() ;	
}

void LcdCmd(unsigned char cmd)
{
	Send_Command(cmd >> 4);
	Send_Command(cmd);
	Delay250();
	Delay250();
}
void LcdDat(unsigned char dat)
{
	Send_Data(dat >> 4);
	Send_Data(dat);
	Delay250();
	Delay250();
}
void DisplayRow (int row, char *str)
{
/*
  pass pointer to 16 character string
  displayes the message on line1 or line2 of LCD, depending on whether row is 1 or 2.
*/
  int k ;
  for(k = 0 ; k < 16 ; k ++)
  {
    if (str[k])
      LcdDat(str[k]) ;
    else
      break ;
  }
  while(k < 16)
  {
    LcdDat(' ') ;
    k ++ ;
  }
}


void SmallDelay(void)
{
	int i;
	for(i=0;i<100;i++);	
}

void Delay250(void)
{
	int k,j ;
	j =200 ;
	for(k = 0 ; k < 100 ; k ++)
	{
		j-- ;
	}
}
void DelayMs(int n)
{
	int k ;
	for(k = 0 ; k < n ; k ++)
	{
		Delay250() ;
		Delay250() ;
	}
}

