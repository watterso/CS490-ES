#include "msp430f5438.h"
void main()
{
	//Switch off the Watch-dog Timer
	WDTCTL = WDTPW + WDTHOLD;

	//Configure switch S1
	P2REN |= 0x40; //Enable pull resistor
	P2OUT |= 0x40; //Enable Pull-Up resistor

	//Configure LED 1
	P1DIR |= 0x01; //P1.0 as output
	P1DS |= 0x01; //Enable Full drive strength
	while(1){
		//Main logic
		if(!(P2IN & 0x40)){ //If P2.6 is high
			P1OUT |= 0x01;
		}else if(P1OUT & 0x01)//If P2.6 is low
			P1OUT ^= 0x01;
	}
}
