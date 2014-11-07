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

//Accelerometer definitions
#define PWR_PIN BIT0
#define X_PIN BIT1
#define Y_PIN BIT2
#define PWR_PIN BIT0
#define X_CHANNEL ADC12INCH_1
#define Y_CHANNEL ADC12INCH_2
#define ACC_LIMIT 1

int x_offset;
int y_offset;
int dx;
int dy;

int acc_count;
const char * HEX_CHARS = "0123456789ABCDEF";

void send_char(char c);
void send_strn(char* str, int n);
void send_hex12(int c);
void setup_db_buttons();
void setup_UART();
void setup_accelerometer();

void print_acc_debug();
void print_offsets();

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	setup_UART();
	setup_accelerometer();
	setup_db_buttons();

	__bis_SR_register(GIE);

	while(1){

	}
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

void send_hex12(int c){
	//c3c2c1
	char chars[3];
	chars[2] = HEX_CHARS[c & 15];
	chars[1] = HEX_CHARS[(c>>4) & 15];
	chars[0] = HEX_CHARS[(c>>8) & 15];
	send_strn("0x", 2);
	send_strn(chars, 3);
}

void setup_accelerometer(){
	//Setup required pins
	P6SEL |= X_PIN | Y_PIN;
	P6DIR |= PWR_PIN;
	P6DIR &= ~(X_PIN | Y_PIN);
	P6OUT |= PWR_PIN;

	//Analog-Digital converter setup
	ADC12CTL0 = ADC12ON + ADC12SHT0_6 + ADC12MSC; 	//Turn on, 128 cycles in sampling time, multi sample conversion
	ADC12CTL1 = ADC12SHP + ADC12CONSEQ_3;			//Sample & Hold mode and repeat sequence of channels
	ADC12CTL2 = ADC12RES_2;							//12 bit resolution
	ADC12MCTL0 = X_CHANNEL;
	ADC12MCTL1 = Y_CHANNEL + ADC12EOS;
	ADC12CTL0 |= ADC12ENC | ADC12SC; 				// start A-D conversion

	//  let accelerometer settle
	__delay_cycles(20000);

	//Get offset values
	/* Not in part b!
	x_offset = ADC12MEM0;
	y_offset = ADC12MEM1;*/
}

void setup_UART(){
	P5SEL |= BIT6 | BIT7;                     // P3 goes to LCD, P5 over the line
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
	acc_count++;

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
	if(acc_count > ACC_LIMIT){
		dx = ADC12MEM0;
		dy = ADC12MEM1;
		acc_count = 0;
	}
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void){
	if(P2IFG & SW1 && P2IE & SW1){
		sw1_mask_count = 0;
		P2IE &= ~SW1; //disable Interrupts
		send_strn("X = ", 4);
		send_hex12(dx);
		send_char(';');
		P2IFG &= ~SW1; //interrupt handled
	}
	if(P2IFG & SW2 && P2IE & SW2){
		sw2_mask_count = 0;
		P2IE &= ~SW2; //disable Interrupts
		send_strn("Y = ", 4);
		send_hex12(dy);
		send_char(';');
		P2IFG &= ~SW2; //interrupt handled
	}
}

void print_acc_debug(){
	send_hex12(dx);
	send_char('|');
	send_hex12(dy);
	send_char(' ');
	send_char('?');
	send_hex12(ADC12MEM0);
	send_char('|');
	send_hex12(ADC12MEM1);
	send_char('\n');
}

void print_offsets(){
	send_strn("X_OFF= ",7);
	send_hex12(x_offset);
	send_strn(" | Y_OFF= ",10);
	send_hex12(y_offset);
	send_char('\n');
}
