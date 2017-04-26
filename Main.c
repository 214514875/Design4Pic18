/*
 * File:   Main.c
 * Author: Denver Pillay
 *
 * Created on 21 February 2017, 9:08 AM
 */


// PIC18F14K50 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1L
#pragma config CPUDIV = NOCLKDIV// CPU System Clock Selection bits (No CPU System Clock divide)
#pragma config USBDIV = OFF     // USB Clock Selection bit (USB clock comes directly from the OSC1/OSC2 oscillator block; no divide)

// CONFIG1H
#pragma config FOSC = IRCCLKOUT // Oscillator Selection bits (Internal RC oscillator, CLKOUT function on OSC2)
#pragma config PLLEN = OFF      // 4 X PLL Enable bit (PLL is under software control)
#pragma config PCLKEN = ON      // Primary Clock Enable bit (Primary clock enabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRTEN = OFF     // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware only (SBOREN is disabled))
#pragma config BORV = 19        // Brown-out Reset Voltage bits (VBOR set to 1.9 V nominal)

// CONFIG2H
#pragma config WDTEN = OFF      // Watchdog Timer Enable bit (WDT is controlled by SWDTEN bit of the WDTCON register)
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config HFOFST = ON      // HFINTOSC Fast Start-up bit (HFINTOSC starts clocking the CPU without waiting for the oscillator to stablize.)
#pragma config MCLRE = OFF       // MCLR Pin Enable bit (MCLR pin enabled; RA3 input pin disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = OFF         // Single-Supply ICSP Enable bit (Single-Supply ICSP enabled)
#pragma config BBSIZ = OFF      // Boot Block Size Select bit (1kW boot block size)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 not code-protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Table Write Protection bit (Block 0 not write-protected)
#pragma config WRT1 = OFF       // Table Write Protection bit (Block 1 not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot block not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot block not protected from table reads executed in other blocks)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define _XTAL_FREQ 4000000
#define RS LC3
#define EN LC4
#define D4 LC0
#define D5 LC1
#define D6 LC2
#define D7 LC6

#include <xc.h>
#include <string.h>
#include "lcd.h"
#include "UART Library.h"
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))     //Method to check a specific bit

int fall_count=0;
unsigned short temp;
unsigned short base_bpm=0;   //Variable for patients base_bpm
unsigned short current_bpm=0;   //Variable to count each pulse within 1 minute
unsigned short count_overflow=0;
unsigned short count_beats=0;

void init()
{
    //Oscillator configurations
    IRCF2=0x01;                 //Configure internal oscillator to 4MHZ
    IRCF1=0x00;
    IRCF0=0x01;
    
    /*Disabling comparators*/
    C1ON = 0;
    C2ON = 0;
    
    //I/O configurations
    TRISC = 0x00;               //Configure for output
    TRISA4 = 0x01;              //Configure for input for interrupts from accelerometer
    TRISA5 = 0x00;              //Configure for output
    TRISC5 =0x01;               //Set to input for input capture module CCP1
    LATA5 = 0x01;
    ANSEL = 0x00;               //Set all ports for digital I/O
    ANSELH = 0x00;
    
    //Timer configurations
    T3CON = 0b00111001; //Set TMR3 with pre-scaler 8, On, and for CCP1 to use TMR3, internal clock
    
    //CCP2 Module configurations (capture)
    CCP1CON = 0b00000101; //Capture on every rising edge
    
    //I2C Master configuration
    SSPCON1 = 0b00101000;            //SSP Module as Master
    SSPCON2 = 0x00;
    SSPSTAT = 0x00;
    SSPADD = 0x09;                   //Set BAUD Rate for SCL=100kHz
    //TRISB = 0x10; //All But RB4 set as output for Port B
    TRISB5 = 1; //Set as input
    TRISB7 = 0; //Set as output
    TRISB4 = 0x01;                   //Setting SDA as input
    TRISB6 = 0x0122;                   //Setting SCL as input
    
    /*EUSART Configuration*/
    TXEN = 1;
    SYNC = 0; //0
    SPEN = 1;
    CREN = 1;
    /*Baud rate of ~115200*/
    BRGH = 1; //1
    BRG16 = 1; //1
    SPBRGH = 0; //Baud Rate of ~ 9600
    SPBRG = 8;
    //CKTXP = 1;
    
    //Interrupt configurations
    IOCA4 = 0x01;
    INTCON = 0b11001000;             //Enable GIE,PEIE,RABIE
    CCP1IF = 0x00;                   //Clear CCP1 interrupt flag
    CCP1IE = 0x01;                   //Enable CCP1 interrupts
}//End init()

//Kshir Start
/******************************************************************************/
/*                            I2C functions                                   */
/******************************************************************************/
void I2C_wait()
{
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F)); //Transmit is in progress
}

void I2C_start()
{ 
    GIE=0x00;           //Disable all interrupts
    I2C_wait();    
    SEN = 1;             //Initiate start condition
}

void I2C_repeatedStart()
{
  I2C_wait();
  RSEN = 1;           //Initiate repeated start condition
}

void I2C_stop()
{
  I2C_wait();
  PEN = 1;           //Initiate stop condition
  I2C_wait();
  GIE = 0x01;
}

void I2C_write(unsigned a)
{
  I2C_wait();
  SSPBUF = a;         //Write data to SSPBUF
}

unsigned short I2C_read(unsigned short a)
{
  unsigned short tmp;
  I2C_wait();
  RCEN = 1;             //Initiate receive condition
  I2C_wait();
  tmp = SSPBUF;         //Read data from SSPBUF
  I2C_wait();
  ACKDT = (a)?0:1;      //Acknowledge bit
  ACKEN = 1;            //Acknowledge sequence
  return tmp;           //Return data byte read
}

/******************************************************************************/
/*                          Sensor functions                                  */
/******************************************************************************/

void accelerometer_write(unsigned short add,unsigned short data) //Write data to specified register in accelerometer
{
    I2C_start();
    I2C_write(0b10100110);    //7 bit address + write
    I2C_wait();
    if(ACKSTAT==0)
    {
        I2C_write(add);      //Register address
        I2C_wait();
        if(ACKSTAT==0)
        {
            I2C_write(data);  //Data to be written to address
        }//end if
    }//end if
    I2C_stop();
}//end accelerometer_write()

unsigned short accelerometer_read(unsigned short add)
{
    unsigned short tmp;
    I2C_start();
    I2C_write(0b10100110);     //7 bit address + write
    I2C_wait();
    if(ACKSTAT==0)
    {
        I2C_write(add);      //Register address
        I2C_wait();
        if(ACKSTAT==0)
        {
            I2C_stop();
            I2C_start();
            I2C_write(0b10100111);  //7 bit address + read
            I2C_wait();
            if(ACKSTAT==0)
            {
                tmp = I2C_read(0);
                I2C_stop();
                return tmp;
            }//end if
        }//end if
    }//end if
    I2C_stop();
}//end accelerometer_read())

void accelerometer_init()               //Initialize registers of accelerometer
{
    //accelerometer_write(0x1E,0x06);
    //accelerometer_write(0x1F,0xF4);     //Y offset
    //accelerometer_write(0x20,0xF5);     //Z offset
    accelerometer_write(0x24,0x30);     //Set THRES_ACT to 3g
    accelerometer_write(0x25,0x03);     //Set THRES_INACT to 0.1875g
    accelerometer_write(0x26,0x02);     //Set TIME_INACT to 2s
    accelerometer_write(0x27,0xFF);     //Set Inactivity = AC-coupled and Activity = AC-coupled
    accelerometer_write(0x28,0x07);     //Set THRESH_FF to 0.4375g
    accelerometer_write(0x29,0x0A);     //Set TIME_FF to 200ms
    accelerometer_write(0x2D,0x08);     //Start measurement
    accelerometer_write(0x2E,0x1C);     //Enable interrupts for: Activity,Inactivity and Free-Fall
    accelerometer_write(0x2F,0x00);     //Map all interrupts to pin INT1
    accelerometer_write(0x31,0x0B);     //Data format: +/-16g, 13-bit right alignment,high level interrupt, I2C interface
}//accelerometer_init()

void temperature()
{
    I2C_write(0b10011000);
    if(ACKSTAT==0)
    {
        I2C_write(0x01);
        if(ACKSTAT==0)
        {
            I2C_start();                //Start condition
            I2C_write(0b10011001);      //7 bit address + Read
            if(ACKSTAT==0)
            {
                temp = I2C_read(0);         //Read + Acknowledge 
                I2C_stop();                 //Stop condition
            }
        }
    }
    I2C_stop();                 //Stop condition
}//End temperature function

void check_temp()
{
    if(temp<30)
    {
        LATA4=0x01;
        LATA5=0x00;
    }//End if
    else if(temp>30)
    {
        LATA4=0x01;
        LATA5=0x00;
    }//End else if
    else
    {
        LATA5=0x01;
        LATA4=0x00;
    }//End else
}//End check_temp()

void Heart_rate()
{   
    count_beats = 0;
    count_overflow=0;
    CCPR1H = 0;
    CCPR1L = 0;
    CCP1IF=0x00;
    CCP1IE = 0x01;
    TMR3IE = 0x01;
    __delay_ms(4000);
    //CCP1IE = 0x00;
    //TMR3IE = 0x00;
    current_bpm = ((((count_overflow*65535+(CCPR1))*8*0.000001)*60)*count_beats);
    
}//End Heart_rate()

void Check_heart_rate()
{
    if(current_bpm < 60)
    {
        LATC3 = 0x00;
        LATC4 = 0x01;
    }
    else if(current_bpm > 100)
    {
        LATC3 = 0x00;
        LATC4 = 0x01;
    }
    else if(((current_bpm-base_bpm)/base_bpm)*100 > 10 || ((base_bpm-current_bpm)/base_bpm)*100)
    {
        LATC3 = 0x00;
        LATC4 = 0x01;
    }
    else
    {
        LATC4 = 0x00;
        LATC3 = 0x01;
    }
}

/******************************************************************************/
/*                              Interrupts                                    */
/******************************************************************************/
void interrupt ISR()
{
    GIE=0;
    int tmp = PORTA;
    if(RABIF)        //Has there been a change on RA4
    {
        //if(CHECK_BIT(accelerometer_read(0x30),3)==1 && fall_count==1)    //Is the activity bit set
        {
            LATA5=~LATA5;
            accelerometer_read(0x30);
        }//End if
        //else if(CHECK_BIT(accelerometer_read(0x30),4)==1)
        {
          //  fall_count = 1;
        }
        //else
        {
          //  fall_count = 0;
        }
        RABIF=0x00;
    }//End if
    
    if(TMR3IF==0x01)
    {
        count_overflow++;
        TMR3IF = 0x00;
    }//end if

    if(CCP1IF == 0x01 && count_beats == 0)
    {
        CCPR1H = 0;
        CCPR1L = 0;
        count_overflow = 0;
        count_beats++;
        CCP1IF=0x00;
        //LATC=0x01;
    }//end else if
    
    if(CCP1IF == 0x01 && count_beats == 1)
    {
        CCP1IE=0x00;
        TMR3IE=0x00;
        //LATC=0x02;
        //Don't clear interrupt flag so that another capture doesn't occur
    }//end else if
    
    GIE = 1;
}//End ISR())
//Kshir End


void main(void) {
    /*Initialize Timers and Ports*/
    init();
    accelerometer_init();
    /*Initialize the LCD Screen*/
    //Lcd_Init();
    //Lcd_Clear();

    /*Four second delay waiting for the ESP8266 to start*/
    __delay_ms(4000);

    /*Communicating with the ESP8266*/

    /*Checking startup*/
    __delay_ms(200);
    UART_Write_Text("AT\r\n");
    newCheck();

    /*Reset*/
    __delay_ms(200);
    UART_Write_Text("AT+RST\r\n");
    newCheck();

    __delay_ms(4000);

    /*Disable Echo*/
    __delay_ms(200);
    UART_Write_Text("ATE0\r\n");
    newCheck();

    /*Enable Client mode only*/
    __delay_ms(200);
    UART_Write_Text("AT+CWMODE=1\r\n");
    newCheck();

    /*Enable Multiple Connections*/
    __delay_ms(200);
    UART_Write_Text("AT+CIPMUX=0\r\n");
    newCheck();

    /*Set transmission to normal transmission mode*/
    __delay_ms(4000);
    UART_Write_Text("AT+CIPMODE=0\r\n");
    newCheck();

    /*Connecting to the router*/
    UART_Write_Text("AT+CWJAP=\"NOKIA 909_0136\",\"4904aA!!\"\r\n");
    //UART_Write_Text("AT+CWJAP=\"HUAWEI P8 lite\",\"12345678\"\r\n");
    newCheck();
    __delay_ms(15000);

    char str [50];
    int x;
    while (1) {
        /*Create a connection*/
        Lcd_Clear();
        Lcd_Set_Cursor(1, 1);
        //UART_Write_Text("AT+CIPSTART=\"TCP\",\"192.168.4.1\",1000\r\n");
        UART_Write_Text("AT+CIPSTART=\"TCP\",\"192.168.137.188\",3000\r\n");
        x = newCheckTimeout();

        if (x == 0) {
            break;
        }
        
        temperature();
        Heart_rate();

        /*Start the data sending*/
        UART_Write_Text("AT+CIPSEND=5\r\n");
        waitToSend();
        __delay_ms(100);

        //UART_Write(temp);
        //UART_Write(current_bpm);
        UART_Write_Text("D&N\r\n");
        __delay_ms(1000);

        UART_Write_Text("AT+CIPCLOSE\r\n");
        __delay_ms(1000);
    }

    LC7 = 0x1;

    Lcd_Clear();
    Lcd_Set_Cursor(1, 1);
    UART_Write_Text("AT+CWQAP\r\n");
    newCheck();
    __delay_ms(1000);

    /*Connecting to the router*/
    UART_Write_Text("AT+CWJAP=\"NOKIA 909_0136\",\"4904aA!!\"\r\n");
    //UART_Write_Text("AT+CWJAP=\"HUAWEI P8 lite\",\"12345678\"\r\n");
    newCheck();
    __delay_ms(15000);

    while (1) {
        /*Create a connection*/
        Lcd_Clear();
        Lcd_Set_Cursor(1, 1);
        //UART_Write_Text("AT+CIPSTART=\"TCP\",\"192.168.137.188\",3000\r\n");
        //UART_Write_Text("AT+CIPSTART=\"TCP\",\"192.168.43.84\",3000\r\n");
        /*IP Address for the ESP8266 Server*/
        x = newCheckTimeout();

        if (x == 0) {
            break;
        }

        /*Start the data sending*/
        UART_Write_Text("AT+CIPSEND=8\r\n");
        waitToSend();
        __delay_ms(100);

        UART_Write_Text("Denver\r\n");
        __delay_ms(1000);

        UART_Write_Text("AT+CIPCLOSE\r\n");
        __delay_ms(1000);
    }

    UART_Write_Text("+++");
    __delay_ms(100);



}//End main