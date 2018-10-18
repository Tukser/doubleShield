#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "main.h"
#include <stdlib.h>
#include <string.h>

float adc_value=0, adc_voltage=0, adc_current=0;

unsigned flags=flag_ADC0;


char voltage_str_send[10];
int main(void)
{

	initializzation_pin();
	initializzation_uart(103);
	initialization_adc();
	LCD_ini();
	clearlcd();
	sei();
    while(1)
		{
			send_information();
		}
}

void initializzation_pin(void)
{
	//Display initialization
	DDRD = 0xFF;
	PORTD = 0x00;
	DDRB |= (1<<PB1);
	PORTB &= ~(1<<PB1);
}

void sendhalfbyte(unsigned char c)
{
	c<<=3;
	e1;
	_delay_us(50);
	PORTD&=0b10000111;
	PORTD|=c;
	e0;
	_delay_us(50);
}
//----------------------------------------
void sendbyte(unsigned char c, unsigned char mode)
{
	if (mode==0) rs0;
	else         rs1;
	unsigned char hc=0;
	hc=c>>4;
	sendhalfbyte(hc); sendhalfbyte(c);
}
//----------------------------------------
void sendcharlcd(unsigned int c)
{
	sendbyte(c,1);
}
//----------------------------------------
void setpos(unsigned char x, unsigned y)
{
	char adress;
	adress=(0x40*y+x)|0b10000000;
	sendbyte(adress, 0);
}
//----------------------------------------
void LCD_ini(void)
{
	_delay_ms(15); //���� 15 �� (��� 45)
	sendhalfbyte(0b00000011);
	_delay_ms(4);
	sendhalfbyte(0b00000011);
	_delay_us(100);
	sendhalfbyte(0b00000011);
	_delay_ms(1);
	sendhalfbyte(0b00000010);
	_delay_ms(1);
	sendbyte(0b00101000, 0); //4���-����� (DL=0) � 2 ����� (N=1)
	_delay_ms(1);
	sendbyte(0b00001100, 0); //�������� ����������� �� ������� (D=1), ������� ������� �� �������� (C=0, B=0)
	_delay_ms(1);
	sendbyte(0b00000110, 0); //������ (���� �� � ��� � ���������) ����� ��������� �����
	_delay_ms(1);
}
//----------------------------------------
void clearlcd(void)
{
	sendbyte(0b00000001, 0);
	_delay_us(1500);
}
//----------------------------------------
void str_lcd (char str1[])
{
	uint8_t n;
	for(n=0;str1[n]!='\0';n++)
	sendcharlcd(str1[n]);
}
//----------------------------------------


void initializzation_uart(unsigned int ubrr)
{
	UBRR0H = (unsigned char) (ubrr>>8);
	UBRR0L = (unsigned char) (ubrr);
	UCSR0B |= (1<<TXEN0);
	UCSR0C = (1<<USBS0) | (1<<UCSZ00) | (1<<UCSZ01);
}

void send_uart(char char_buffer)
{
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = char_buffer;
}

void send_str_uart(char *str_buffer)
{
	while (*str_buffer != '\0')
	{
		send_uart(*str_buffer);
		str_buffer++;
	}
}


ISR(ADC_vect)
{
	if (flags==0)
		{
			adc_voltage=ADCW;
			//adc_current=ADCW;
			ADCSRA &= ~(1<<ADSC);
			ADMUX &= ~(1<<MUX0);
			flags=1;
		}
		else
		{

			adc_current=ADCW;
			//adc_voltage=ADCW;
			ADCSRA &= ~(1<<ADSC);
			ADMUX |= (1<<MUX0);
			flags=0;
		}
		ADCSRA |= (1<<ADSC);
}
//adc_voltage=ADCW;
//adc_current=ADCW;
//}
void initialization_adc()
{
	//ADC Initialization
	ADMUX |= (0<<REFS1) | (0<<REFS0);
	ADCSRA |= (1<<ADEN) | (1<<ADIE)  | (1<<ADATE) | (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2);
	ADCSRA |= (1<<ADSC);
	ADMUX |= (1<<MUX0);
}

void send_information()
{
  float voltage_output=0, current_output=0;
  float voltage=0, current=0, power_output=0;
  float voltage_array[100];
  float current_array[100];
	float aref=4.75/1023;
  int count;
  for (count=0; count<100; count++)
    {
      voltage_array[count] = adc_voltage;
      voltage = voltage + voltage_array[count];
    }

  for (count=0; count<100; count++)
    {
      current_array[count] = adc_current;
      current = current + current_array[count];
    }

  voltage = (voltage/count);
  current = (current/count);

	voltage_output= (voltage*aref)*10;
	current_output= ((current)*0.165);
	power_output=voltage_output*current_output;
	if (current_output!=0)
		current_output++;

	if (voltage_output<=12 && voltage_output!=0)
		{
			sound_signal();
			setpos(0,0);
			sendcharlcd(0x9F);
			setpos(0, 1);
			sendcharlcd(0xED);
			//_delay_ms(500);
		}
		else
			{

				setpos(0,0);
				sendcharlcd(0x9B);
				setpos(0, 1);
				sendcharlcd(0x8E);
			}

			setpos(2, 0);
			sendcharlcd(((unsigned char) voltage_output%100)/10 + 0x30);
			sendcharlcd((unsigned char) voltage_output%10 + 0x30);
			sendcharlcd('.');
			sendcharlcd(((unsigned char) (voltage_output*10))%10 + 0x30);
			sendcharlcd(((unsigned char) (voltage_output*100))%10 + 0x30);
			sendcharlcd('V');

			setpos(10, 0);

			sendcharlcd(((unsigned char) current_output%100)/10 + 0x30);
			sendcharlcd((unsigned char) current_output%10 + 0x30);
			sendcharlcd('.');
			sendcharlcd(((unsigned char) (current_output*10))%10 + 0x30);
			sendcharlcd(((unsigned char) (current_output*100))%10 + 0x30);
			sendcharlcd('A');

			setpos(2, 1);
			sendcharlcd(((unsigned char) power_output%100)/10 + 0x30);
			sendcharlcd((unsigned char) power_output%10 + 0x30);
			sendcharlcd('.');
			sendcharlcd(((unsigned char) (power_output*10))%10 + 0x30);
			sendcharlcd(((unsigned char) (power_output*100))%10 + 0x30);
			sendcharlcd('W');
			_delay_ms(1500);

	clearlcd();
}

void sound_signal()
{
	uint8_t i;
	for(i=0; i<254; i++) {
			SOUND_ON;
			_delay_us(500);
			SOUND_OFF;
			_delay_us(500);
	}
	for(int i=0; i<254; i++) {
			SOUND_ON;
			_delay_ms(1);
		SOUND_OFF;
			_delay_ms(1);
	}

}
