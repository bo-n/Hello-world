#include <linux/init.h>
#include <linux/kernel.h>/* We're doing kernel work*/
#include <linux/module.h>/* Specifically, a module */
#include <linux/fs.h>
#include <asm/uaccess.h>/*for get_user and put_user*/
#include <asm/io.h>
/*
*硬件寄存器参数
*/
#define DEVICE_NAME "/dev/dma_d"//待改
#define XPAR_PS7_TOP_BASEDDR 0X40000000
#define XPAR_PS7_TOP_REG_SIZE 0x3FFFFFFF

/*addr_offset*/
#define ADDR_STATUS 0x00
#define ADDR_CONTROL 0x04
#define ADDR_ID 0x08
#define ADDR_MADDR 0x0C
#define ADDR_CHNL 0x10
#define ADDR_BTT 0x14
