    void UART_Read_Text(char *Output, unsigned int length);
unsigned char UART_Read();
char UART_Data_Ready();
void UART_Write_Text(char *text);
void UART_Write(char data);
char UART_TX_Empty();
char UART_Init(const long int baudrate);

/*Reading Text*/
void UART_Read_Text(char *Output, unsigned int length) {
    unsigned int i;
    for (int i = 0; i < length; i++) {
        if (OERR == 1) {
            CREN = 0;
            CREN = 1;
        }
        Output[i] = UART_Read();
    }
}

void Check(unsigned char *data) {
    if (strstr(data, "OK") != NULL) {
        Lcd_Write_String("OK");
    } else {
        Lcd_Write_String("ERROR");
    }
}

void waitToSend()
{
    int i;
    for (i=0;i<10;i++) {
        if (OERR == 1) {
            CREN = 0;
            CREN = 1;
        }
        if(UART_Read() == '>' || UART_Read() == '>'){
            Lcd_Write_String("OK");
            break;
        }
        // Output[i] = UART_Read();
    }
}

/*Trying a method where its tested as it arrives instead of saving and storing*/
void newCheck() {
    while (1) {
        if (OERR == 1) {
            CREN = 0;
            CREN = 1;
        }
        if(UART_Read() == 'O' && UART_Read() == 'K'){
            Lcd_Write_String("OK");
            break;
        }
        // Output[i] = UART_Read();
    }
}

unsigned int newCheckTimeout() {
    char str [20];
    UART_Read_Text(&str,15);
    if(strstr(str,"OK") != 0){
        Lcd_Write_String("OK");
        return 1;
    }else{
        Lcd_Write_String("Disconnected");
        return 0;
    }
}

/*Reading a character*/
unsigned char UART_Read() {
    while (!RCIF);
    return RCREG;
}

/*Checks whether data is ready to be recieved*/
char UART_Data_Ready() {
    return RCIF;
}

/*Writing Text*/
void UART_Write_Text(char *text) {
    int i;
    for (i = 0; text[i] != '\0'; i++)
        UART_Write(text[i]);
}

/*Writing a character*/
void UART_Write(char data) {
    /*  
    while(!TRMT);
    TXREG = data;
     */
    while (!TXIF) /* set when register is empty */
        continue;
    TXREG = data;
}

/*Checking the Transmit register*/
char UART_TX_Empty() {
    return TRMT;
}

/*Initialising the EUSART registers*/
char UART_Init(const long int baudrate) {
    unsigned int x;
    x = (_XTAL_FREQ - baudrate * 64) / (baudrate * 64); //SPBRG for Low Baud Rate
    if (x > 255) //If High Baud Rage Required
    {
        x = (_XTAL_FREQ - baudrate * 16) / (baudrate * 16); //SPBRG for High Baud Rate
        BRGH = 1; //Setting High Baud Rate
    }
    if (x < 256) {
        SPBRG = x; //Writing SPBRG Register
        SYNC = 0; //Setting Asynchronous Mode, ie UART
        SPEN = 1; //Enables Serial Port
        TRISB7 = 0; //As Prescribed in Datasheet
        TRISB5 = 1; //As Prescribed in Datasheet
        CREN = 1; //Enables Continuous Reception
        TXEN = 1; //Enables Transmission
        return 1; //Returns 1 to indicate Successful Completion
    }
    return 0; //Returns 0 to indicate UART initialization failed
}