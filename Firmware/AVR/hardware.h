/*
 *  LED���b�Z���W���[ �n�[�h�E�F�A��` for AVR ATmega328P
 *  hardware.h
 *  by yuhki50 2010/10/30
 */


#ifndef _HARDWARE_H
#define _HARDWARE_H


/* �|�[�g�������ݒ� */
#define INIT_DDRB 0b00000000
#define INIT_DDRC 0b00000000
#define INIT_DDRD 0b00000000
#define INIT_PORTB 0b00000000
#define INIT_PORTC 0b00000000
#define INIT_PORTD 0b00000000


/* �^�C�}�[������ */
#define INIT_TCCR0A _BV(WGM01)  // �W������
#define INIT_TCCR0B (5)  // �v���X�P�[�� 1024����
#define INIT_TCNT0 (0)  // �J�E���^�[�l
#define INIT_OCR0A (4)  // �J�E���^�[�l
#define INIT_OCR0B (0)  // �J�E���^�[�l
#define INIT_TIMSK0 _BV(OCIE0A)  // �I�[�o�[�t���[���荞�ݗL��


/* �������W���[�������� */
#define INIT_ACSR _BV(ACD)  // �A�i���O�R���p���[�^OFF
//#define INIT_PRR _BV(PRTWI) | _BV(PRTIM1) | _BV(PRTIM2) | _BV(PRUSART0) | _BV(PRADC)  // �d�͍팸���W�X�^
#define INIT_PRR _BV(PRTWI) | _BV(PRTIM1) | _BV(PRTIM2) | _BV(PRADC)  // �d�͍팸���W�X�^



/* �����I�V���[�^�Z�� */
#define OSCCAL_EEPROM_ADDRESS (uint8_t*)(0x03FF)


/* USART�{�[���[�g */
#define USART_BAUDRATE 9600


/* �X�e�[�^�XLED */
#define STATUS_LED0 PC0
#define STATUS_LED1 PC1
#define STATUS_PORT PORTC

#define STATUS_LED0_ON() STATUS_PORT |= _BV(STATUS_LED0)
#define STATUS_LED0_OFF() STATUS_PORT &= ~_BV(STATUS_LED0)
#define STATUS_LED0_TOGGLE() STATUS_PORT ^= _BV(STATUS_LED0)
#define STATUS_LED1_ON() STATUS_PORT |= _BV(STATUS_LED1)
#define STATUS_LED1_OFF() STATUS_PORT &= ~_BV(STATUS_LED1)
#define STATUS_LED1_TOGGLE() STATUS_PORT ^= _BV(STATUS_LED1)


#endif
