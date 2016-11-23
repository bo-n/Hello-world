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


/*SET*/
//#define STATUS_SET 0x11100001//ID=1,CFG=1,MAJOR=1,MINOR=0,ERR=0,IS=0,IDLE=1
#define CONTROL_IE 0x4//IE=0
/*CMD*/
//#define CONTROL_CMD_M2L 0x6
//#define CONTROL_CMD_L2M 0x7
/*START*/
#define STATUS_START 0xFFFFFFFE//IDLE=0
#define CONTROL_START 0x4//clear CMD
/*WAIT*/
#define STATUS_IDLE 0x00000001//idle 
/*ERR*/
#define STATUS_ERR 0x000000F0
/*clear irq*/
#define CONTROL_IC 0x8


#define START 0
#define RELEASE 1

/*******************************************
*驱动
*/
static void *dev_map_addr;
static int major_num;
static int Device_Open = 0;
/*
*
*/
static int device_open(struct inode *inode, struct file *file)
{
	//prink(KERN_INFO "device_open(%p)\n", file);
	
	/*
	*We don't want to talk to two processes at the same time
	*/
	if (Device_Open)
		return -EBUSY;
	Device_Open++;
	/*
	* Initialize the message
	*/
	// Message_Ptr = Message;
	try_module_get(THIS_MODULE);
	return 0;
}
static int device_release(struct inode *inode, struct file *file)
{
	
	//printk(KERN_INFO "device_release(%p,%p)\n",inode, file);
	
	/*
	*We're now ready for our next caller
	*/
	Device_Open--;
	module_put(THIS_MODULE);
	return 0;
}
static ssize_t device_read(
                struct file *file,/* see include/linux/fs.h */
                char __user * buffer,/* buffer to be filled with data */
                size_t length,/* length of the buffer */
                loff_t * offset
				)
{
	return 0;
}
static ssize_t device_write(
                struct file *file,
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

static char *itoa_bin(unsigned int data)  
{  
unsigned char *str;
 int i;
 for(i=0;i<4;i++)
    {  
        if(data & 0x1)  
            *(str+3-i) = '1';  
        else  
            *(str+3-i) = '0';    
        data >>= 1;  
    } 
 *(str+4)='\0';
return str;
}
static int device_ioctl(
                struct file *file,/* ditto 文件标示符*/
                unsigned int ioctl_num,/* number and param for ioctl */
                unsigned long ioctl_param
                )
{
	/*
	* Switch according to the ioctl called
	*/
	
	unsigned int data[5];
	unsigned int status;
	unsigned int a;
	unsigned int b;
	unsigned char *err;
//printk(KERN_INFO "\nsb\n");
	switch (ioctl_num)
	{
		case START:
		status= *(unsigned int*)(dev_map_addr + ADDR_STATUS);
		*(volatile unsigned int*)(dev_map_addr + ADDR_CONTROL)=CONTROL_IC;//clear interrupt
	//printk(KERN_INFO "STATUS:%x\n",status);
		while(((*(unsigned int*)(dev_map_addr + ADDR_STATUS))&STATUS_IDLE)!=STATUS_IDLE);//等待直至空闲

		*(volatile unsigned int*)(dev_map_addr + ADDR_CONTROL)= CONTROL_IE;//enable irq
		copy_from_user(data,(unsigned int *)ioctl_param, sizeof(unsigned int)*5);
		*(volatile unsigned int*)(dev_map_addr + ADDR_ID)= data[0];//ID
		*(volatile unsigned int*)(dev_map_addr + ADDR_MADDR)= data[1];//MADDR
		*(volatile unsigned int*)(dev_map_addr + ADDR_CHNL)= data[2];//CHNL
        *(volatile unsigned int*)(dev_map_addr + ADDR_BTT)= data[3];//BTT
		
		*(volatile unsigned int*)(dev_map_addr + ADDR_CONTROL)=data[4];//CONTROL_CMD:0x00000006 memory2logic, 0x00000007 logic2memory
		
		while(((*(unsigned int*)(dev_map_addr + ADDR_STATUS))|STATUS_START)!=STATUS_START);//等待直到忙碌（IDLE=0），DMA开始工作
		*(volatile unsigned int*)(dev_map_addr + ADDR_CONTROL)=CONTROL_START;//clear CMD
			//printk(KERN_INFO "STATUS:%x\n",status);
		if(((*(unsigned int*)(dev_map_addr + ADDR_CONTROL))&CONTROL_IE)!=CONTROL_IE)//if interrupt is disabled, 轮询IDLE
			{	//printk(KERN_INFO "STATUS:%x\n",status);
				while(((*(unsigned int*)(dev_map_addr + ADDR_STATUS))&STATUS_IDLE)!=STATUS_IDLE)
				;}
		/*report error code	*/

		while((status&STATUS_ERR)!=0x00000000)
		{
			a = status%256;
			b = a/16;
            err=itoa_bin(b);
		    printk(KERN_INFO "STATUS_ERR: %s\n",err);
		}
			
		*(volatile unsigned int*)(dev_map_addr + ADDR_CONTROL)=CONTROL_IC;	//clear interrupt	
		break;
		
		case RELEASE:
		*(volatile unsigned int*)(dev_map_addr + ADDR_ID)= 0;//ID
		*(volatile unsigned int*)(dev_map_addr + ADDR_MADDR)= 0;//MADDR
		*(volatile unsigned int*)(dev_map_addr + ADDR_CHNL)= 0;//CHNL
		
		*(volatile unsigned int*)(dev_map_addr + ADDR_CONTROL)= 0;//CONTROL_CMD:0x6 memory2logic, 0x7 logic2memory		
		
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
	major_num = register_chrdev(0, DEVICE_NAME, &Fops);
	if(major_num < 0)
	{
		//printk(KERN_ALERT "%s failed with %d\n","Sorry, registerging the character device ",ret_val);
		return -1;
	}
 printk(KERN_INFO "%s The major device number is %d.\n","Registeration is a success",major_num);
// printk(KERN_INFO "If you want to talk to the device driver,\n");
// printk(KERN_INFO "Than create a device file by following command. \n");
// printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, major_num);
// printk(KERN_INFO "The device file name is important, because\n");
// printk(KERN_INFO "the ioctl program assume that's the\n");
// printk(KERN_INFO "file you'll use.\n");
    dev_map_addr=ioremap(XPAR_PS7_TOP_BASEDDR, XPAR_PS7_TOP_REG_SIZE);
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
