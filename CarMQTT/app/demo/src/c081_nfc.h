#ifndef __C081_NFC_H__
#define __C081_NFC_H__

#include <hi_i2c.h>

// #define INTERRUPT
#define CHECK

#define     NFC_I2C_REG_ARRAY_LEN           (32)  
#define     NFC_SEND_BUFF                   (3)
#define     C08I_NFC_DEMO_TASK_STAK_SIZE    (1024*10)
#define     C08I_NFC_DEMO_TASK_PRIORITY     (24)
#define     NFC_TAG_HISTREAMING

#define CLA                                 (1)
#define INS                                 (2)
#define P1                                  (3)    
#define P2                                  (4)
#define LC                                  (5)
#define DATA                                (6)

typedef enum {
    NONE, 
    CC_FILE,
    NDEF_FILE 
} T4T_FILE;

#define C081_NFC_ADDR       0xAE // 7 bit slave device address  1010 111 0/1
#define I2C_WR              0x00
#define I2C_RD              0x01
#define C081_NFC_READ_ADDR  0xAF
#define C081NFC_WRITE_ADDR    (C081_NFC_ADDR|I2C_WR)
#define C081NFC_READ_ADDR     (C081_NFC_ADDR|I2C_RD)
#define FM11_E2_USER_ADDR   0x0010
#define FM11_E2_MANUF_ADDR  0x03FF
#define FM11_E2_BLOCK_SIZE  16


/*FM11NC08i 寄存器*/
#define FM327_FIFO				0xFFF0
#define FIFO_FLUSH_REG			0xFFF1
#define	FIFO_WORDCNT_REG		0xFFF2
#define RF_STATUS_REG			0xFFF3
#define RF_TXEN_REG				0xFFF4
#define RF_BAUD_REG				0xFFF5
#define RF_RATS_REG				0xFFF6
#define MAIN_IRQ_REG			0xFFF7
#define FIFO_IRQ_REG			0xFFF8
#define AUX_IRQ_REG				0xFFF9
#define MAIN_IRQ_MASK_REG		0xFFFA
#define FIFO_IRQ_MASK_REG		0xFFFB
#define AUX_IRQ_MASK_REG		0xFFFC
#define NFC_CFG_REG				0xFFFD
#define VOUT_CFG_REG			0xFFFE
#define EE_WR_CTRL_REG			0xFFFF


#define MAIN_IRQ				0xFFF7
#define FIFO_IRQ				0xFFF8
#define AUX_IRQ		    	    0xFFF9
#define MAIN_IRQ_MASK		    0xFFFA
#define FIFO_IRQ_MASK		    0xFFFB
#define AUX_IRQ_MASK	        0xFFFC
#define FIFO_FLUSH			    0xFFF1
#define	FIFO_WORDCNT		    0xFFF2



#define MAIN_IRQ_RF_PWON        0x80 
#define MAIN_IRQ_ACTIVE         0x40
#define MAIN_IRQ_RX_START       0x20
#define MAIN_IRQ_RX_DONE        0x10
#define MAIN_IRQ_TX_DONE        0x08
#define MAIN_IRQ_ARBIT          0x04
#define MAIN_IRQ_FIFO           0x02
#define MAIN_IRQ_AUX            0x01
#define FIFO_IRQ_WL             0x08



extern hi_u8 irq_data_in;
extern hi_u8 irq_rxdone;
extern hi_u8 irq_txdone;
extern hi_u8 FlagFirstFrame;
extern hi_u8 isr_flag;

hi_u32 c08i_nfc_i2c_write( hi_u8 reg_high_8bit_cmd, hi_u8 reg_low_8bit_cmd, hi_u8* data_buff, hi_u8 len);
hi_void *app_i2c_nfc_demo(hi_void* param);
hi_void app_c08i_nfc_i2c_demo_task(hi_void);
hi_void fm11_t4t(hi_void);
hi_u32 fm11_data_recv(hi_u8 *rbuf);
hi_u8  fm11_write_reg(hi_u16 addr, hi_u8 data);
hi_u32 fm11_read_eep(hi_u8 *dataBuff, hi_u16 ReadAddr, hi_u16 len);
hi_void nfc_init(hi_void);
hi_void fm11_write_eep(hi_u16 addr,hi_u32 len,hi_u8 *wbuf);
hi_void sEE_WritePage(hi_u8 *pBuffer, hi_u16 WriteAddr, hi_u8 datalen);
hi_void fm11_data_send(hi_u32 ilen,hi_u8 *ibuf);
hi_u8 fm11_write_fifo(hi_u8 *pbuf, hi_u8 len);
hi_u32 write_read(hi_u8 reg_high_8bit_cmd, hi_u8 reg_low_8bit_cmd, hi_u8* recv_data, hi_u8 send_len, hi_u8 read_len);
hi_u8 fm11_read_reg(hi_u16 addr);
hi_u32 write_fifo_reg( hi_u8 reg_high_8bit_cmd, hi_u8 reg_low_8bit_cmd, hi_u8 data_buff);
hi_u32 write_fifo_data( hi_u8* data_buff, hi_u8 len);
#endif