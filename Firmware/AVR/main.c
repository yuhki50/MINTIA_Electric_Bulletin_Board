/*
 *  MINTIA Electric Bulletin Board for AVR ATmega328P
 *  main.c �N���b�N8MHz�Ő݌v
 *  by yuhki50 2011/07/26
 *  WinAVR 20100110
 */


#include "common.h"
//#include "usart/usart.h"  // debug
#include "gif/gif.h"
#include "led_matrix_ctrl/led_matrix_ctrl.h"


static inline void port_init(void);
static inline void timer_init(void);
static inline void interrupt_init(void);
static inline void module_init(void);
static inline void osc_init(void);


volatile uint16_t frame[LMC_HEIGHT_COUNT] = {
	#include "bitmap.h"
};


int main(void){
	// �f�o�C�X������ //
	port_init();
	timer_init();
	interrupt_init();
	module_init();
	osc_init();
//	usart_init(USART_BAUDRATE);

	// SD�J�[�h�����肷��܂ő҂� //
	_delay_ms(1000);

	// ���Ӄf�o�C�X������ //
	ledMatrixCtrl_init();
	ledMatrixCtrl_disable();
	ledMatrixCtrl_enable();

	// ���荞�݋��� //
	sei();

	// �X�^�[�g�A�b�v��ʂ̕\������ //
//	_delay_ms(500);

	// GIF�t�@�C���ǂݍ���//
	while(1) {
		gif_open("mintia.gif");
		convertImages();
	}
}


/* �|�[�g������ */
static inline void port_init(void) {
	DDRB = INIT_DDRB;
	DDRC = INIT_DDRC;
	DDRD = INIT_DDRD;
	PORTB = INIT_PORTB;
	PORTC = INIT_PORTC;
	PORTD = INIT_PORTD;
}


/* �^�C�}�[������ */
static inline void timer_init(void) {
	TCCR0A = INIT_TCCR0A;
	TCCR0B = INIT_TCCR0B;
	TCNT0 = INIT_TCNT0;
	OCR0A = INIT_OCR0A;
	OCR0B = INIT_OCR0B;
	TIMSK0 = INIT_TIMSK0;
}


/* ���荞�ݏ����� */
static inline void interrupt_init(void) {
	
}


/* �������W���[�������� */
static inline void module_init(void) {
	ACSR = INIT_ACSR;
	PRR = INIT_PRR;
}


/* �����I�V���[�^�Z�� */
static inline void osc_init(void) {
	uint8_t osc_cal = eeprom_read_byte(OSCCAL_EEPROM_ADDRESS);

	if(!(osc_cal == 0x00) && !(osc_cal == 0xFF)) {
		OSCCAL = osc_cal;
	}
}


/* �^�C�}�[0���荞�� (LED�}�g���b�N�X�\��) */
ISR(TIMER0_COMPA_vect) {
	static uint8_t rowCount = 0;

	if(rowCount >= LMC_HEIGHT_COUNT) {
		rowCount = 0;
		ledMatrixCtrl_disable();
		ledMatrixCtrl_enable();
	} else {
		ledMatrixCtrl_writeFrameAsync((uint16_t*)frame);
		rowCount++;
	}
}
