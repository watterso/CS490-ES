#include <stdio.h>
#include "msp430f5438.h"
#define LED1 0x01
#define LED2 0x02
#define SW1 0x40
#define SW2 0x80
#define SW1_MASK 0x01
#define SW2_MASK 0x02
#define ON 1

//??1kHz?? frequency makes these
#define DBC_THRESH 200
#define SEC_1 500		//on half sec, off half sec, period = 1
#define SEC_2 1000
#define SEC_3 1500
#define SEC_4 2000
#define SEC_5 2500
#define SEC_6 3000
#define SEC_7 3500
#define SEC_8 4000
#define SEC_9 4500		// on 4.5 sec, off 4.5 sec, period = 9
#define NR_FREQ 9
#define NR_FREQ_0 NR_FREQ-1		//# of freqs, 0 index

int sw1_mask_count = 0;
int sw2_mask_count = 0;
const int sw_mask_limit = DBC_THRESH;

const int freqs[] = {SEC_1, SEC_2, SEC_3, SEC_4, SEC_5, SEC_6,
		SEC_7, SEC_8, SEC_9};
unsigned int freq_sel = 4;	//0 index! start at SEC_5

int led1_count = 0;
int led1_limit;
int led2_count = 0;
int led2_limit;


void main(void)
{
	WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer

	P1DIR |= LED1; //P1.0 as output
	P1DS |= LED1; //Enable Full drive strength
	P1DIR |= LED2; //P1.6 as output
	P1DS |= LED2; //Enable Full drive strength

	//clear P1OUT
	P1OUT &= ~LED1;
	P1OUT &= ~LED2;

	TA1CCTL0 = CCIE;
	TA1CTL = TASSEL_1 + MC_1 + TACLR;         // ACLK, upmode, clear TAR
	TA1CTL |= ID_3;
	TA1EX0 = TAIDEX_1;
	//ID_3 divide by 8, TAIDEX_3 divide by 4, 32k/32 = 1k
	// CORRECTION: Empirical testing says this is actually a .5kHz?
	//
	TA1CCR0 = 1;


	//Configure switch S1
	P2REN |= SW1; //Enable pull resistor
	P2OUT |= SW1; //Enable Pull-Up resistor
	P2IES |= SW1; //Interrupts on high to low transition
	P2IE |= SW1; //Enable Interrupts
	P2IFG &= ~SW1; //clear IFG


	//Configure switch S2
	P2REN |= SW2; //Enable pull resistor
	P2OUT |= SW2; //Enable Pull-Up resistor
	P2IES |= SW2; //Interrupts on high to low transition
	P2IE |= SW2; //Enable Interrupts
	P2IFG &= ~SW2; //clear IFG

	led1_limit = freqs[freq_sel];
	led2_limit = freqs[NR_FREQ_0 - freq_sel];

	__bis_SR_register(GIE);       //enable interrupts
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
{
	sw1_mask_count++;
	sw2_mask_count++;
	led1_count++;
	led2_count++;

	if(sw1_mask_count >= sw_mask_limit){
		P2IFG &= ~SW1; //clear IFG
		P2IE |= SW1; //Enable Interrupts
		sw1_mask_count = 0;
	}
	if(sw2_mask_count >= sw_mask_limit){
		P2IFG &= ~SW2; //clear IFG
		P2IE |= SW2; //Enable Interrupts
		sw2_mask_count = 0;
	}
	if(led1_count >= led1_limit){
		P1OUT ^= LED1;
		led1_count = 0;
	}
	if(led2_count >= led2_limit){
		P1OUT ^= LED2;
		led2_count = 0;
	}
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void){
	if(P2IFG & SW1 && P2IE & SW1){
		sw1_mask_count = 0;
		P2IE &= ~SW1; //disable Interrupts
		if(freq_sel > 0){
			freq_sel--;
			led1_limit = freqs[freq_sel];
			led2_limit = freqs[NR_FREQ_0 - freq_sel];
		}else{
			//clear P1OUT
			P1OUT &= ~LED1;
			P1OUT &= ~LED2;
			//stop timer
			TA1CCR0 = 0;
		}
		P2IFG &= ~SW1; //interrupt handled
	}
	if(P2IFG & SW2 && P2IE & SW2){
		sw2_mask_count = 0;
		P2IE &= ~SW2; //disable Interrupts
		if(freq_sel < NR_FREQ_0){
			freq_sel++;
			led1_limit = freqs[freq_sel];
			led2_limit = freqs[NR_FREQ_0 - freq_sel];
		}else{
			//clear P1OUT
			P1OUT &= ~LED1;
			P1OUT &= ~LED2;
			//stop timer
			TA1CCR0 = 0;
		}
		P2IFG &= ~SW2; //interrupt handled
	}
}
