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

void Init() {
    /*Oscillator setup*/
    IRCF2 = 0x01;
    IRCF1 = 0x00;
    IRCF0 = 0x01;

    /*Disabling analogue inputs and comparators*/
    ANSEL = 0x00;
    ANSELH = 0x00;

    /*Disabling comparators*/
    C1ON = 0;
    C2ON = 0;

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

    /*I/O configurations*/
    TRISC = 0x00; //Configure for output
    TRISB = 0x10; //All But RB4 set as output for Port B
    TRISB5 = 1; //Set as input
    TRISB7 = 0; //Set as output

}//End init()

void main(void) {
    /*Initialize Timers and Ports*/
    Init();
    /*Initialize the LCD Screen*/
    Lcd_Init();
    Lcd_Clear();

    /*Four second delay waiting for the ESP8266 to start*/
    __delay_ms(4000);

    /*Communicating with the ESP8266*/

    /*Checking startup*/
    __delay_ms(200);
    UART_Write_Text("AT\r\n");
    newCheck();

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
    UART_Write_Text("AT+CIPMUX=1\r\n");
    newCheck();

    /*Connecting to the router*/
    UART_Write_Text("AT+CWJAP=\"NOKIA 909_0136\",\"4904aA!!\"\r\n");
    newCheck();
    __delay_ms(15000);


    /*Create a connection*/
    UART_Write_Text("AT+CIPSTART=3,\"TCP\",\"192.168.137.188\",4000\r\n");
    newCheck();
    __delay_ms(1000);

    while(1){
    __delay_ms(4000); 
    LC7 = 1;
    __delay_ms(4000);
    LC7 = 0;
        
    /*Specify number of bits to send*/
    UART_Write_Text("AT+CIPSEND=3,10\r\n");
    waitToSend();
    __delay_ms(200);

    /*Send data*/
    UART_Write_Text("111111\r\n");
    __delay_ms(200);

    }
    /*Close connection*/
    UART_Write_Text("AT+CIPCLOSE=3\r\n");
    newCheck();


    while (1) {

    }//End infinite while loop

}//End main