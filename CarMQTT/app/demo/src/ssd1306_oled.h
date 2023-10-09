#ifndef __SSD1306_OLED_H__
#define __SSD1306_OLED_H__

#include <hi_types_base.h>
#include <hi_i2c.h>

#define OLED_ADDRESS                0x78 //默认地址为0x78
#define OLED_ADDRESS_WRITE_CMD      0x00 // 0000000 写命令
#define OLED_ADDRESS_WRITE_DATA     0x40 //0100 0000(0x40)

static hi_u32 i2c_write_byte(hi_u8 reg_addr, hi_u8 cmd);
static hi_void write_cmd(hi_u8 cmd);
static hi_void write_data(hi_u8 i2c_data);
hi_void oled_init(hi_void);
hi_void oled_set_position(hi_u8 x, hi_u8 y) ;
hi_void oled_fill_screen(hi_u8 fii_data);
hi_void i2c_oled_write(hi_i2c_idx id, hi_u8* cmd, hi_u16 device_addr, hi_u8 send_len);
hi_void i2c_oled_write_read(hi_i2c_idx id, hi_u16 device_addr, hi_u32 recv_len);
hi_void *app_i2c_oled_demo(hi_void* param);
hi_void oled_show_char(hi_u8 x, hi_u8 y, hi_u8 chr, hi_u8 char_size);
hi_void oled_show_str(hi_u8 x,hi_u8 y,hi_u8 *chr,hi_u8 char_size);
hi_u32 app_oled_i2c_demo_task(hi_void);
hi_void oled_position_clean_screen(hi_u8 fill_data, hi_u8 line, hi_u8 pos, hi_u8 len);
#endif