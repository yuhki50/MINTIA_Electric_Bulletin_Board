/*----------------------------------------------------------------*/
/* SD�J�[�h�ǂݍ��݃��C�u����              (C)KAMAI(TOPOZO), 2010 */
/*----------------------------------------------------------------*/

#include "sd.h"

/*------------------------------------*/
/* �v���g�^�C�v�錾                   */
/*------------------------------------*/
static uint8_t spi_inout(uint8_t d);
static uint8_t sd_send_cmd(uint8_t cmd, unsigned long data, uint8_t crc);

/*------------------------------------*/
/* �֐����Fsd_init                    */
/* �߂�l�F�G���[�R�[�h(0�Ȃ琳��I��)*/
/* �����F�u���b�N�E�t���O�ւ̃|�C���^ */
/* �@�\�FSD�J�[�h�̏������������s��   */
/*------------------------------------*/
uint8_t sd_init(uint8_t* const block_flg)
{
	uint16_t cnt;		// ���M�J�E���^
	uint8_t	res,		// ���X�|���X
			type = 0;	// ��ށF0:MMC 1:SD1 2:SD2
	/* SPI������ */
	DDRB |= _BV(SCK) | _BV(MOSI) | _BV(SS);
	SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR1);	// Mode0
	SPSR = _BV(SPI2X); 							// 1/32 (250kHz)
	/* �R�}���h��t���� */
	SS_H();
	for(cnt = 10 ; cnt ; cnt--) spi_inout(0xff);
	/* �_�~�[�E�E�F�C�g */
	SS_L();
	for(cnt = 1000 ; cnt ; cnt--) spi_inout(0xff);
	/* �\�t�g�E�F�A�E���Z�b�g */
	cnt = 10;
	do{
		res = sd_send_cmd(0, 0, 0x4a);
	} while(res != 0x01 && --cnt);
	if(res == 0x01){
		/* SDCv2���� */
		if(
			sd_send_cmd(8, 0x1aa, 0x43) < 2 &&
			(
				(spi_inout(0xff) << 8) | spi_inout(0xff) |
				(spi_inout(0xff) & 0x00) | (spi_inout(0xff) & 0x00)
			) == 0x1aa
		) type = 2;
		/* SDCv1���� */
		else if(sd_send_cmd(55, 0, 0) < 2 && (res = sd_send_cmd(41, 0, 0)) < 2) type = 1;
		/* ������ */
		cnt = 30000;
		while(res && --cnt){
			res = type ? sd_send_cmd(55, 0, 0) : 0x00;
			if(res < 2) res = sd_send_cmd(type ? 41 : 1, (type == 2) ? 1UL << 30 : 0, 0);
		}
		/* �ݒ� */
		if(res == 0x00){
			/* �u���b�N�E�A�h���b�V���O�������� */
			res = sd_send_cmd(58, 0, 0);
			*block_flg = (spi_inout(0xff) & 0x40) ? 1 : 0;
			spi_inout(0xff); spi_inout(0xff); spi_inout(0xff);
			if(res == 0x00 && *block_flg == 0) res = sd_send_cmd(16, 512, 0); // �u���b�N���Z�b�g
		
		}
		else res = 0x81;
	}
	else if(res == 0x00) res = 0x80;
	SS_H();
	/* �N���b�N�ύX */
	SPCR &= ~_BV(SPR1);	// 1/2(4Mhz)
	return res;
}

/*------------------------------------*/
/* �֐����Fsd_read                    */
/* �߂�l�F�G���[�R�[�h(0�Ȃ琳��I��)*/
/* �����F�A�h���X                     */
/* �f�[�^�E�|�C���^                   */
/* �I�t�Z�b�g                         */
/* �Ǎ��o�C�g��                       */
/* �@�\�FSD�J�[�h�̓ǂݍ��݂��s��     */
/*------------------------------------*/
uint8_t sd_read(unsigned long address, uint8_t buf[], uint16_t offset, uint16_t count)
{
	register uint16_t cnt;	// �J�E���^
	uint8_t res;			// ���X�|���X

	/* �V���O���E�u���b�N�E���[�h */
	res = sd_send_cmd(17, address, 0);
	if(res == 0x00){
		/* �f�[�^�E�X�^�[�g(0xFE)�܂��̓G���[�E���X�|���X */
		for(cnt = 10000 ; cnt && ((res = spi_inout(0xff)) == 0xff) ; cnt--);
		if(res == 0xfe){
			res = 0x00;
			/* �f�[�^�i�[ */
			for(cnt = offset ; cnt ; cnt--) spi_inout(0xff);					// �I�t�Z�b�g�ړ�
			for(; cnt < count ; cnt++) buf[cnt] = spi_inout(0xff);			// �w��o�C�g�Ǎ�
			for(cnt = 512 - offset - count ; cnt ; cnt--) spi_inout(0xff);	// �c��ړ�
			spi_inout(0xff); spi_inout(0xff);									// CRC�ǂݍ���
		}
	}
	SS_H();
	return res;
}

/*------------------------------------*/
/* �֐����Fspi_inout                  */
/* �߂�l�F��M�f�[�^                 */
/* �����F���M�f�[�^                   */
/* �@�\�FSPI�Ńf�[�^�̑���M���s��    */
/*------------------------------------*/
static uint8_t spi_inout(uint8_t d)
{
	SPDR = d;
	while(!(SPSR & _BV(SPIF)));
	return SPDR;
}

/*------------------------------------*/
/* �֐����Fsd_send_cmd                */
/* �߂�l�F���X�|���X                 */
/* �����F�R�}���h                     */
/* �f�[�^                             */
/* �@�\�F�R�}���h�𑗐M����           */
/*------------------------------------*/
static uint8_t sd_send_cmd(uint8_t cmd, unsigned long data, uint8_t crc)
{
	uint8_t res; //���X�|���X
	int8_t i;
	
	/* �_�~�[�N���b�N, �L���� */
	SS_H(); spi_inout(0xff); SS_L();
	/* �X�^�[�g�E�r�b�g,�R�}���h */
	spi_inout(0x40 | cmd);
	/* �f�[�^ */
	for(i = 24 ; i >= 0 ; i -= 8) spi_inout(data >> i);
	/* CRC,�X�g�b�v�E�r�b�g */
	spi_inout((crc << 1) | 0x01);
	/* ���X�|���X */
	for(i = 0 ; ((res = spi_inout(0xff)) & 0x80) && i < 8 ; i++);
	return res;
}
