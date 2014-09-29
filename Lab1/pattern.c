#include "msp430f5438.h"

#define LED1 0x01
#define LED2 0x02
#define SW1 0x40
#define SW2 0x80
#define ON 1
#define OFF 0

int switch_pressed(int port);
int p2_is_on(int port);
void p1_set(int port, int on_or_off);

void main1()
{
	//Switch off the Watch-dog Timer
	WDTCTL = WDTPW + WDTHOLD;

	//Configure switch S1
	P2REN |= SW1; //Enable pull resistor
	P2OUT |= SW1; //Enable Pull-Up resistor

	//Configure switch S2
	P2REN |= SW2; //Enable pull resistor
	P2OUT |= SW2; //Enable Pull-Up resistor

	//Configure LED 1
	P1DIR |= LED1; //P1.0 as output
	P1DS |= LED1; //Enable Full drive strength

	//Configure LED 2
	P1DIR |= LED2; //P1.0 as output
	P1DS |= LED2; //Enable Full drive strength
	while(1){
		if(switch_pressed(SW1) && !switch_pressed(SW2)){
			p1_set(LED1, ON);
			p1_set(LED2, OFF);
		}else if(!switch_pressed(SW1) && switch_pressed(SW2)){
			p1_set(LED1, OFF);
			p1_set(LED2, ON);
		}else{
			p1_set(LED1, OFF);
			p1_set(LED2, OFF);
		}
	}
}
/*
 * Switch being pressed is actually a low signal
 */
int switch_pressed(int port){
	return !p2_is_on(port);
}
/*
 * Returns whether the provide port on pin 2 is in the on state
 */
int p2_is_on(int port){
	return P2IN & port;
}
/*
 * Sets the specified port on pin 1 to 1:ON 0:OFF
 */
void p1_set(int port, int on_or_off){
	if(on_or_off && !(P1OUT & port)){
		P1OUT |= port;
	}else if(!on_or_off && P1OUT & port){
		P1OUT ^= port;
	}
}
