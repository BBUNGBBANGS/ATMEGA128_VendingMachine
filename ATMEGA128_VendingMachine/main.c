#define F_CPU 14.7456E6
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>

#define sbi(x, y) (x |= (1 << y))  // x의 y 비트를 설정(1)
#define cbi(x, y) (x &= ~(1 << y)) // x의 y 비트를 클리어(0)

// CON 포트는 포트 C와 연결됨을 정의
#define LCD_CON      PORTG
// DATA 포트는 포트 A와 연결됨을 정의
#define LCD_DATA     PORTA
// DATA 포트의 출력 방향 설정 매크로를 정의
#define LCD_DATA_DIR DDRA
// DATA 포트의 입력 방향 설정 매크로를 정의
#define LCD_DATA_IN  PINA
// RS 신호의 비트 번호 정의
#define LCD_RS   2
// RW 신호의 비트 번호 정의
#define LCD_RW   1
// E 신호의 비트 번호 정의
#define LCD_E    0

uint8_t Seven_Segment_Num[11] =
{
    0xC0, //0
    0xF9, //1
    0xA4, //2
    0xB0, //3
    0x99, //4
    0x92, //5
    0x82, //6
    0xD8, //7
    0x80, //8
    0x90, //9
    0x3F // -
}; // 애노드 공통

uint8_t Vending_Machine_Count[8] = 
{
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3
};

uint16_t Vending_Machine_Price[8] = 
{
    500,
    1200,
    800,
    1500,
    2000,
    200,
    2200,
    1700
};

uint8_t Vending_Machine_status;

uint8_t Timer_10ms_flag;
uint8_t Timer_10ms_counter;
uint8_t Timer_100ms_flag;

uint8_t Coin_100,Coin_500,Coin_1000,Coin_5000;
uint8_t Coin_n1,Coin_n10,Coin_n100,Coin_n1000;
uint16_t Coin_Total;

uint8_t Keypad_Num;
uint8_t Selected_Num;

char LCD_Tx_Data[32];

#define SEVEN_SEGMENT_0                 (0)
#define SEVEN_SEGMENT_1                 (1)
#define SEVEN_SEGMENT_2                 (2)
#define SEVEN_SEGMENT_3                 (3)

#define VENDING_MACHINE_IDLE            (0)
#define VENDING_MACHINE_INSERT_COIN     (1)
#define VENDING_MACHINE_SELECT_ITEM     (2)
#define VENDING_MACHINE_ITEM_SELECTED   (3)
#define VENDING_MACHINE_MONEY_ERROR     (4)
#define VENDING_MACHINE_MONEY_FINISH    (5)
#define VENDING_MACHINE_CANCEL          (6)

/* 함수 프로토 타입 설정 */
void Port_Init(void);
void LCD_Init(void);
void Timer1_Init(void);
void ISR_Init(void);
void LCD_Print(void);

void Key_Scan(void);
void LED_Output(void);
void Switch_Scan(void);
void Vending_Machine_Mode(void);

int main(void)
{
    Port_Init();
    LCD_Init();
    Timer1_Init();
    ISR_Init();
    /* Replace with your application code */
    while (1) 
    {
        if(Timer_10ms_flag == 1)
        {
            Timer_10ms_flag = 0;
            Key_Scan();
            Switch_Scan();
            Seven_Segment_Output();
            Vending_Machine_Mode();
        }

        if(Timer_100ms_flag == 1)
        {
            Timer_100ms_flag = 0;
            LED_Output();
            LCD_Print();
        }
    }
}

void Port_Init(void)
{
    DDRE = 0xF0; // 7-세그먼트 선택 포트 출력 설정 (PE4 : FND0, PE5, FND1, PE6 : FND2, PE7 : FND3)
    DDRB = 0xFF; // 7-세그먼트 포트 출력 설정
    DDRC = 0x0F; // PC0~3 : Output , PC4~7 : Input 설정
    DDRA = 0xFF; // LCD Data line 출력 설정 PA0~PA7
    DDRF = 0xFF; // 재고 LED 출력 설정
    DDRG = 0x0F; // LCD Command line 출력 설정 PG0~PG2
}

void LCD_Init(void)
{
	LCD_Transmit_Command(0x38); // Function Set , 2-Line Mode
	_delay_ms(1);		
	LCD_Transmit_Command(0x38); // Function Set , 2-Line Mode
	_delay_ms(1);	
	LCD_Transmit_Command(0x38); // Function Set , 2-Line Mode
	_delay_ms(1);	
	LCD_Transmit_Command(0x0C); // Display ON
	_delay_ms(10);
	LCD_Transmit_Command(0x06); // Increment Mode
	_delay_ms(10);
	LCD_Transmit_Command(0x01); // Display Clear
	_delay_ms(10);

}

void Timer1_Init(void)
{
    TCCR1A = 0x00;
    TCCR1B = 0x13; //CTC Mode , Prescale = 1024
                   // 1 Tick당 0.069444[ms]
    OCR1A = 0x64;  // 100 Tick : 10[ms]
}

void ISR_Init(void)
{
    TIMSK = 0x04; //Timer 1 OverFlow Interrupt Enable
    SREG = 0x80; //Global Interrupt Enable
}

void Key_Scan(void)
{
    
}

void Switch_Scan(void)
{
    uint8_t PD0_status,PD1_status,PD2_status,PD3_status;
    static uint8_t PD0_status_old,PD1_status_old,PD2_status_old,PD3_status_old;

    PD0_status = PIND & 0x01;
    PD1_status = PIND & 0x02;
    PD2_status = PIND & 0x04;
    PD3_status = PIND & 0x08;

    if((PD0_status == 1)&&(PD0_status_old == 0))
    {
        Coin_100++;
    }
    if((PD1_status == 1)&&(PD1_status_old == 0))
    {
        Coin_500++;
    }    
    if((PD2_status == 1)&&(PD2_status_old == 0))
    {
        Coin_1000++;
    }    
    if((PD3_status == 1)&&(PD3_status_old == 0))
    {
        Coin_5000++;
    }

    Coin_Total = Coin_100 * 100 + Coin_500 * 500 + Coin_1000 * 1000 + Coin_5000 * 5000;

    Coin_n1 = Coin_Total % 10; //첫번째 자리 취득
    Coin_n10 = (Coin_Total / 10) % 10; //두번째 자리 취득
    Coin_n100 = (Coin_Total / 100) % 10; //세번째 자리 취득
    Coin_n1000 = (Coin_Total / 1000) % 10; //네번째 자리 취득

    PD0_status_old = PD0_status;
    PD1_status_old = PD1_status;
    PD2_status_old = PD2_status;
    PD3_status_old = PD3_status;
}

void Vending_Machine_Mode(void)
{
    static uint16_t Coin_Total_Old;
    switch(Vending_Machine_status)
    {
        case VENDING_MACHINE_IDLE :
            if(Coin_Total > 0)
            {
                Vending_Machine_status = VENDING_MACHINE_INSERT_COIN; //코인 입력이 들어왔을 경우 Insert Coin 모드로 변경
            }
        break;
        case VENDING_MACHINE_INSERT_COIN :
            if(Keypad_Num == 'A')
            {
                Vending_Machine_status = VENDING_MACHINE_SELECT_ITEM; //'A' 키패드 버튼이 들어왔을 경우, Select Item 모드로 변경
            }
            else if(Keypad_Num == 'D')
            {
                Vending_Machine_status = VENDING_MACHINE_CANCEL; //'D' 키패드 버튼이 입력될 경우, Cancel 모드로 변경
            }
        break;
        case VENDING_MACHINE_ITEM_SELECTED :
            if(Coin_Total < Vending_Machine_Price[Selected_Num])
            {
                Vending_Machine_status = VENDING_MACHINE_MONEY_ERROR;
            }
            if(Coin_Total == Vending_Machine_Price[Selected_Num])
            {
                Vending_Machine_status = VENDING_MACHINE_MONEY_FINISH;
            }
            // Coin input 인식했을때 코인 insert 모드로 가야함
        break;
        case VENDING_MACHINE_SELECT_ITEM : 
        break;
        case VENDING_MACHINE_MONEY_ERROR :
        break;
        case VENDING_MACHINE_MONEY_FINISH :
        break;
        case VENDING_MACHINE_CANCEL :
        break;
    }
}
void LCD_Print(void)
{
    switch(Vending_Machine_status)
    {
        case VENDING_MACHINE_IDLE :
            LCD_Cursor(0,0);
            sprintf(&LCD_Tx_Data[0],"vending machine");
            for(uint8_t i = 0;i<16;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }
            LCD_Cursor(1,0);
            sprintf(&LCD_Tx_Data[16],"-OPEN-");
            for(uint8_t i = 16;i<32;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }            
        break;
        case VENDING_MACHINE_INSERT_COIN :
            LCD_Cursor(0,0);
            sprintf(&LCD_Tx_Data[0]," insert money");
            for(uint8_t i = 0;i<16;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }
            LCD_Cursor(1,0);
            LCD_Tx_Data[17] = Coin_n1000;
            LCD_Tx_Data[18] = Coin_n100;
            LCD_Tx_Data[19] = Coin_n10;
            LCD_Tx_Data[20] = Coin_n1;
            sprintf(&LCD_Tx_Data[22],"won");
            for(uint8_t i = 16;i<32;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }   
        break;
        case VENDING_MACHINE_SELECT_ITEM :
            LCD_Cursor(0,0);
            sprintf(&LCD_Tx_Data[0],"selected goods");
            LCD_Tx_Data[13] = 0x3E;
            LCD_Tx_Data[14] = Keypad_Num;
            for(uint8_t i = 0;i<16;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }
            LCD_Cursor(1,0);
            sprintf(&LCD_Tx_Data[16],"buyit-A cancel-D");
            for(uint8_t i = 16;i<32;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }   
        break;
    }

}

void LCD_Transmit_Command(char cmd)
{
	cbi(LCD_CON, LCD_RS); // 0번 비트 클리어, RS = 0, 명령
	cbi(LCD_CON, LCD_RW); // 1번 비트 클리어, RW = 0, 쓰기
	sbi(LCD_CON, LCD_E);  // 2번 비트 설정, E = 1
	_delay_us(1);
	PORTA = cmd;          // 명령 출력
	cbi(LCD_CON, LCD_E);  // 명령 쓰기 동작 끝
	_delay_us(1);
}

void LCD_Cursor(char col, char row)
{
	LCD_Transmit_Command(0x80 | (row + col * 0x40));
}

void LCD_Transmit_Data(char data)
{
	sbi(LCD_CON, LCD_RS);
	cbi(LCD_CON, LCD_RW);
	sbi(LCD_CON, LCD_E);
	_delay_us(1);
	LCD_DATA = data;
	cbi(LCD_CON, LCD_E);
	_delay_us(1);
}

void Seven_Segment_Output(void)
{
    static uint8_t counter;

    counter++;
    if(counter>=4)
    {
        counter = 0;
    }
    
    switch(Vending_Machine_status)
    {
        case VENDING_MACHINE_IDLE :
            PORTE = 0xF0; // Seven Segment 선택 FND0~3
            PORTB = Seven_Segment_Num[10]; //- - - - 출력
        break;
        case VENDING_MACHINE_INSERT_COIN :
            switch(counter)
            {
                case SEVEN_SEGMENT_0 :
                    PORTE = 0x10; //FND0 선택
                    PORTB = Seven_Segment_Num[Coin_n1]; //첫번째 자리 출력
                break;
                case SEVEN_SEGMENT_1 :
                    PORTE = 0x20; //FND1 선택
                    PORTB = Seven_Segment_Num[Coin_n10]; //두번째 자리 출력
                break;
                case SEVEN_SEGMENT_2 :
                    PORTE = 0x40; //FND2 선택
                    PORTB = Seven_Segment_Num[Coin_n100]; //세번째 자리 출력
                break;
                case SEVEN_SEGMENT_3 :
                    PORTE = 0x80; //FND3 선택
                    PORTB = Seven_Segment_Num[Coin_n1000]; //네번째 자리 출력
                break;
                default :
                break;
            }
        break;
        default :
        break;
    }
}

void LED_Output(void)
{
    for(uint8_t i = 0;i<8;i++)
    {
        if(Vending_Machine_Count[i]>0)
        {
            sbi(PORTF,i);
        }
        else
        {
            cbi(PORTF,i);
        }
    }
}

ISR(TIMER1_OVF_Vect)
{
    Timer_10ms_flag = 1; //10ms 마다 flag set
    Timer_10ms_counter++;
    if(Timer_10ms_counter>=10)
    {
        Timer_10ms_counter = 0;
        Timer_100ms_flag = 1; // 100ms 마다 flag set
    }
}