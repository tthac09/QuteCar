#ifndef __TH06_H__
#define __TH06_H__

#define TH06_DEVICE_ADDR        0x40 //设备地址
#define TH06_WRITE_ADDR         0x80
#define TH06_READ_ADDR          0x81


typedef enum{
    TEMPERATURE =1,
    HUMIDITY    =2,
}th06_serson_type;
#endif
 