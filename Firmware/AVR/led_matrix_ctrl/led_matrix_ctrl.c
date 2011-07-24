/*
 *  LED Matrix Controller
 *  led_matrix_ctrl.c
 *  by yuhki50 2011/06/20
 */


#include "led_matrix_ctrl.h"


// �v���g�^�C�v //
static inline void writeLine(uint16_t line);
static inline void writePadding();


/* ������ */
void ledMatrixCtrl_init()
{
    DDRD |= _BV(LMC_MOSI) | _BV(LMC_SCLK) | _BV(LMC_SS);
}


/* �L���� */
void ledMatrixCtrl_enable()
{
	LMC_SS_L();
}


/* ������ */
void ledMatrixCtrl_disable()
{
	LMC_SS_H();
}


/* 1�t���[�����u���b�L���O�ŕ\�� */
uint8_t ledMatrixCtrl_writeFrame(uint16_t *frame)
{
	for(uint8_t row=0; row<LMC_HEIGHT_COUNT; row++) {
		// �f�[�^��]�� //
		writeLine(frame[row]);

		// �������� //
		_delay_us(LMC_TIME_OFF);

		// �p�f�B���O //
		writePadding();

		// �_������ //
		_delay_us(LMC_TIME_ON);
	}

	return 0;
}


/* 1�t���[�����m���u���b�L���O�ŕ\�� */
uint8_t ledMatrixCtrl_writeFrameAsync(uint16_t *frame)
{
	static uint8_t row = 0;

	// �f�[�^��]�� //
	writeLine(frame[row]);

	// �p�f�B���O //
	writePadding();

	// �J�E���^�[���C���N�������g //
	row = (row + 1) % LMC_HEIGHT_COUNT;

	return row;
}


/* 1���C������������ */
static void writeLine(uint16_t line)
{
	// �f�[�^��]�� //
	for(uint8_t col=0; col<LMC_WIDTH_COUNT; col++) {
#if LMC_BITODER == 0
		// MSBFIRST //
		if((line >> (LMC_WIDTH_COUNT - col - 1)) & 0x0001) {
			LMC_MOSI_L();
		} else {
			LMC_MOSI_H();
		}

#elif LMC_BITODER == 1
		// LSBFIRST //
		if((line >> col) & 0x0001) {
			LMC_MOSI_L();
		} else {
			LMC_MOSI_H();
		}
#endif

		LMC_SCLK_H();
		LMC_SCLK_L();
	}
}


/* �p�f�B���O���������� */
static inline void writePadding()
{
	for(uint8_t i=0; i<8; i++) {
		LMC_SCLK_H();
		LMC_SCLK_L();
	}
}

