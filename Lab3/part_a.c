#include <msp430.h> 

/*
 * main.c
 */

#define SW1 0x40
#define SW2 0x80
#define SW1_MASK 0x01
#define SW2_MASK 0x02
#define ON 1

//1kHz frequency makes these
#define DBC_THRESH 200
#define HZ_1 500		//on half sec, off half sec, period = 1
#define HZ_2 250		//on for quarter sec, off for quarter sec, period = .5

int sw1_mask_count = 0;
int sw2_mask_count = 0;
int global_count = 5;
const int sw_mask_limit = DBC_THRESH;
const char * RANGE_ERR = "OUT OF RANGE";
const int RANGE_ERR_SIZE = 12;

void send_char(char c);
void setup_db_buttons();
void setup_UART();
void print_count();

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	setup_UART();
	setup_db_buttons();
	print_count();

	__bis_SR_register(GIE);

	__no_operation();
}

void send_char(char c){
	while (!(UCA1IFG&UCTXIFG));
	UCA1TXBUF = c;
}

void send_strn(char * str, int n){
	int i = 0;
	while(i<n){
		send_char(*(str+i));
		i++;
	}
}

void print_count(){
	send_char(global_count + 48);
	send_char(';');
}
void setup_UART(){
	P5SEL |= BIT6 | BIT7;                      // P3 goes to LCD, P5 over the line
	UCA1CTL1 |= UCSWRST;                      // Hold reset to stop pause UART
	UCA1CTL1 |= UCSSEL_2;                     //
	UCA1BR0 = 9;                              // 1MHz 115200
	UCA1BR1 = 0;                              // 1MHz 115200
	UCA1MCTL |= UCBRS_1 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
	UCA1CTL1 &= ~UCSWRST;                     // Release UART
}

void setup_db_buttons(){
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
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
{
	sw1_mask_count++;
	sw2_mask_count++;

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
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void){
	if(P2IFG & SW1 && P2IE & SW1){
		sw1_mask_count = 0;
		P2IE &= ~SW1; //disable Interrupts
		if(global_count > 8 || global_count < 1){
			send_strn((char *)RANGE_ERR, RANGE_ERR_SIZE);
			send_char(';');
		}else{
			global_count--;
			print_count();
		}
		P2IFG &= ~SW1; //interrupt handled
	}
	if(P2IFG & SW2 && P2IE & SW2){
		sw2_mask_count = 0;
		P2IE &= ~SW2; //disable Interrupts
		if(global_count > 8 || global_count < 1){
			send_strn((char *)RANGE_ERR, RANGE_ERR_SIZE);
			send_char(';');
		}else{
			global_count++;
			print_count();
		}
		P2IFG &= ~SW2; //interrupt handled
	}
}
