/* 
 * File:   Main.c
 * Author: Reshan Seemungal
 * 
 */

#include <p18f4550.h>
#include "xlcd.h"
#include "keypad.h"
#include <p18Cxxx.h>
#include <delays.h>
#include <timers.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwm.h>
#include <capture.h>
#include <adc.h>
#include "ow.h"

//   Configuration
//==========================================================================

extern unsigned char keypad_read();
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config PBADEN = OFF
#define _XTAL_FREQ 4000000    


//Variables
//==============================================================================
//HR Module
int Count_10 = 0, heart_beat = 0, bpmfin = 0, risingEdge = 0;

//HRV Module
unsigned int capture1 = 0, capture2 = 0, riseEdge = 0, interval = 0, overFlw = 0,  prev = 0, num = 0;
float hrv = 0, nn = 0, nn_50 = 0;

//Speaker Module
int sound[2]={150, 155}; //insert notes of song in array
int length[2]={1, 1}; //relative length of each note
int i;

//Glucose Module
float result;
float voltage,adcResults;
int  intADC = 0,decADC;
char adc_Buff[8];

//Temperature Module
unsigned char msbyte=0,ls_byte=0,sign=0, degree= 0xDF;
float         temp_fract=0.0;
unsigned int  temp_int = 0,temp_fract_val=0;
int x;


int i;  


/*Function Definition*/

void isr_function(void);
void initLCD(void);
void Setup_Timers(void);
void Setup_Interrupts(void);
void Setup_Interrupts(void);
void Setup_PWM(void);
void Setup_ADC(void);
void BPM(void);
void HRV(void);


//Interrupt
//==============================================================================

#pragma code
/*****************High priority ISR **************************/
#pragma interrupt isr_function

void isr_function(void) {
    BPM();

    HRV();
    
    if(PIR1bits.ADIF == 1){
        PIR1bits.ADIF = 0;
        
        result = ReadADC();   // Read result
        voltage = ((result*5)/1023);
        intADC = voltage ;
        decADC = (voltage - intADC) * 10;
        adcResults = intADC;
        sprintf(adc_Buff,"ADC: %d.%d", intADC, decADC);
        sprintf(adc_Buff, "ADC: %d",result);
        while( BusyXLCD() );
        SetDDRamAddr(0x10); 
        putsXLCD(adc_Buff);
        result = ReadADC();   // Read result
        voltage = ((result*5)/1023.0);
        ADCON0bits.GO_DONE = 1; 
        
    }
}

/*****************High priority interrupt vector **************************/
#pragma code high_vector=0x08

void interrupt_at_high_vector(void) {
    _asm
    GOTO isr_function
    _endasm
}


//LCD Delays 
//==============================================================================
void DelayFor18TCY( void )
{
 Nop();
 Nop();
 Nop();
 Nop();
 Nop();
 Nop();
 Nop();
 Nop();
 Nop();
 Nop();
 Nop();
 Nop();
}
void DelayPORXLCD (void)
{
 Delay1KTCYx(15); // Delay of 15ms
 // Cycles = (TimeDelay * Fosc) / 4
 // Cycles = (15ms * 4MHz) / 4
 // Cycles = 15,000
 return;
}



void DelayXLCD (void)
{
 Delay1KTCYx(5); // Delay of 5ms
 // Cycles = (TimeDelay * Fosc) / 4
 // Cycles = (5ms * 4MHz) / 4
 // Cycles = 5,000
 return;
}


//Initialization Functions
//==============================================================================
//LCD
void init_XLCD( void )
{
 // configure external LCD
OpenXLCD(FOUR_BIT & LINES_5X7);
while( BusyXLCD() );
WriteCmdXLCD( FOUR_BIT & LINES_5X7 );
while( BusyXLCD() );
WriteCmdXLCD( BLINK_ON );
while( BusyXLCD() );
WriteCmdXLCD( SHIFT_DISP_LEFT );

}

//Timer 0 setup
void Setup_Timers() {
    OpenTimer0(TIMER_INT_ON & T0_SOURCE_INT & T0_PS_1_16 & T0_16BIT);
    INTCONbits.TMR0IE = 1; //Enable Timer0 Interrupt
    INTCONbits.TMR0IF = 0; //Clear Timer0 Interrupt Flag
    INTCON2bits.TMR0IP = 1; //Enable Priority Levels
    TMR1H = 0x00; // clear timer1 
    TMR1L = 0x00;
    T1CON = 0x81; // Timer1 enabled
    
}

//Interrupt Setup
void Setup_Interrupts() {
    INTCONbits.GIEH = 1;// enable high priority interrupts
    INTCONbits.GIE = 1; // enable interrupts 
    INTCONbits.INT0E = 1; // Enables the RBO external interrupt
    INTCONbits.INT0F = 0; //Clear RBO interrupt flag   
    INTCON2bits.INTEDG0 = 1; //Interrupt on rising edge
    PIE1bits.ADIE = 1; // ADC interrupt enabled 
    IPR1bits.ADIP = 1; //ADC interrupt set to high priority 
    PIR1bits.ADIF = 0;////Clear ADC Interrupt Flag
    PIE1bits.CCP1IE = 1; // CCP1 interrupt enabled 
    RCONbits.IPEN = 1; //Enable Priority Levels
    INTCONbits.PEIE = 1;// enable peripheral interrupts
    PIR1 = 0x00; // clear the interrupt flags 
    CCP1CON = 0x05; // CCP1 as capture mode on every rising edge  
    IPR1 = 0x04; // CCP1 interrupt set to high priority 
    
}

void enable_int(){
            INTCONbits.INT0E = 0; // Enables the RB0 external interrupt
            INTCONbits.GIEH = 0; // Enable high priority interrupt
            INTCONbits.GIE = 0; // enable interrupts 
            PIE1bits.CCP1IE = 0; // CCP1 interrupt enabled 
}


//ACD Setup
void Setup_ADC(){
    OpenADC(ADC_FOSC_RC&ADC_RIGHT_JUST&ADC_12_TAD,ADC_CH1&ADC_INT_ON,15);
}


//PWM Setup
void Setup_PWM(){
    TRISCbits.RC1 = 0; //Set RC1 as output for PWM
    OpenTimer2(TIMER_INT_OFF & T0_PS_1_4 & T2_POST_1_1 );
    SetDCPWM2(50);
}



//Heart Rate Functions
void BPM(){
    if (INTCONbits.TMR0IF == 1) { // Check for timer0 overflow
        Count_10 = Count_10 + 1;
        if (Count_10 == 10) {
            CloseTimer0();
            bpmfin = 1;

            INTCONbits.TMR0IF = 0;
        } else {
            INTCONbits.TMR0IF = 0;
            WriteTimer0(0xBDC);// overflow in 1 second
        }
    }
    if (INTCONbits.INT0F == 1) {
        risingEdge = risingEdge + 1;
        INTCONbits.INT0F = 0;
    }
}


int Calc_Bpm() {
    int val60 = 0;
    val60 = heart_beat * 6;
    return val60;
}

void Bpm_LCD(int val60) {
    char BPMbuffer[50];
    sprintf(BPMbuffer, "HR: %d bpm", val60);
    while( BusyXLCD() );
    SetDDRamAddr(0x00); 
    putsXLCD(BPMbuffer);
    
}

//HRV Functions
void HRV(){
    float tempy;
    if (PIR1bits.TMR1IF == 1) {
        PIR1bits.TMR1IF = 0;
        overFlw++;
    }
    if (PIR1bits.CCP1IF == 1) {
        //LATAbits.LATA0 = 1;
        PIR1bits.CCP1IF = 0;
        if (riseEdge == 0) {
            if (num == 0) {
                capture1 = ReadCapture1();
            } else {
                capture2 = ReadCapture1();
            }
        } else if ((riseEdge == 1) && (num == 0)) {
            capture2 = ReadCapture1();
        }
        riseEdge++;
        if ((riseEdge > 1) || (num == 1)) {
            num = 1;
            interval = 65535 * overFlw + capture2 - capture1;
            nn++;
            prev = capture2;
            capture1 = prev;
            tempy = (float) interval / (float) 1000;
            if ((float) tempy > (float) 50) {
                nn_50++;
            }
            riseEdge = 0;
            if (nn > 15) {
                hrv = (float) nn_50 / (float) 15;
                hrv = hrv * 100;                
                CloseCapture1();
                CloseTimer1();
            }
        }
    }
}
int getHrv() {
    int hrvVal = 0;
    hrvVal = hrv;
    return hrvVal;
}

void Hrv_LCD(int hrvVal) {
    char HRVbuffer[20];
    sprintf(HRVbuffer, "HRV: %d%", hrvVal);
     while( BusyXLCD() );
     SetDDRamAddr(0x40); 
     putsXLCD(HRVbuffer);
   
}


//Temperature Functions
void temp_read(){
     
    TRISBbits.RB5 = 0;          //Strong Pullup Output for Transistor
    PORTBbits.RB5 = 0;          //Set RB5 low

    ow_reset();
    ow_write_byte(0xCC); //Skip ROM
    ow_write_byte(0x44); //Convert T Command
    PORTBbits.RB5 = 1; //Turn ON Strong Pull-up
    for (x = 1; x <= 8; x++) { //800ms (750ms is recommended conversion time))
        Delay1KTCYx(100);
    }
    PORTBbits.RB5 = 0; //Turn-off Strong Pull-up4
    ow_reset();
    ow_write_byte(0xCC); //Skip ROM
    ow_write_byte(0xBE); //Read Scratchpad
    ls_byte = ow_read_byte(); //Obtain Least Significant byte.
    msbyte = ow_read_byte(); //Obtain Most Significant byte.
}

void get_int(){
    unsigned char temp=0,int_temp=0;
    temp = ls_byte >>4;             //Shift bit7:bit4 down 4 places to the right.
    int_temp = temp | int_temp;     //Bitwise OR temp with int_temp
    temp = msbyte << 4;            //shift register 4 places to the left
    int_temp = temp | int_temp;     //Bitwise OR temp with int_temp
    
    temp_int = int_temp; 
    
}


void get_fract(){
    
    if (ls_byte & 0x01) {
        temp_fract += 0.0625;
    }
    if (ls_byte & 0x02) {
        temp_fract += 0.125;
    }
    if (ls_byte & 0x04) {
        temp_fract += 0.25;
    }
    if (ls_byte & 0x08) {
        temp_fract += 0.5;
    }
    temp_fract_val = temp_fract;
    
    
}


void display() {
    char temperature[50];
    sprintf(temperature, "Temp:%d.%d%cC", temp_int, temp_fract_val, degree); //pass temperature data to a buffer
    while (BusyXLCD());
    SetDDRamAddr(0x10);
    putsXLCD(temperature);
}

    
 void clear(){
    temp_int = 0;
    temp_fract_val = 0.0;
}
 
//Speaker Functions

void speaker_tone(int i) {
    OpenPWM2(sound[i]); //set PWM frequency according to entries in song array
    Delay1KTCYx(500 * length[i]); //play each note for 500ms
    OpenPWM2(1); //creates short pause                         
    Delay10KTCYx(25); //pauses for 50 ms
}

//Reset variables
void resetVar(){
    capture1=0,capture2=0,riseEdge=0,interval=0,overFlw=0,prev=0,num=0,nn=0,nn_50=0;
    Count_10 = 0,heart_beat = 0,bpmfin = 0,risingEdge = 0;
	intADC = 0;
    
}

//	main function
//==========================================================================
void Start(char key_get) {
    int bpm;
    int hrv;
    int i;
    
    WriteCmdXLCD(0x01);
    resetVar();// set all variables to 0
    
    Setup_Interrupts();
    Setup_Timers();
    Setup_ADC();
    
    
    while (1) {
        Delay1KTCYx(100);     // Delay for 50TCY
        ConvertADC();
        SetDCPWM2(0);
        
        temp_read();
        get_fract();
        get_int();
        display();  //displays temperature
        clear();
        
        if (bpmfin == 0) {
            heart_beat = risingEdge;
            Bpm_LCD("Pease wait"); //display heart rate
            Hrv_LCD("Please wait");  //display HRV
           
        }
        else 
        {
            CloseTimer0();
            bpm = Calc_Bpm();            
            hrv = getHrv();
            Bpm_LCD(bpm);
            Hrv_LCD(hrv); 
            
            //Check to see if reading is out of range
            if(bpm > 100 || bpm  < 60){
                if(i == 2){
                    i = 0;
                }// set speaker to play in loop if BPM is more than 100 or less than 60
                CloseADC();
                Setup_PWM();
                speaker_tone(i);
                i++;
            }
   
        }
        
        key_get = keypad_wait();
        if(key_get == '2'){
            SetDCPWM2(0);            
            enable_int();// enable interrupts
            break;
        }
    }
}


void main( void )
{
    char key_get;  
    
	UCONbits.USBEN = 0; //Disable USB => RC4 + RC5 output
    UCFGbits.UTRDIS = 1;//Disable USB => RC4 + RC5 output
    PORTD = 0x00;
    TRISBbits.RB0 = 1;
    TRISCbits.RC1 = 0;
    
	TRISC = 0b11110100;				// set PORTC I/O
    TRISAbits.RA1 = 1;              // Set RA1 as input for ADC
	init_XLCD();                    //Initialize LCD       
    
    while (1) {
        key_get = keypad_wait();
        if(key_get != 0xFF){
            
            switch(key_get){
                case '1':
                    Start(key_get);              
                    WriteCmdXLCD(0x01);
                    break; 
                   
            }  
        }else{
            
            while( BusyXLCD() );
            SetDDRamAddr(0x00); 
            putrsXLCD("MicroP Project"); 
            while( BusyXLCD() );
            SetDDRamAddr(0x40); 
            putrsXLCD("Group B Exodus"); 
            while( BusyXLCD() );
            SetDDRamAddr(0x10); 
            putrsXLCD("Press one(1)"); 
            while( BusyXLCD() );
            SetDDRamAddr(0x50); 
            putrsXLCD("When Ready");
            
        }
    }
    
 }