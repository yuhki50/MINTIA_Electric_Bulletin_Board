/*----------------------------------------------------------------*/
/* SDカード読み込みライブラリ              (C)KAMAI(TOPOZO), 2010 */
/*----------------------------------------------------------------*/

#include "sd.h"

/*------------------------------------*/
/* プロトタイプ宣言                   */
/*------------------------------------*/
static uint8_t spi_inout(uint8_t d);
static uint8_t sd_send_cmd(uint8_t cmd, unsigned long data, uint8_t crc);

/*------------------------------------*/
/* 関数名：sd_init                    */
/* 戻り値：エラーコード(0なら正常終了)*/
/* 引数：ブロック・フラグへのポインタ */
/* 機能：SDカードの初期化処理を行う   */
/*------------------------------------*/
uint8_t sd_init(uint8_t* const block_flg)
{
	uint16_t cnt;		// 送信カウンタ
	uint8_t	res,		// レスポンス
			type = 0;	// 種類：0:MMC 1:SD1 2:SD2
	/* SPI初期化 */
	DDRB |= _BV(SCK) | _BV(MOSI) | _BV(SS);
	SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR1);	// Mode0
	SPSR = _BV(SPI2X); 							// 1/32 (250kHz)
	/* コマンド受付準備 */
	SS_H();
	for(cnt = 10 ; cnt ; cnt--) spi_inout(0xff);
	/* ダミー・ウェイト */
	SS_L();
	for(cnt = 1000 ; cnt ; cnt--) spi_inout(0xff);
	/* ソフトウェア・リセット */
	cnt = 10;
	do{
		res = sd_send_cmd(0, 0, 0x4a);
	} while(res != 0x01 && --cnt);
	if(res == 0x01){
		/* SDCv2判定 */
		if(
			sd_send_cmd(8, 0x1aa, 0x43) < 2 &&
			(
				(spi_inout(0xff) << 8) | spi_inout(0xff) |
				(spi_inout(0xff) & 0x00) | (spi_inout(0xff) & 0x00)
			) == 0x1aa
		) type = 2;
		/* SDCv1判定 */
		else if(sd_send_cmd(55, 0, 0) < 2 && (res = sd_send_cmd(41, 0, 0)) < 2) type = 1;
		/* 初期化 */
		cnt = 30000;
		while(res && --cnt){
			res = type ? sd_send_cmd(55, 0, 0) : 0x00;
			if(res < 2) res = sd_send_cmd(type ? 41 : 1, (type == 2) ? 1UL << 30 : 0, 0);
		}
		/* 設定 */
		if(res == 0x00){
			/* ブロック・アドレッシング方式判定 */
			res = sd_send_cmd(58, 0, 0);
			*block_flg = (spi_inout(0xff) & 0x40) ? 1 : 0;
			spi_inout(0xff); spi_inout(0xff); spi_inout(0xff);
			if(res == 0x00 && *block_flg == 0) res = sd_send_cmd(16, 512, 0); // ブロック長セット
		
		}
		else res = 0x81;
	}
	else if(res == 0x00) res = 0x80;
	SS_H();
	/* クロック変更 */
	SPCR &= ~_BV(SPR1);	// 1/2(4Mhz)
	return res;
}

/*------------------------------------*/
/* 関数名：sd_read                    */
/* 戻り値：エラーコード(0なら正常終了)*/
/* 引数：アドレス                     */
/* データ・ポインタ                   */
/* オフセット                         */
/* 読込バイト数                       */
/* 機能：SDカードの読み込みを行う     */
/*------------------------------------*/
uint8_t sd_read(unsigned long address, uint8_t buf[], uint16_t offset, uint16_t count)
{
	register uint16_t cnt;	// カウンタ
	uint8_t res;			// レスポンス

	/* シングル・ブロック・リード */
	res = sd_send_cmd(17, address, 0);
	if(res == 0x00){
		/* データ・スタート(0xFE)またはエラー・レスポンス */
		for(cnt = 10000 ; cnt && ((res = spi_inout(0xff)) == 0xff) ; cnt--);
		if(res == 0xfe){
			res = 0x00;
			/* データ格納 */
			for(cnt = offset ; cnt ; cnt--) spi_inout(0xff);					// オフセット移動
			for(; cnt < count ; cnt++) buf[cnt] = spi_inout(0xff);			// 指定バイト読込
			for(cnt = 512 - offset - count ; cnt ; cnt--) spi_inout(0xff);	// 残り移動
			spi_inout(0xff); spi_inout(0xff);									// CRC読み込み
		}
	}
	SS_H();
	return res;
}

/*------------------------------------*/
/* 関数名：spi_inout                  */
/* 戻り値：受信データ                 */
/* 引数：送信データ                   */
/* 機能：SPIでデータの送受信を行う    */
/*------------------------------------*/
static uint8_t spi_inout(uint8_t d)
{
	SPDR = d;
	while(!(SPSR & _BV(SPIF)));
	return SPDR;
}

/*------------------------------------*/
/* 関数名：sd_send_cmd                */
/* 戻り値：レスポンス                 */
/* 引数：コマンド                     */
/* データ                             */
/* 機能：コマンドを送信する           */
/*------------------------------------*/
static uint8_t sd_send_cmd(uint8_t cmd, unsigned long data, uint8_t crc)
{
	uint8_t res; //レスポンス
	int8_t i;
	
	/* ダミークロック, 有効化 */
	SS_H(); spi_inout(0xff); SS_L();
	/* スタート・ビット,コマンド */
	spi_inout(0x40 | cmd);
	/* データ */
	for(i = 24 ; i >= 0 ; i -= 8) spi_inout(data >> i);
	/* CRC,ストップ・ビット */
	spi_inout((crc << 1) | 0x01);
	/* レスポンス */
	for(i = 0 ; ((res = spi_inout(0xff)) & 0x80) && i < 8 ; i++);
	return res;
}
