#define F_CPU 14.7456E6
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

unsigned char zero_flag = 0;
unsigned int sec_count = 0;
unsigned char Port_char[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xd8, 0x80, 0x90};
unsigned int Port_fnd[] = {0x1f, 0x2f, 0x4f, 0x8f};

void port_Init()
{
	DDRC = 0x0f;
	DDRB = 0xff;
	DDRE = 0xf0;
}

void Init_Timer0()
{
	TCCR0 |= ((1<<CS02)|(1<<CS01)|(1<<CS00));
	TCNT0 = 0x70;
	TIMSK |= (1<<TOIE0);
}

ISR(TIMER0_OVF_vect)
{
	TCNT0 = 0x70;
	sec_count++;
}

unsigned char keyScan(void)
{
	unsigned char key_scan_line = 0xf7;
	unsigned char key_scan_loop = 0;
	unsigned char getpinData = 0;
	unsigned char key_num = 0;
	
	for(key_scan_loop=0; key_scan_loop<4; key_scan_loop++)
	{
		PORTC = key_scan_line;
		_delay_us(1);
		getpinData = PINC & 0xf0;
		
		if(getpinData != 0)
		{
			switch(getpinData)
			{
				case 0x10:
				key_num = (key_scan_loop*4) + 1;
				break;
				case 0x20:
				key_num = (key_scan_loop*4) + 2;
				break;
				case 0x40:
				key_num = (key_scan_loop*4) + 3;
				break;
				case 0x80:
				key_num = (key_scan_loop*4) + 4;
				break;
				default:
				break;
			}
			return key_num;
		}
		key_scan_line = (key_scan_line >> 1);
	}
	return 0;
}

unsigned char changeNum(unsigned char key_num)
{
	unsigned char return_num = 0;
	
	if(key_num % 4 == 0)
		return_num = 12 + key_num / 4;
	else if(key_num / 4 == 0)
		return_num = (4 * (key_num/4) + key_num % 4);
	else if(key_num / 4 == 1)
		return_num = (4 * (key_num/4) + key_num % 4) - 1;
	else if(key_num / 4 == 2)
		return_num = (4 * (key_num/4) + key_num % 4) - 2;
	else if(key_num / 4 == 3)
		return_num = (4 * (key_num/4) + key_num % 4) - 3;
	else;
	
	if(return_num == 11)
	{
		return_num = 0;
		zero_flag = 1;
	}
	
	return return_num;
}

void fnd_output(unsigned int final_num)
{
	int buffer = 0;
	unsigned char FND0 = 0, FND1 = 0, FND2 = 0, FND3 = 0;
	
	FND3 = final_num / 1000;
	buffer = final_num % 1000;
	FND2 = buffer / 100;
	buffer = buffer % 100;
	FND1 = buffer / 10;
	FND0 = buffer % 10;
	
	PORTE = Port_fnd[0];
	PORTB = Port_char[FND0];
	_delay_ms(10);
	PORTE = Port_fnd[1];
	PORTB = Port_char[FND1];
	_delay_ms(10);
	PORTE = Port_fnd[2];
	PORTB = Port_char[FND2];
	_delay_ms(10);
	PORTE = Port_fnd[3];
	PORTB = Port_char[FND3];
	_delay_ms(10);
}

void start_fnd()
{
	PORTE = Port_fnd[0];
	PORTB = 0xbf;
	_delay_ms(10);
	PORTE = Port_fnd[1];
	PORTB = 0xbf;
	_delay_ms(10);
	PORTE = Port_fnd[2];
	PORTB = 0xbf;
	_delay_ms(10);
	PORTE = Port_fnd[3];
	PORTB = 0xbf;
	_delay_ms(10);
}

void end_fnd()
{
	PORTE = Port_fnd[0];
	PORTB = 0xa1;
	_delay_ms(10);
	PORTE = Port_fnd[1];
	PORTB = 0xab;
	_delay_ms(10);
	PORTE = Port_fnd[2];
	PORTB = 0x86;
	_delay_ms(10);
	PORTE = Port_fnd[3];
	PORTB = 0xbf;
	_delay_ms(10);	
}
int main(void)
{
    int key_in_value = 0;
	unsigned char key_num = 0;
	int fnd[4] = {0,0,0,0};
	int start = 0, finish = 0;
	unsigned int timer_start = 0;	
	int final_num = 0;
	
	port_Init();
	Init_Timer0();
	sei();
	
    while (1) 
    {
		key_num = keyScan();
		key_in_value = changeNum(key_num);
		
		if(key_in_value == 13)
		{
			start = 1;
		}
		else if(key_in_value == 16)
		{
			timer_start = 1;
			start = 0;
			sec_count = 0;
		}
		else;
		
		if(start == 1)
		{
			if((key_in_value >= 0) && (key_in_value < 10))
			{
				fnd[3] = fnd[2];
				_delay_ms(100);
				fnd[2] = fnd[1];
				_delay_ms(100);
				fnd[1] = fnd[0];
				_delay_ms(100);
				fnd[0] = key_in_value;
			}				
			final_num = 1000*fnd[3] + 100*fnd[2] + 10*fnd[1] + fnd[0];
			fnd_output(final_num);
		}
		else if(timer_start == 1)
		{
			if(sec_count >= 99)
			{
				if(final_num > 0)
				{
					final_num--;
					sec_count = 0;
				}
				else
				{
					fnd[3] = 0;
					fnd[2] = 0;
					fnd[1] = 0;
					fnd[0] = 0;	
					
					timer_start = 0;				
					finish = 1;
				}
			}
			else;
			fnd_output(final_num);
		}
		else if(finish == 1)
		{
			end_fnd();
			
			if(sec_count >= 500)
				finish = 0;
		}
		else if(start == 0)
		{
			start_fnd();
		}
		else;
	}
}
