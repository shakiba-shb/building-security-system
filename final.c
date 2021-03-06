#include <mega32a.h>
#include <delay.h>
#include <stdio.h>
#include <alcd.h>

#define c1 PINC.4 
#define c2 PINC.5 
#define c3 PINC.6 
#define c4 PINC.7
#define FIRE_THRESHOLD 639

int people_inside1();
int people_inside2();

// Declare your global variables here
int fire1,fire2 = 0;

// External Interrupt 0 service routine
interrupt [EXT_INT0] void ext_int0_isr(void)
{

    PORTD.7 = 1;

}

// External Interrupt 1 service routine
interrupt [EXT_INT1] void ext_int1_isr(void)
{

    PORTD.7 = 1;

}

// Timer1 overflow interrupt service routine
interrupt [TIM1_OVF] void timer1_ovf_isr(void)
{
// Reinitialize Timer1 value
TCNT1H=0x85EE >> 8;
TCNT1L=0x85EE & 0xff;
// Place your code here

}

// Voltage Reference: AREF pin
#define ADC_VREF_TYPE ((0<<REFS1) | (0<<REFS0) | (0<<ADLAR))
// Read the AD conversion result
#define FIRST_ADC_INPUT 0
#define LAST_ADC_INPUT 1
unsigned int adc_data[LAST_ADC_INPUT-FIRST_ADC_INPUT+1];

// ADC interrupt service routine
// with auto input scanning
interrupt [ADC_INT] void adc_isr(void)
{
static unsigned char input_index=0;
// Read the AD conversion result
adc_data[input_index]=ADCW;
// Select next ADC input
if (++input_index > (LAST_ADC_INPUT-FIRST_ADC_INPUT))
   input_index=0;
ADMUX=(FIRST_ADC_INPUT | ADC_VREF_TYPE)+input_index;
// Delay needed for the stabilization of the ADC input voltage
delay_us(10);
//
//lcd_clear();
//sprintf(st,"Temp1= %d",adc_data[0]);
//lcd_puts(st);
//sprintf(st,"\nTemp2= %d",adc_data[1]);
//lcd_puts(st);
if (!fire1 && adc_data[0] > FIRE_THRESHOLD){
            lcd_clear();
            lcd_puts("floor1 on fire");
            lcd_gotoxy(0,1); 
            lcd_putchar(people_inside1()+'0');
            lcd_puts(" people inside");
            fire1 = 1;
            PORTD.7 =1;
            PORTD.6 = 0;
            delay_ms(5);  
            putchar('x');
            delay_ms(5); 
            PORTD.6 = 1;           
            }
if (!fire2 && adc_data[1] > FIRE_THRESHOLD){
            lcd_clear();
            lcd_puts("floor2 on fire"); 
            lcd_gotoxy(0,1); 
            lcd_putchar(people_inside2()+'0');
            lcd_puts(" people inside");
            fire2 = 1;
            PORTD.7 =1; 
            PORTD.6 = 0;
            delay_ms(5);
            putchar('X');
            delay_ms(5);
            PORTD.6 = 1;           
            }


}


#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE (1<<RXC)
#define FRAMING_ERROR (1<<FE)
#define PARITY_ERROR (1<<UPE)
#define DATA_OVERRUN (1<<DOR)

// USART Receiver buffer
#define RX_BUFFER_SIZE 12
char rx_buffer[RX_BUFFER_SIZE];

#if RX_BUFFER_SIZE <= 256
unsigned char rx_wr_index=0,rx_rd_index=0;
#else
unsigned int rx_wr_index=0,rx_rd_index=0;
#endif

#if RX_BUFFER_SIZE < 256
unsigned char rx_counter=0;
#else
unsigned int rx_counter=0;
#endif

// This flag is set on USART Receiver buffer overflow
bit rx_buffer_overflow;
int inside_counter = 0;
char shift[4] = {0xFE, 0xFD, 0xFB, 0xF7 };
char layout[16]= {'1','2','3','x',
                        '4','5','6','x',
                        '7','8','9','x',
                        'x','0','x','x'};
char position;
                        
// USART Receiver interrupt service routine
interrupt [USART_RXC] void usart_rx_isr(void)
{

char status,data;
status=UCSRA;
data=UDR;

if (data == 'x'){
    lcd_clear();
    lcd_puts("floor1 on fire!");  
    rx_counter = 0;
    rx_wr_index = 0; 
    rx_rd_index=0;
    } 
if (data == 'X'){
    lcd_clear();
    lcd_puts("floor2 on fire!");  
    rx_counter = 0;
    rx_wr_index = 0; 
    rx_rd_index=0;
    } 
if (data == 'z'){
    lcd_clear();
    lcd_puts("Evacuated");
    }
if (data == 'y'){
    lcd_clear();
    lcd_puts("System off."); 
    }

if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0)
   {
   rx_buffer[rx_wr_index++]=data;
   
#if RX_BUFFER_SIZE == 256
   // special case for receiver buffer size=256
   if (++rx_counter == 0) rx_buffer_overflow=1;
#else
   if (rx_wr_index == RX_BUFFER_SIZE) rx_wr_index=0;
   if (++rx_counter == RX_BUFFER_SIZE)
      {
      rx_counter=0;
      rx_buffer_overflow=1;
      }
#endif
   }
}

#ifndef _DEBUG_TERMINAL_IO_
// Get a character from the USART Receiver buffer
#define _ALTERNATE_GETCHAR_
#pragma used+
char getchar(void)
{
char data;
while (rx_counter==0);
data=rx_buffer[rx_rd_index++];
#if RX_BUFFER_SIZE != 256
if (rx_rd_index == RX_BUFFER_SIZE) rx_rd_index=0;
#endif
#asm("cli")
--rx_counter;
#asm("sei")
return data;
}
#pragma used-
#endif

// USART Transmitter buffer
#define TX_BUFFER_SIZE 16
char tx_buffer[TX_BUFFER_SIZE];

#if TX_BUFFER_SIZE <= 256
unsigned char tx_wr_index=0,tx_rd_index=0;
#else
unsigned int tx_wr_index=0,tx_rd_index=0;
#endif

#if TX_BUFFER_SIZE < 256
unsigned char tx_counter=0;
#else
unsigned int tx_counter=0;
#endif

// USART Transmitter interrupt service routine
interrupt [USART_TXC] void usart_tx_isr(void)
{
if (tx_counter)
   {
   --tx_counter;
   UDR=tx_buffer[tx_rd_index++];
#if TX_BUFFER_SIZE != 256
   if (tx_rd_index == TX_BUFFER_SIZE) tx_rd_index=0;
#endif
   }
}

#ifndef _DEBUG_TERMINAL_IO_
// Write a character to the USART Transmitter buffer
#define _ALTERNATE_PUTCHAR_
#pragma used+
void putchar(char c)
{
while (tx_counter == TX_BUFFER_SIZE);
#asm("cli")
if (tx_counter || ((UCSRA & DATA_REGISTER_EMPTY)==0))
   {
   tx_buffer[tx_wr_index++]=c;
#if TX_BUFFER_SIZE != 256
   if (tx_wr_index == TX_BUFFER_SIZE) tx_wr_index=0;
#endif
   ++tx_counter;
   }
else
   UDR=c;
#asm("sei")
}
#pragma used-
#endif



 unsigned char keypad(){
  unsigned char r,b;
      while(1){
        for (r=0; r<4; r++){
         b=4;
         PORTC=shift[r];
         if(c1==0) b=0;
         if(c2==0) b=1;
         if(c3==0) b=2;
         if(c4==0) b=3;
         
          if (!(b==4)){
           position=layout[(r*4)+b];
           while(c1==0);
           while(c2==0);
           while(c3==0);
           while(c4==0);
           delay_ms(50);
           return position;
          }
        }
      }
 }

void clear_buffer(){
    int i=0;
     for (i=0;i<12;i++){
                rx_buffer[i]='0';
                }
     }                                  
     
      int entrance_status1[2]= {0};
    //Sara, Atieh
      int entrance_status2[3]= {0};
    //Zahra, Jack, Shakiba   
    int people_inside1(){
            int j= 0;
            int counter= 0;
            for(j=0;j<2;j++){
                if(entrance_status1[j] == 1)
                    counter++; }
            return counter;
            } 
    int people_inside2(){
            int j= 0;
            int counter= 0;
            for(j=0;j<3;j++){
                if(entrance_status2[j] == 1)
                    counter++; }
            return counter;
            } 
    int keypad_w_index=0;
    char keypad_buffer[3]; 
// External Interrupt 2 service routine           
 interrupt [EXT_INT2] void ext_int2_isr(void)
{
           unsigned char ch;
           ch = keypad();
           if (keypad_w_index == 0) 
            lcd_clear();
           lcd_putchar(ch);  
           PORTC = 0xF0;
           keypad_buffer[keypad_w_index++] = ch;
           if(keypad_w_index > 2){
           keypad_w_index=0;    

           if(keypad_buffer[0] == '3' && keypad_buffer[1] == '3' && keypad_buffer[2] == '3' && fire1 == 1 && adc_data[0] < FIRE_THRESHOLD){
           lcd_clear();
           lcd_puts("System OFF.");
           PORTD.4 = 0;
           PORTD.7 = 0;
           fire1 = 0;
           putchar('y'); 
           }  
           
           if(keypad_buffer[0] == '3' && keypad_buffer[1] == '3' && keypad_buffer[2] == '3' && fire2 == 1 &&  adc_data[1] < FIRE_THRESHOLD){                         
           lcd_clear();
           lcd_puts("System OFF.");
           PORTD.4 = 0;
           PORTD.4 = 7;                     
           fire2 = 0;
           putchar('y');
           }  
           
            if(keypad_w_index == 0 && (keypad_buffer[0] != '3' || keypad_buffer[1] != '3' || keypad_buffer[2] != '3') ){
            lcd_clear();
            lcd_puts("invalid code");
            
           
           if(keypad_buffer[0] == '9' && keypad_buffer[1] == '9' && keypad_buffer[2] == '9'){                          
           lcd_clear();
           lcd_puts("Test mode");
           delay_ms(1000);
           lcd_clear();
            lcd_puts("floor1 on fire");
            lcd_gotoxy(0,1); 
            lcd_putchar(people_inside1()+'0');
            lcd_puts(" people inside");
            fire1 = 1; 
            PORTD.7 = 1; 
            PORTD.6 = 0;
            delay_ms(5);
            putchar('x');  
            delay_ms(5);
            PORTD.6 = 1;
           }
           
          }
 }     
    
}    
    
void lcd_show(){
               
                lcd_clear();
                inside_counter = people_inside1(); 
                lcd_puts("1st floor: ");
                lcd_putchar(inside_counter+'0');
                inside_counter = people_inside2();
                lcd_gotoxy(0,1);
                lcd_puts("2nd floor: ");
                lcd_putchar(inside_counter+'0'); 
                lcd_gotoxy(0,0);
                }
   
void main(void)
{
int inside_counter1,inside_counter2 = 0;
     
DDRA=(0<<DDA7) | (0<<DDA6) | (0<<DDA5) | (0<<DDA4) | (1<<DDA3) | (1<<DDA2) | (0<<DDA1) | (0<<DDA0);
PORTA=(1<<PORTA7) | (1<<PORTA6) | (1<<PORTA5) | (1<<PORTA4) | (0<<PORTA3) | (0<<PORTA2) | (1<<PORTA1) | (1<<PORTA0);

DDRB=(1<<DDB7) | (0<<DDB6) | (0<<DDB5) | (0<<DDB4) | (0<<DDB3) | (0<<DDB2) | (0<<DDB1) | (1<<DDB0);
PORTB=(0<<PORTB7) | (0<<PORTB6) | (0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (1<<PORTB2) | (0<<PORTB1) | (0<<PORTB0);

DDRC=(0<<DDC7) | (0<<DDC6) | (0<<DDC5) | (0<<DDC4) | (1<<DDC3) | (1<<DDC2) | (1<<DDC1) | (1<<DDC0);
PORTC=(1<<PORTC7) | (1<<PORTC6) | (1<<PORTC5) | (1<<PORTC4) | (0<<PORTC3) | (0<<PORTC2) | (0<<PORTC1) | (0<<PORTC0);

DDRD=(1<<DDD7) | (1<<DDD6) | (0<<DDD5) | (1<<DDD4) | (0<<DDD3) | (0<<DDD2) | (0<<DDD1) | (0<<DDD0);
PORTD=(0<<PORTD7) | (1<<PORTD6) | (0<<PORTD5) | (0<<PORTD4) | (1<<PORTD3) | (1<<PORTD2) | (0<<PORTD1) | (0<<PORTD0);


// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: 125.000 kHz
// Mode: Normal top=0xFFFF
// OC1A output: Disconnected
// OC1B output: Disconnected
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer Period: 0.125 s
// Timer1 Overflow Interrupt: On
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
TCCR1A=(0<<COM1A1) | (0<<COM1A0) | (0<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (0<<WGM10);
TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (0<<CS12) | (1<<CS11) | (1<<CS10);
TCNT1H=0xC2;
TCNT1L=0xF7;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=(0<<OCIE2) | (0<<TOIE2) | (0<<TICIE1) | (0<<OCIE1A) | (0<<OCIE1B) | (1<<TOIE1) | (0<<OCIE0) | (0<<TOIE0);

// ADC initialization
// ADC Clock frequency: 1000.000 kHz
// ADC Voltage Reference: AREF pin
// ADC Auto Trigger Source: Timer1 Overflow
ADMUX=FIRST_ADC_INPUT | ADC_VREF_TYPE;
ADCSRA=(1<<ADEN) | (0<<ADSC) | (1<<ADATE) | (0<<ADIF) | (1<<ADIE) | (0<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
SFIOR=(1<<ADTS2) | (1<<ADTS1) | (0<<ADTS0);


// External Interrupt(s) initialization
// INT0: On
// INT0 Mode: Falling Edge
// INT1: On
// INT1 Mode: Falling Edge
// INT2: On
// INT2 Mode: Falling Edge
GICR|=(1<<INT1) | (1<<INT0) | (1<<INT2);
MCUCR=(1<<ISC11) | (0<<ISC10) | (1<<ISC01) | (0<<ISC00);
MCUCSR=(0<<ISC2);
GIFR=(1<<INTF1) | (1<<INTF0) | (1<<INTF2);



// USART initialization
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART Receiver: On
// USART Transmitter: On
// USART Mode: Asynchronous
// USART Baud Rate: 9600
UCSRA=(0<<RXC) | (0<<TXC) | (0<<UDRE) | (0<<FE) | (0<<DOR) | (0<<UPE) | (0<<U2X) | (0<<MPCM);
UCSRB=(1<<RXCIE) | (1<<TXCIE) | (0<<UDRIE) | (1<<RXEN) | (1<<TXEN) | (0<<UCSZ2) | (0<<RXB8) | (0<<TXB8);
UCSRC=(1<<URSEL) | (0<<UMSEL) | (0<<UPM1) | (0<<UPM0) | (0<<USBS) | (1<<UCSZ1) | (1<<UCSZ0) | (0<<UCPOL);
UBRRH=0x00;
UBRRL=0x33;


// Alphanumeric LCD initialization
// Connections are specified in the
// Project|Configure|C Compiler|Libraries|Alphanumeric LCD menu:
// RS - PORTB Bit 0
// RD - PORTB Bit 1
// EN - PORTB Bit 3
// D4 - PORTB Bit 4
// D5 - PORTB Bit 5
// D6 - PORTB Bit 6
// D7 - PORTB Bit 7
// Characters/line: 16
lcd_init(16);

// Global enable interrupts
#asm("sei")

lcd_puts("1st floor: 0\n");
lcd_puts("2nd floor: 0");

while (1)
      {     
            inside_counter1 = people_inside1();
            inside_counter2 = people_inside2();
      
            if (!PORTD.4 && fire1 == 1 && inside_counter1 == 0){
            lcd_clear();
            lcd_puts("Evacuated!");
            PORTD.4 = 1;
            PORTD.7 = 0; 
            PORTD.6 = 0;
            delay_ms(5);  
            putchar('z');
            delay_ms(5);
            PORTD.6 = 1;  
            } 
            if (!PORTD.4 && fire2 == 1 && inside_counter2 == 0){
            lcd_clear();
            lcd_puts("Evacuated!");
            PORTD.7 = 0;
            PORTD.4 = 1;
            PORTD.6 = 0;
            delay_ms(5);  
            putchar('z');
            delay_ms(5);
            PORTD.6 = 1;
            }          
       
        switch (rx_buffer[8]){
                case '3': {
                    lcd_clear();
                    if (entrance_status1[0]){  
                         lcd_puts("Sara exited.");  
                         entrance_status1[0] = 0;
                    }else if(!fire1){ 
                         lcd_puts("Sara entered.");  
                         entrance_status1[0] = 1;
                         }
                    clear_buffer();     
                    delay_ms(1000);     
                    lcd_show();
                    break; 
                    }
                case '7':  {
                    lcd_clear();
                    if (entrance_status1[1]){  
                         lcd_puts("Atieh exited.");  
                         entrance_status1[1] = 0;
                    }else if(!fire1) { 
                         lcd_puts("Atieh entered.");  
                         entrance_status1[1] = 1;
                         }
                    clear_buffer();     
                    delay_ms(1000);        
                    lcd_show();
                    break; 
                    }
                case '1': {
                    lcd_clear();
                    if (entrance_status2[0]){  
                         lcd_puts("Zahra exited.");  
                         entrance_status2[0] = 0;
                    }else if(!fire2) { 
                         lcd_puts("Zahra entered.");  
                         entrance_status2[0] = 1;
                         } 
                    clear_buffer();     
                    delay_ms(1000);     
                    lcd_show();
                    break;
                    }
                case 'F': { 
                    lcd_clear();
                    if (entrance_status2[1]){  
                         lcd_puts("Jack exited.");  
                         entrance_status2[1] = 0;
                    }else if(!fire2) { 
                         lcd_puts("Jack entered.");  
                         entrance_status2[1] = 1;
                         }  
                    clear_buffer();     
                    delay_ms(1000);     
                    lcd_show();
                    break; 
                    }
                case 'A': {
                    lcd_clear();
                    if (entrance_status2[2]){  
                         lcd_puts("Shakiba exited.");  
                         entrance_status2[2] = 0;
                    }else if(!fire2) { 
                         lcd_puts("Shakiba entered.");  
                         entrance_status2[2] = 1;
                         } 
                    clear_buffer();     
                    delay_ms(1000);      
                    lcd_show();
                    break; 
                    }
                }
                
        
                
      
        
        }        
        
        
        

}
        
        
        
       