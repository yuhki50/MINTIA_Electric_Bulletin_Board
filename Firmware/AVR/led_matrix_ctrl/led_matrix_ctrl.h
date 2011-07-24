/*
 *  LED Matrix Controller
 *  led_matrix_ctrl.h
 *  by yuhki50 2011/06/20
 */


#ifndef _LED_MATRIX_CTRL_H
#define _LED_MATRIX_CTRL_H


#include <avr/io.h>
#include <util/delay.h>


// �s���A�T�C�� //
#define LMC_MOSI PD0
#define LMC_SCLK PD1
#define LMC_SS PD2

// LED Matrix�T�C�Y //
#define LMC_WIDTH_COUNT 16  // 1-16
#define LMC_HEIGHT_COUNT 14  // 1-256

// LED���邳���� //
#define LMC_TIME_ON 300  //us
#define LMC_TIME_OFF 50  //us

// �r�b�g���я� //
#define LMC_BITODER 0  // MSBFIRST: 0; LSBFIRST: 1


// �}�N���֐� //
#define LMC_MOSI_H() PORTD |= _BV(LMC_MOSI)
#define LMC_MOSI_L() PORTD &= ~_BV(LMC_MOSI)
#define LMC_SCLK_H() PORTD |= _BV(LMC_SCLK)
#define LMC_SCLK_L() PORTD &= ~_BV(LMC_SCLK)
#define LMC_SS_H() PORTD |= _BV(LMC_SS)
#define LMC_SS_L() PORTD &= ~_BV(LMC_SS)


// �v���g�^�C�v //
void ledMatrixCtrl_init();
void ledMatrixCtrl_enable();
void ledMatrixCtrl_disable();
uint8_t ledMatrixCtrl_writeFrame(uint16_t *frame);
uint8_t ledMatrixCtrl_writeFrameAsync(uint16_t* frame);


#endif
