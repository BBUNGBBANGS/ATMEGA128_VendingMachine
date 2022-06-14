#define F_CPU 14.7456E6
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

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

uint8_t Seven_Segment_Num[] =
{
    0xc0, //0
    0xf9, //1
    0xa4, //2
    0xb0, //3
    0x99, //4
    0x92, //5
    0x82, //6
    0xd8, //7
    0x80, //8
    0x90 //9
}; // 애노드 공통

uint8_t Vending_Machine_Count[8] = {3,3,3,3,3,3,3,3};

/* 함수 프로토 타입 설정 */
void Port_Init(void);
void LCD_Init(void);
void Key_Scan(void);

int main(void)
{
    Port_Init();
    LCD_Init();
    /* Replace with your application code */
    while (1) 
    {
        Key_Scan();
    }
}

void Port_Init(void)
{
    DDRE = 0xF0; // 7-세그먼트 선택 포트 출력 설정 (PE4 : FND0, PE5, FND1, PE6 : FND2, PE7 : FND3)
    DDRB = 0xFF; // 7-세그먼트 포트 출력 설정
    DDRC = 0x0F; // PC0~3 : Output , PC4~7 : Input 설정
    DDRA = 0xFF; // LCD Data line 출력 설정 PA0~PA7
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

void Key_Scan(void)
{

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
