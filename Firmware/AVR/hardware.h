/*
 *  LEDメッセンジャー ハードウェア定義 for AVR ATmega328P
 *  hardware.h
 *  by yuhki50 2010/10/30
 */


#ifndef _HARDWARE_H
#define _HARDWARE_H


/* ポート初期化設定 */
#define INIT_DDRB 0b00000000
#define INIT_DDRC 0b00000000
#define INIT_DDRD 0b00000000
#define INIT_PORTB 0b00000000
#define INIT_PORTC 0b00000000
#define INIT_PORTD 0b00000000


/* タイマー初期化 */
#define INIT_TCCR0A (0)  // 標準動作
#define INIT_TCCR0B (2)  // プリスケーラ 8分周 2
#define INIT_TCNT0 (0x00)  // カウンター値
#define INIT_TIMSK0 _BV(TOIE0)  // オーバーフロー割り込み有効


/* 内部モジュール初期化 */
#define INIT_ACSR _BV(ACD)  // アナログコンパレータOFF
//#define INIT_PRR _BV(PRTWI) | _BV(PRTIM1) | _BV(PRTIM2) | _BV(PRADC)  // 電力削減レジスタ
#define INIT_PRR _BV(PRTWI) | _BV(PRTIM1) | _BV(PRTIM2) | _BV(PRUSART0) | _BV(PRADC)  // 電力削減レジスタ


/* 内部オシレータ校正 */
#define OSCCAL_EEPROM_ADDRESS (uint8_t*)(0x03FF)


/* USARTボーレート */
#define USART_BAUDRATE 9600


/* ステータスLED */
#define STATUS_LED0 PC0
#define STATUS_LED1 PC1
#define STATUS_PORT PORTC


#endif
