#include "adder.h"
#include "xadder_hw.h"
#include <linux/init.h>
#include <linux/kernel.h>/* We're doing kernel work*/
#include <linux/module.h>/* Specifically, a module */
#include <linux/fs.h>
#include <asm/uaccess.h>/*for get_user and put_user*/
#include <asm/io.h>
/*
*硬件寄存器参数
*/
#define DEVICE_NAME "/dev/top_dev"
#define XPAR_PS7_TOP_BASEDDR 0x43c00000
#define XPAR_PS7_TOP_REG_SIZE 0x74
#define AP_START 0x01
#define AP_DONE  0x02
#define AP_IDLE  0x04
#define AP_READY 0x08
#define AP_RESTART 0x80
#define IOCTL_ADD 0
#define IOCTL_REVOKE 1
/*******************************************
*驱动
*/
static void *dev_map_addr;
static int major_num;
static int Device_Open = 0;
/*
*
*/
static int device_open(struct inode *inode, struct file *filp)
{
	//prink(KERN_INFO "device_open(%p)\n", filp);
	
	/*
	*We don't want to talk to two processes at the same time
	*/
	if (Device_Open)
		return -EBUSY;
	Device_Open++;
	/*
	* Initialize the message
	*/
	// Message_Ptr = Message;//？？？
	try_module_get(THIS_MODULE);//模块使用计数管理宏，用于增加模块使用计数。如果模块已经插入内核，则递增该模块引用计数；如果该模块还没有插入内核，则返回0表示出错
	return 0;
}
static int device_release(struct inode *inode, struct file *filp)
{
	
	//printk(KERN_INFO "device_release(%p,%p)\n",inode, filp);
	
	/*
	*We're now ready for our next caller
	*/
	Device_Open--;
	module_put(THIS_MODULE);//用于减少模块使用计数
	return 0;
}
static ssize_t device_read(
                struct file *filp,/* see include/linux/fs.h */
                char __user * buffer,/* buffer to be filled with data */
                size_t length,/* length of the buffer */
                loff_t * offset
				)
{
	
	return 0;
}
static ssize_t device_write(
                struct file *filp,
                const char __user * buffer,
                size_t length,
                loff_t * offset
                )
{
	return 0;
}
/*
*
*/
static int device_ioctl(
                struct file *filp,/* ditto 文件标示符*/
                unsigned int ioctl_num,/* number and param for ioctl */
                unsigned long ioctl_param
                )
{
	/*
	* Switch according to the ioctl called
	*/
	//Param kk_data;
	unsigned char k_data[2];

	unsigned char c_data;
	
	//printk(KERN_INFO "\nsb\n");//KERN_INFO信息消息。许多驱动程序在启动时刻用它来输出获得的硬件信息。
	switch (ioctl_num)
	{
		case IOCTL_ADD:
		// printk(KERN_INFO "device_apply\n");
		copy_from_user(k_data, (unsigned char *)ioctl_param, sizeof(unsigned char)*2);//从用户空间拷贝数据到内核空间，失败返回没有被拷贝的字节数，成功返回0
				
		//   while(((*(unsigned char*)(dev_map_addr + XADDER_ADDER_IO_ADDR_AP_CTRL))&AP_IDLE)!=AP_IDLE);
		
			*(volatile unsigned char*)(dev_map_addr + XADDER_ADDER_IO_ADDR_A_DATA)= k_data[0]; //a
			*(volatile unsigned char*)(dev_map_addr + XADDER_ADDER_IO_ADDR_B_DATA)= k_data[1]; //b			
		    *(volatile unsigned char*)(dev_map_addr + XADDER_ADDER_IO_ADDR_AP_CTRL)|=AP_START;  
		    while(((*(volatile unsigned char *)(dev_map_addr + XADDER_ADDER_IO_ADDR_AP_CTRL))&AP_DONE)!=AP_DONE)
		    ;
				c_data = *(volatile unsigned char*)(dev_map_addr + XADDER_ADDER_IO_ADDR_C_DATA);
			    copy_to_user((unsigned char*)ioctl_param, &c_data, sizeof(unsigned char));
	    break;
	    case IOCTL_REVOKE://撤销
		// printk(KERN_INFO "device_revoke\n");
		
		// while(((*(unsigned char *)(dev_map_addr + XTOP_TOP_IO_ADDR_AP_CTRL))&AP_IDLE)!= AP_IDLE);
		    *(volatile unsigned char*)(dev_map_addr + XADDER_ADDER_IO_ADDR_A_DATA)=0; 
			*(volatile unsigned char*)(dev_map_addr + XADDER_ADDER_IO_ADDR_B_DATA)=0; 
			*(volatile unsigned char*)(dev_map_addr + XADDER_ADDER_IO_ADDR_AP_CTRL)|=AP_START;
			while(((*(volatile unsigned char *)(dev_map_addr + XADDER_ADDER_IO_ADDR_AP_CTRL))&AP_DONE)!=AP_DONE)
				;	
	     	*(volatile unsigned char*)(dev_map_addr + XADDER_ADDER_IO_ADDR_C_DATA)=0;
			break;
		}
		return 0;
// printk(KERN_INFO "device_end\n");
}
/*
*
*/
struct file_operations Fops= {
	              .read = device_read,
				  .write = device_write,
				  .unlocked_ioctl = device_ioctl,
				  .open = device_open,
				  .release = device_release /*close*/
};

/*
*
*/
int init_module()
{
	// int ret_val=0;
	major_num = register_chrdev(0, DEVICE_NAME, &Fops);//获取设备编号
	if(major_num < 0)
	{
		//printk(KERN_ALERT "%s failed with %d\n","Sorry, registerging the character device ",ret_val);
		return -1;
	}
 printk(KERN_INFO "%s The major device number is %d.\n","Registeration is a success",major_num);
// printk(KERN_INFO "If you want to talk to the device driver,\n");
// printk(KERN_INFO "Than create a device file by following command. \n");
//printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, major_num);
// printk(KERN_INFO "The device file name is important, because\n");
// printk(KERN_INFO "the ioctl program assume that's the\n");
// printk(KERN_INFO "file you'll use.\n");
    dev_map_addr=ioremap(XPAR_PS7_TOP_BASEDDR, XPAR_PS7_TOP_REG_SIZE);//物理地址映射到虚拟地址
	if(dev_map_addr<0)
	{
	//	printk(KERN_INFO "failed! Sorry, mapping error ");
		return -1;
	}
	return 0;
}
void cleanup_module()
{
	iounmap(dev_map_addr);
	unregister_chrdev(major_num,DEVICE_NAME);
}



MODULE_LICENSE("GPL");
