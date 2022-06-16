#define F_CPU 14.7456E6
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
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
    0xBF // -
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

uint16_t Melody_value[7] =
{
    3517, //도
    3135, //레
    2792, //미
    2641, //파
    2490, //솔
    2094, //라
    1865 //시
};

uint8_t Vending_Machine_status;

uint8_t Timer_10ms_flag;
uint8_t Timer_10ms_counter;
uint8_t Timer_100ms_flag;

uint8_t Coin_100,Coin_500,Coin_1000,Coin_5000;
uint8_t Coin_n1,Coin_n10,Coin_n100,Coin_n1000;
uint16_t Coin_Total;
uint16_t Coin_Inserted;

uint8_t Keypad_Num = ' ';
uint8_t Selected_Num = 1;

uint8_t row,column;

uint8_t Selected_item_flag = 1;

uint8_t Melody_status;
uint16_t Melody_timer;

char LCD_Tx_Data[32];

/***********  Seven Segment 모듈 정보 ***********/
#define SEVEN_SEGMENT_0                 (0)
#define SEVEN_SEGMENT_1                 (1)
#define SEVEN_SEGMENT_2                 (2)
#define SEVEN_SEGMENT_3                 (3)
/************************************************/

/***************  자판기 모드 정보 ***************/
#define VENDING_MACHINE_IDLE            (0)
#define VENDING_MACHINE_INSERT_COIN     (1)
#define VENDING_MACHINE_SELECT_ITEM     (2)
#define VENDING_MACHINE_ITEM_SELECTED   (3)
#define VENDING_MACHINE_MONEY_ERROR     (4)
#define VENDING_MACHINE_MONEY_FINISH    (5)
#define VENDING_MACHINE_CANCEL          (6)
/************************************************/

/*********  멜로디 음계 출력 모드 정보 ***********/
#define MELODY_STATUS_START             (0)
#define MELODY_STATUS_DO                (1)
#define MELODY_STATUS_RE                (2)
#define MELODY_STATUS_MI                (3)
#define MELODY_STATUS_FA                (4)
#define MELODY_STATUS_SOL               (5)
#define MELODY_STATUS_LA                (6)
#define MELODY_STATUS_SI                (7)
#define MELODY_STATUS_FINISHED          (8)
/************************************************/

/*******  함수 프로토 타입 설정 *******/
void Port_Init(void);
void LCD_Init(void);
void Timer1_Init(void);
void Timer3_Init(void);
void ISR_Init(void);
void LCD_Print(void);
void Key_Scan(void);
void LED_Output(void);
void Switch_Scan(void);
void Vending_Machine_Mode(void);
void Seven_Segment_Output(void);
void Clear_Tx_Buffer(void);
void LCD_Transmit_Command(char cmd);
void LCD_Cursor(char col, char row);
void LCD_Transmit_Data(char data);
void Melody_Update(void);
void Send_String(char * buf,uint16_t length);
void Send_Char(char data);
void Uart1_Init(void);
/*************************************/

int main(void)
{
    Port_Init();
    LCD_Init();
    Timer1_Init();
    Timer3_Init();
    Uart1_Init();
    ISR_Init();
    while (1) 
    {
        if(Timer_10ms_flag == 1)
        {
            Timer_10ms_flag = 0;
            Key_Scan();
            Switch_Scan();
            Vending_Machine_Mode();
            Melody_Update();
            Seven_Segment_Output();
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
    DDRG = 0x17; // LCD Command line 출력 설정 PG0~PG2,PG4
    DDRD = 0x00;
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
    TCCR1B = 0x0D; //CTC Mode , Prescale = 1024
                   // 1 Tick당 0.069444[ms]
    OCR1A = 144;  // 144 Tick : 10[ms]
}

void Timer3_Init(void)
{
    TCCR3A = 0x00;
    TCCR3B = 0x0A; //CTC Mode , Prescale = 8
                   // 1 Tick당 0.54253472[us]
    OCR3A = 3517;  // 3517 Tick : 1908[us] -도-
}

void ISR_Init(void)
{
    TIMSK = 0x10; //Timer1 Output Compare A Match Interrupt Enable
    ETIMSK = 0x10; // Timer3 Output Compare A Match Interrupt Enable
    SREG = 0x80; //Global Interrupt Enable
}

void Key_Scan(void)
{   
    row++;
    if(row>=4)
    {
        row = 0;
    }

    switch(row)
    {
        case 0 :
            PORTC = 0xFE;
        break;
        case 1 :
            PORTC = 0xFD;
        break;
        case 2 :
            PORTC = 0xFB;
        break;
        case 3 :
            PORTC = 0xF7;
        break;
        default : 
        break;
    }
    _delay_us(500);

    column = (PINC & 0xF0)>>4;

    if((row == 0) && (column != 0))
    {
        if(column == 14)
        {
            Keypad_Num = '1';
        }
        else if(column == 13)
        {
            Keypad_Num = '2';
        }
        else if(column == 11)
        {
            Keypad_Num = '3';
        }
        else if(column == 7)
        {
            Keypad_Num = 'A';
        }
    }
    else if((row == 1) && (column != 0))
    {
        if(column == 14)
        {
            Keypad_Num = '4';
        }
        else if(column == 13)
        {
            Keypad_Num = '5';
        }
        else if(column == 11)
        {
            Keypad_Num = '6';           
        }
        else if(column == 7)
        {
            Keypad_Num = 'B';
        }
    }
    else if((row == 2) && (column != 0))
    {
        if(column == 14)
        {
            Keypad_Num = '7';
        }
        else if(column == 13)
        {
            Keypad_Num = '8';
        }
        else if(column == 11)
        {
            Keypad_Num = '9';           
        }
        else if(column == 7)
        {
            Keypad_Num = 'C';
        }
    }
    else if((row == 3) && (column != 0))
    {
        if(column == 14)
        {
            Keypad_Num = '*';
        }
        else if(column == 13)
        {
            Keypad_Num = '0';
        }
        else if(column == 11)
        {
            Keypad_Num = '#';           
        }
        else if(column == 7)
        {
            Keypad_Num = 'D';
        }
    }
}

void Switch_Scan(void)
{
    uint8_t PD0_status,PD1_status,PD2_status,PD3_status;
    static uint8_t PD0_status_old,PD1_status_old,PD2_status_old,PD3_status_old;

    PD0_status = PIND & 0x01;
    PD1_status = PIND & 0x02;
    PD2_status = 0;//PIND & 0x04;
    PD3_status = 0;//PIND & 0x08;

    if((PD0_status == 0x01)&&(PD0_status_old == 0))
    {
        Coin_100++;
    }
    if((PD1_status == 0x02)&&(PD1_status_old == 0))
    {
        Coin_500++;
    }    
    if((PD2_status == 0x04)&&(PD2_status_old == 0))
    {
        Coin_1000++;
    }    
    if((PD3_status == 0x08)&&(PD3_status_old == 0))
    {
        Coin_5000++;
    }

    Coin_Total = Coin_100 * 100 + Coin_500 * 500 + Coin_1000 * 1000 + Coin_5000 * 5000;

    if((Vending_Machine_status == VENDING_MACHINE_ITEM_SELECTED)||(Vending_Machine_status == VENDING_MACHINE_MONEY_FINISH))
    {
        Coin_n1 = Coin_Inserted % 10;//첫번째 자리 취득 
        Coin_n10 = (Coin_Inserted / 10) % 10; //두번째 자리 취득 
        Coin_n100 = (Coin_Inserted / 100) % 10; //세번째 자리 취득 
        Coin_n1000 = (Coin_Inserted / 1000) % 10; //네번째 자리 취득 
    }
    else
    {
        Coin_n1 = Coin_Total % 10;//첫번째 자리 취득 
        Coin_n10 = (Coin_Total / 10) % 10; //두번째 자리 취득 
        Coin_n100 = (Coin_Total / 100) % 10; //세번째 자리 취득 
        Coin_n1000 = (Coin_Total / 1000) % 10; //네번째 자리 취득 
    }

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
            if((Keypad_Num > '0')&&(Keypad_Num < '9'))
            {
                Selected_Num = Keypad_Num - 0x30; // ASCII -> 숫자로 변환
                Vending_Machine_status = VENDING_MACHINE_SELECT_ITEM; //'0~8' 키패드 버튼이 들어왔을 경우, Select Item 모드로 변경
                Coin_Inserted = Coin_Total;
            }
        break;
        case VENDING_MACHINE_SELECT_ITEM : 
            if(Keypad_Num == '9')//'A')
            {
                Vending_Machine_status = VENDING_MACHINE_ITEM_SELECTED; //'A' 키패드 버튼이 들어왔을 경우, Select Item 모드로 변경
            }
            else if(Keypad_Num == '0')//'D')
            {
                Vending_Machine_status = VENDING_MACHINE_CANCEL; //'D' 키패드 버튼이 입력될 경우, Cancel 모드로 변경
            }
        break;
        case VENDING_MACHINE_ITEM_SELECTED :
            if((Coin_Inserted < Vending_Machine_Price[Selected_Num - 1])&&(Melody_status == MELODY_STATUS_FINISHED)&&(Selected_item_flag==1))
            {
                Vending_Machine_status = VENDING_MACHINE_MONEY_ERROR;
            }            
            else if(Keypad_Num == '0')//'D')
            {
                Vending_Machine_status = VENDING_MACHINE_CANCEL; //'D' 키패드 버튼이 입력될 경우, Cancel 모드로 변경
            }
            else
            {
                if(Selected_item_flag == 1)
                {
                    Coin_Inserted = Coin_Inserted - Vending_Machine_Price[Selected_Num - 1];
                    Selected_item_flag = 0;
                }

                if((Coin_Inserted == 0)&&(Melody_status == MELODY_STATUS_FINISHED))
                {
                    Vending_Machine_status = VENDING_MACHINE_MONEY_FINISH;
                }
            }
        break;
        case VENDING_MACHINE_MONEY_ERROR :
            if(Keypad_Num == '0')//'D')
            {
                Vending_Machine_status = VENDING_MACHINE_CANCEL; //'D' 키패드 버튼이 입력될 경우, Cancel 모드로 변경
            }
        break;
        case VENDING_MACHINE_MONEY_FINISH :
            if(Melody_status == MELODY_STATUS_FINISHED)
            {
                Vending_Machine_status = VENDING_MACHINE_IDLE;
                Melody_status = MELODY_STATUS_START;
                Selected_item_flag = 1;
                Coin_Total = 0;
                Coin_100 = 0;
                Coin_500 = 0;
                Coin_1000 = 0; 
                Coin_5000 = 0;;
            }
        break;
        case VENDING_MACHINE_CANCEL :
            if(Melody_status == MELODY_STATUS_FINISHED)
            {
                Vending_Machine_status = VENDING_MACHINE_IDLE;
                Melody_status = MELODY_STATUS_START;
                Selected_item_flag = 1;
                Coin_Total = 0;
                Coin_100 = 0;
                Coin_500 = 0;
                Coin_1000 = 0; 
                Coin_5000 = 0;;
            }
        break;
    }
}
void LCD_Print(void)
{
    static uint8_t open_status,shift_counter;

    if(open_status == 0)
    {
        shift_counter++;
        if(shift_counter>= 10)
        {
            open_status = 1;
        }
    }
    else
    {
        shift_counter--;
        if(shift_counter == 0)
        {
            open_status = 0;
        }
    }

    switch(Vending_Machine_status)
    {
        case VENDING_MACHINE_IDLE :
            Clear_Tx_Buffer();
            LCD_Cursor(0,0);
            sprintf(&LCD_Tx_Data[0],"vending machine");
            LCD_Tx_Data[15] = '\n';
            Send_String(&LCD_Tx_Data[0],16);
            
            for(uint8_t i = 0;i<16;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }
            LCD_Cursor(1,0);
            LCD_Tx_Data[16+shift_counter] = '-';
            LCD_Tx_Data[17+shift_counter] = 'O';
            LCD_Tx_Data[18+shift_counter] = 'P';
            LCD_Tx_Data[19+shift_counter] = 'E';
            LCD_Tx_Data[20+shift_counter] = 'N';
            LCD_Tx_Data[21+shift_counter] = '-';
            LCD_Tx_Data[31] = '\n';
            Send_String(&LCD_Tx_Data[16],16);
            for(uint8_t i = 16;i<32;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }            
        break;
        case VENDING_MACHINE_INSERT_COIN :
            Clear_Tx_Buffer();
            LCD_Cursor(0,0);
            sprintf(&LCD_Tx_Data[0]," insert money");
            LCD_Tx_Data[15] = '\n';
            Send_String(&LCD_Tx_Data[0],16);
            for(uint8_t i = 0;i<16;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }
            LCD_Cursor(1,0);
            LCD_Tx_Data[17] = Coin_n1000 + 0x30; // 숫자 ASCII 변환
            LCD_Tx_Data[18] = Coin_n100 + 0x30; // 숫자 ASCII 변환
            LCD_Tx_Data[19] = Coin_n10 + 0x30; // 숫자 ASCII 변환
            LCD_Tx_Data[20] = Coin_n1 + 0x30; // 숫자 ASCII 변환
            sprintf(&LCD_Tx_Data[22],"won");
            LCD_Tx_Data[31] = '\n';
            Send_String(&LCD_Tx_Data[16],16);
            for(uint8_t i = 16;i<32;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }   
        break;
        case VENDING_MACHINE_SELECT_ITEM :
            Clear_Tx_Buffer();
            LCD_Cursor(0,0);
            sprintf(&LCD_Tx_Data[0],"selected goods");
            LCD_Tx_Data[13] = 0x3E;
            LCD_Tx_Data[14] = Selected_Num + 0x30; // 숫자 ASCII 변환
            LCD_Tx_Data[15] = '\n';
            Send_String(&LCD_Tx_Data[0],16);
            for(uint8_t i = 0;i<16;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }
            LCD_Cursor(1,0);
            sprintf(&LCD_Tx_Data[16],"buyit-A cancel-D");
            LCD_Tx_Data[31] = '\n';
            Send_String(&LCD_Tx_Data[16],16);
            for(uint8_t i = 16;i<32;i++)
            {
                LCD_Transmit_Data(LCD_Tx_Data[i]);
            }   
        break;
        case VENDING_MACHINE_MONEY_ERROR :
            Clear_Tx_Buffer();
            LCD_Cursor(0,0);        
            sprintf(&LCD_Tx_Data[0]," be short of");
            LCD_Tx_Data[15] = '\n';
            Send_String(&LCD_Tx_Data[0],16);            
            LCD_Cursor(1,0);        
            sprintf(&LCD_Tx_Data[16],"  money.......");
            LCD_Tx_Data[32] = '\n';
            Send_String(&LCD_Tx_Data[16],16);
        break;
    }

}

void Melody_Update(void)
{
    switch(Vending_Machine_status)
    {
        case VENDING_MACHINE_ITEM_SELECTED :
            switch(Melody_status)
            {
                case MELODY_STATUS_START : 
                    Melody_status = MELODY_STATUS_DO;
                    Melody_timer = 0;
                break;
                case MELODY_STATUS_DO:
                    OCR3A = Melody_value[MELODY_STATUS_DO-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_MI;
                    }
                break;
                case MELODY_STATUS_MI:
                    OCR3A = Melody_value[MELODY_STATUS_MI-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_SOL;
                    }
                break;                
                case MELODY_STATUS_SOL:
                    OCR3A = Melody_value[MELODY_STATUS_SOL-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_FINISHED;
                    }
                break;
                case MELODY_STATUS_FINISHED:
                    //Melody_status = MELODY_STATUS_START;
                    cbi(PORTG,4); // PG4 출력 차단
                break;
                default :
                    cbi(PORTG,4); // PG4 출력 차단
                break;
            }
        break;        
        case VENDING_MACHINE_MONEY_ERROR :
            switch(Melody_status)
            {
                case MELODY_STATUS_START : 
                    Melody_status = MELODY_STATUS_MI;
                    Melody_timer = 0;
                break;
                case MELODY_STATUS_MI:
                    OCR3A = Melody_value[MELODY_STATUS_MI-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_SI;
                    }
                break;                
                case MELODY_STATUS_SI:
                    OCR3A = Melody_value[MELODY_STATUS_SI-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_FINISHED;
                    }
                break;
                case MELODY_STATUS_FINISHED:
                    Melody_status = MELODY_STATUS_START;
                    cbi(PORTG,4); // PG4 출력 차단
                break;
                default :
                    cbi(PORTG,4); // PG4 출력 차단
                break;
            }
        break;        
        case VENDING_MACHINE_MONEY_FINISH :
            switch(Melody_status)
            {
                case MELODY_STATUS_START : 
                    Melody_status = MELODY_STATUS_SI;
                    Melody_timer = 0;
                break;
                case MELODY_STATUS_SI:
                    OCR3A = Melody_value[MELODY_STATUS_SI-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_LA;
                    }
                break;                
                case MELODY_STATUS_LA:
                    OCR3A = Melody_value[MELODY_STATUS_LA-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_SOL;
                    }
                break;
                case MELODY_STATUS_SOL:
                    OCR3A = Melody_value[MELODY_STATUS_SOL-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_FA;
                    }
                break;
                case MELODY_STATUS_FA:
                    OCR3A = Melody_value[MELODY_STATUS_FA-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_MI;
                    }
                break;                
                case MELODY_STATUS_MI:
                    OCR3A = Melody_value[MELODY_STATUS_MI-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_RE;
                    }
                break;              
                case MELODY_STATUS_RE:
                    OCR3A = Melody_value[MELODY_STATUS_RE-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_DO;
                    }
                break;              
                case MELODY_STATUS_DO:
                    OCR3A = Melody_value[MELODY_STATUS_DO-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_FINISHED;
                    }
                break;
                case MELODY_STATUS_FINISHED:
                    Melody_status = MELODY_STATUS_START;
                    cbi(PORTG,4); // PG4 출력 차단
                break;
                default :
                    cbi(PORTG,4); // PG4 출력 차단
                break;
            }
        break;        
        case VENDING_MACHINE_CANCEL :
            switch(Melody_status)
            {
                case MELODY_STATUS_START : 
                    Melody_status = MELODY_STATUS_DO;
                    Melody_timer = 0;
                break;
                case MELODY_STATUS_DO:
                    OCR3A = Melody_value[MELODY_STATUS_DO-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_RE;
                    }
                break;                
                case MELODY_STATUS_RE:
                    OCR3A = Melody_value[MELODY_STATUS_RE-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_MI;
                    }
                break;
                case MELODY_STATUS_MI:
                    OCR3A = Melody_value[MELODY_STATUS_MI-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_FA;
                    }
                break;
                case MELODY_STATUS_FA:
                    OCR3A = Melody_value[MELODY_STATUS_FA-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_SOL;
                    }
                break;                
                case MELODY_STATUS_SOL:
                    OCR3A = Melody_value[MELODY_STATUS_SOL-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_LA;
                    }
                break;              
                case MELODY_STATUS_LA:
                    OCR3A = Melody_value[MELODY_STATUS_LA-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_SI;
                    }
                break;              
                case MELODY_STATUS_SI:
                    OCR3A = Melody_value[MELODY_STATUS_SI-1];
                    Melody_timer++;
                    if(Melody_timer>=50)
                    {
                        Melody_timer = 0;
                        Melody_status = MELODY_STATUS_FINISHED;
                    }
                break;
                case MELODY_STATUS_FINISHED:
                    Melody_status = MELODY_STATUS_START;
                    cbi(PORTG,4); // PG4 출력 차단
                break;
                default :
                    cbi(PORTG,4); // PG4 출력 차단
                break;
            }
        break;
        default :
            cbi(PORTG,4); // PG4 출력 차단
        break;
    }
}

void Clear_Tx_Buffer(void)
{
    for(uint8_t i = 0;i<32;i++)
    {
        LCD_Tx_Data[i] = ' ';
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
        case VENDING_MACHINE_SELECT_ITEM :
        case VENDING_MACHINE_ITEM_SELECTED :
        case VENDING_MACHINE_MONEY_FINISH :
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
            PORTB = 0xFF;
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

ISR(TIMER1_COMPA_vect)
{
    Timer_10ms_flag = 1; //10ms 마다 flag set
    Timer_10ms_counter++;
    if(Timer_10ms_counter>=10)
    {
        Timer_10ms_counter = 0;
        Timer_100ms_flag = 1; // 100ms 마다 flag set
    }
}

ISR(TIMER3_COMPA_vect)
{
    static uint8_t flag;
    if((Melody_status != MELODY_STATUS_START)&&(Melody_status != MELODY_STATUS_FINISHED))
    {
        if(flag == 0)
        {
            sbi(PORTG,4);
            flag = 1;
        }
        else
        {
            cbi(PORTG,4);
            flag = 0;
        }
    }
    else
    {
        cbi(PORTG,4);
        flag = 0;
    }

}

void Send_String(char * buf,uint16_t length)
{
	uint16_t i;
    for(i = 0;i<length;i++)
    {
        Send_Char(buf[i]);
    }
}

 void Send_Char(char data)
{
    while((UCSR1A & 0x20) == 0x0); 
    UDR1 = data;
}

void Uart1_Init(void)
{
    UCSR1A = 0x0;
    UCSR1B = 0b00011000; //Uart1 Rx/Tx Enable
    UCSR1C = 0b00000110; //Uart1 8bit character size
    UBRR1H = 0;                                                  
    UBRR1L = 8; //Uart1 115200 bps        
}
