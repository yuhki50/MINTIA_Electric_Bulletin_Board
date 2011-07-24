/*----------------------------------------------------------------*/
/* SD�J�[�h�ǂݍ��݃��C�u�����w�b�_        (C)KAMAI(TOPOZO), 2010 */
/*----------------------------------------------------------------*/

#ifndef _SDRL
#define _SDRL

#include <avr/io.h>

/*------------------------------------*/
/* �|�[�g�ʖ�                         */
/*------------------------------------*/
#define SCK PB5
#define MISO PB4
#define MOSI PB3
#define SS PB2

/*------------------------------------*/
/* �}�N��                             */
/*------------------------------------*/
#define SS_H() PORTB |= _BV(SS)
#define SS_L() PORTB &= ~_BV(SS)

/*------------------------------------*/
/* �֐�                               */
/*------------------------------------*/
uint8_t sd_init(uint8_t* const block_flg);
uint8_t sd_read(unsigned long address, uint8_t buf[], uint16_t offset, uint16_t count);

#endif
