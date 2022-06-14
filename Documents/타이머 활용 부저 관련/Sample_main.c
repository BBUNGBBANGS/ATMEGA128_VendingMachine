#include <mega128.h>
#include <delay.h>
#include <math.h>

#define CPU_F                    14745600
#define Buzzer_Timer_OCR_value   1 

#define FND_Null    17
#define FND_Star    18
#define FND_Sharp   19

#define Do   0
#define Re   1
#define Mi   2
#define Pa   3
#define Sol  4
#define Ra   5
#define Si   6

unsigned int Port_char[] ={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0x88,0x83,0xA7,0xA1,0x86,0x8E,0xFF,0xBF,0xB6,0xC9};  // 0~F �� ����ǥ
unsigned int Port_fnd[] ={0x1f,0x2f,0x4f,0x8f,0x0f};    // FND0 ON, FND1 ON, FND2 ON, FND3 ON, ALL FND OFF
float Buzzer_frequency[] ={32.7032,36.7081,41.2035,43.6535,48.9994,55.0000,61.7354};

unsigned int Buzzer_timer=0, Buzzer_Play_timer=0, Buzzer_timer_count_value=0;
bit Buzzer_Play_flag=0;

void init_reg(void)
{
    DDRE = 0xf0; //FND Sel
    DDRB = 0xff; //FND Data Line
    
    DDRC = 0x0f; // ���� 4bit Col(�Է�), ���� 4bit Row(���)
    DDRG = 0x10; // Buzzer out Pin 
    
    PORTC = 0x0f; //Port �ʱ�ȭ       
    PORTE = Port_fnd[4]; //ALL FND OFF   
}

void Timer_init(void)
{
    TIMSK = 0x02;
    TCCR0 = 0x0B;
    OCR0 = Buzzer_Timer_OCR_value; // �� 0.00002S
    TCNT0 = 0;
    SREG |= 0x80;  
}

void Buzzer_Play_decoder(unsigned char octave,unsigned char scale)
{
    //Buzzer_Freq. to Timer(4.34us) Count_value 
    //Count_value = Main_Freq/(2*N*(OCR+1)*Buzzer_Freq)
    
    float Buzzer_Freq = Buzzer_frequency[scale]*pow(2,(octave-1));          
    Buzzer_timer_count_value = (unsigned int)((float)CPU_F/(float)(2*32*(Buzzer_Timer_OCR_value+1)*Buzzer_Freq)); 
    
    Buzzer_Play_flag=1;
}

void Print_Segment(unsigned int* seg_value)
{

      PORTE = Port_fnd[0]; 
      PORTB = Port_char[seg_value[0]]; 
      delay_ms(1);          
    
      PORTE = Port_fnd[1]; 
      PORTB = Port_char[seg_value[1]];
      delay_ms(1);        
      
      PORTE  = Port_fnd[2]; 
      PORTB =  Port_char[seg_value[2]];
      delay_ms(1);        
                
      PORTE = Port_fnd [3]; 
      PORTB = Port_char[seg_value[3]];  
      delay_ms(1);                                     
}    

unsigned char KeyScan(void)                     // 4X4 Ű�е� ��ĵ �Լ�, ��� ���� 10���� 1~16  
{    
                                                 
      unsigned int Key_Scan_Line_Sel = 0xf7;    // Init_data ���� �Ϻ��� ����� ����  
                                                // ���� �Ϻ�(4bit)�� ����Ī(���������� ���ư��鼭)�ϸ鼭 ���    
      unsigned char Key_Scan_sel=0, key_num=0;         
      unsigned char Get_Key_Data=0;             // ���� Ű ������        
      
      //Ű��ĵ �κ�  
      for(Key_Scan_sel=0; Key_Scan_sel<4; Key_Scan_sel++)     
      {           

            // �ʱ�ȭ 
            PORTC = Key_Scan_Line_Sel;               
            delay_us(10);                                
            
            //���� �κ�
            Get_Key_Data = (PINC & 0xf0);   // 74LS14�� ������ ���      
            
            if(Get_Key_Data != 0x00)
            {                  
                  switch(Get_Key_Data)      // C��Ʈ ������ ���� �Ϻ�(4bit)�� ����            
                  {
                        case 0x10:  // 0001�� ������ ���� count���� 4�� ������                                
                                    //  1�� ���ϰ� key_num������ ����                
                              key_num = Key_Scan_sel*4 + 1;                
                              break;                
                        case 0x20:  // 0010�� ������ ���� count���� 4�� ������
                                    //  2�� ���ϰ� key_num������ ����                
                              key_num = Key_Scan_sel*4 + 2;                
                              break;                
                        case 0x40:  // 0100�� ������ ���� count���� 4�� ������ 
                                    //  3�� ���ϰ� key_num������ ����               
                              key_num = Key_Scan_sel*4 + 3;                
                              break;                
                        case 0x80:  // 1000�� ������ ���� count���� 4�� ������                                
                                    //  4�� ���ϰ� key_num������ ����                
                              key_num = Key_Scan_sel*4 + 4;                 
                              break;
                        default :
                              key_num = FND_Null;                 
                  }           
                  return key_num;       
            }               
            
            Key_Scan_Line_Sel = (Key_Scan_Line_Sel>>1); //Init_data�� ����Ʈ ��, Key_Scan_Line �̵�     
      } 
      
      return key_num;
}     
                                  
void main(void)                        
{                                   
                                
    bit Key_off_flag=0;
    unsigned char New_key_data=0, key_Num=0, octave_value=4;                          
    unsigned int buf_seg[4] = {FND_Null,FND_Null,FND_Null,FND_Null}; 
               
    PORTG = 0x10;
    
    init_reg();
    Timer_init(); 
               
    while(1)                 
    {                  

       New_key_data = KeyScan();

       if(New_key_data)
       {
           if(New_key_data%4 != 0)
           {
                key_Num = (New_key_data/4)*3+(New_key_data%4);
                
                if(key_Num >= 10)
                {
                    switch(key_Num)
                    {
                      case 10 :
                            key_Num = FND_Star;
                            break;
                      case 11 :
                            key_Num = 0;
                            break; 
                      case 12 :
                            key_Num = FND_Sharp;
                      default :
                            break;
                      }
                }
                else;     
            }
            else
                key_Num = (New_key_data/4)+9;
                
           if(Key_off_flag)
           {
                buf_seg[3] = buf_seg[2];
                buf_seg[2] = buf_seg[1];
                buf_seg[1] = buf_seg[0];  
                
                if(key_Num == FND_Star)
                    octave_value++;
                else if(key_Num == FND_Sharp)
                    octave_value--;
                else;
                                                
                Key_off_flag = ~Key_off_flag;
           }
           else;
             
           buf_seg[0] = key_Num;
           Buzzer_Play_decoder(octave_value,key_Num);      
        }
        else
            Key_off_flag=1;
                
        Print_Segment(buf_seg);   

//--------New Code Record---------------------------------

        Switch(key_Num) // buf_seg[0]
        {
	case 10 :  //set function, buf_seg[0]=buf_seg[1]=buf_seg[2]=buf_seg[3]=0; Print_Segment(buf_seg);
	break;
	case 11 :  //set function, if -> input_value=buf_seg[3] * 100 + buf_seg[2] * 10 + buf_seg[1]; //0~180 degree, set PWM value : input_value * PWM value of one degree + PWM value of 0 degree value
	break;
	case 12 : //set function 
	break;
	case 13 : //set function 
	break;
	default : 
	break;

        } 

   }
}

interrupt[TIM0_COMP] void timer_comp0(void)
{

    if(Buzzer_timer > Buzzer_timer_count_value)
    {
        Buzzer_timer=0;
        PORTG = ~PORTG;
    }
    else
        Buzzer_timer++;  
        
    if(Buzzer_Play_timer > 57600) // 0.25S 
    {
        Buzzer_timer_count_value=0;
        Buzzer_Play_timer=0;
        Buzzer_Play_flag=0;
    }
    else if(Buzzer_Play_flag==1)
        Buzzer_Play_timer++;  
    else; 
        
        
        
}


