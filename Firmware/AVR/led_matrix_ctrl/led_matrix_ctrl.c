/*
 *  LED Matrix Controller
 *  led_matrix_ctrl.c
 *  by yuhki50 2011/06/20
 */


#include "led_matrix_ctrl.h"


// プロトタイプ //
static inline void writeLine(uint16_t line);
static inline void writePadding();


/* 初期化 */
void ledMatrixCtrl_init()
{
    DDRD |= _BV(LMC_MOSI) | _BV(LMC_SCLK) | _BV(LMC_SS);
}


/* 有効化 */
void ledMatrixCtrl_enable()
{
	LMC_SS_L();
}


/* 無効化 */
void ledMatrixCtrl_disable()
{
	LMC_SS_H();
}


/* 1フレームをブロッキングで表示 */
uint8_t ledMatrixCtrl_writeFrame(uint16_t *frame)
{
	for(uint8_t row=0; row<LMC_HEIGHT_COUNT; row++) {
		// データを転送 //
		writeLine(frame[row]);

		// 消灯時間 //
		_delay_us(LMC_TIME_OFF);

		// パディング //
		writePadding();

		// 点灯時間 //
		_delay_us(LMC_TIME_ON);
	}

	return 0;
}


/* 1フレームをノンブロッキングで表示 */
uint8_t ledMatrixCtrl_writeFrameAsync(uint16_t *frame)
{
	static uint8_t row = 0;

	// データを転送 //
	writeLine(frame[row]);

	// パディング //
	writePadding();

	// カウンターをインクリメント //
	row = (row + 1) % LMC_HEIGHT_COUNT;

	return row;
}


/* 1ラインを書き込み */
static void writeLine(uint16_t line)
{
	// データを転送 //
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


/* パディングを書き込み */
static inline void writePadding()
{
	for(uint8_t i=0; i<8; i++) {
		LMC_SCLK_H();
		LMC_SCLK_L();
	}
}

