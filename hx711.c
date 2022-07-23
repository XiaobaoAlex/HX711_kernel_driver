#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/power_supply.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/scatterlist.h>
#define CONFIG_OF
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <linux/suspend.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/of_gpio.h>

static int hx_remove(struct platform_device *dev);
static int hx_probe(struct platform_device *dev);
void F_hx711_Stop(void);
int get_adc_data(void);

//定义设备结构体，包含设备名义引脚

struct hx711_platform_data {
	const char *name;
	u32 pd_sck_gpio;
	u32 dout_gpio;

};
//	 PC		内核         内核
//DTS ->DTB-> device_node->platform_device
//
//定义对应hx711设备的具体结构体，该结构体可以完全表示hx711
struct hx711_platform_data *pdata_g;
//将设备树里面的值（实际IO）读到并赋给hx711_platform_data结构体(名义引脚)
static int hx711_parse_dt(struct device *dev,
			struct hx711_platform_data *pdata)
{
	//int rc;
	struct device_node *node;
	//struct device_node *child_node;
	//int gpio_num1=0;
//	int gpio_num2=0;
	//得到设备树节点
	//传入np，就是设备树的节点，然后返回 "hx711" 字符串对应的值，存入pdata->name 里面。
	node = of_find_node_by_path("/hx711");
	if(node==NULL)
	printk( "********  get device_node failed !********\n");
	printk(KERN_ALERT "name: %s",node->name); //输出节点名
	pdata->name=node->name;
	printk( "******** %s ********\n",pdata->name);


	/*获取 rgb_led_red_device_node 的子节点*/
    //  child_node = of_get_next_child(node,NULL);
    //  if(child_node == NULL)
    //  {
    //      printk(KERN_ALERT "\n get child_node failed ! \n");
    //      return -1;
    //  }
     //printk(KERN_ALERT "name: %s",child_node->name); //输出节点名
	/* sdn, lod_p and lod_n gpio info */
	pdata->pd_sck_gpio = of_get_named_gpio(node,"pd-sclk-gpio",0);//解析dts并且获取描述对应gpio口：函数返回值为gpio号。
	printk( "******** %d ********\n",pdata->pd_sck_gpio);
	//printk( "******** %d ********\n",gpio_num1);
	if (pdata->pd_sck_gpio < 0)
		return pdata->pd_sck_gpio;
		

	pdata->dout_gpio = of_get_named_gpio(node,"pd-ldout-gpio",0);
	printk( "******** %d ********\n",pdata->dout_gpio);
	//printk( "******** %d ********\n",gpio_num2);
	if (pdata->dout_gpio < 0)
		return pdata->dout_gpio;
	


	printk( "******** %s ********\n",__func__);//
	return 0;

}



int hx711_GPIO_init(void)
{
	int err = 0;

	if (gpio_is_valid(pdata_g->pd_sck_gpio)) //判断io口是否有效，return true if valid.
	{
		printk("clk_gpio_request\n");
		err = gpio_request(pdata_g->pd_sck_gpio,
					"hx-pd-clk");//有效返回0，无效申请io，并为这个端口命名为"hx-pd-clk"
		if (err) {
			printk("request hx-pd-clk gpio error\n");
		}//申请错误，打印信息
	
		err = gpio_direction_output(pdata_g->pd_sck_gpio,0);//指定方向
		if (err) {
			printk("set hx-pd-clk gpio direction error\n");
		}//方向申请错误，打印信息
		gpio_export(pdata_g->pd_sck_gpio, 0);//导出这个IO口到用户空间
	}
	
	if (gpio_is_valid(pdata_g->dout_gpio)) {
		printk("dout_gpio_request\n");
		err = gpio_request(pdata_g->dout_gpio,
					"hx-dout");
		if (err) {
			printk("request hx-dout gpio error\n");
		}
	
		err = gpio_direction_input(pdata_g->dout_gpio);
		if (err) {
			printk("set hx-dout gpio direction error\n");
		}
		gpio_export(pdata_g->dout_gpio, 0);
	}
	printk("hx711_GPIO_init\n");

	return 0;

}

static ssize_t hx711_read(struct file *file, char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t ret = 0;
	int adc_data;
	//printk("hx711_read\n");
	//return heartbeat_read(file,buffer,count,offset);
	adc_data = get_adc_data();
	ret =  copy_to_user(buffer, &adc_data, sizeof(adc_data));//将内核空间的数据传入用户空间，buffer为用户空间存储adc值的地址
	//如果数据拷贝成功，则返回零；否则，返回没有拷贝成功的数据字节数。
	if(ret)
	{
		printk("ret = %d\n", ret);//内核通过 printk() 输出相关信息
	}
	return count;
}

static ssize_t hx711_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset)
{
    char buf[64];
	size_t buf_size;
	unsigned int state;
	printk("hx711_write\n");
	memset(buf, 0, sizeof(buf));//将某一块内存设置为指定值，buf为指定的内存，0为指定值，sizeof(buf)为设置大小
	buf_size = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, buffer, buf_size))
		return -EFAULT;
	sscanf(buf, "%d", &state);//解析字符串，将字符串buf转换为整数型state
/*
	if (state == 1)
	{
		F_hx711_Start();
	}
	else if( state == 0)
	{
		F_hx711_Stop();
	}
	else if(state == 2)
	{
		mdelay(200);
	}
*/
	printk(KERN_CRIT "hx711 buffer %s, state %d\n",buf,state);
	return count;
}

#define hx711_ADC_DEVNAME "hx_adc"
#define HX711_STOP _IO('E', 0)
#define HX711_START _IO('E', 1)
#define HX711_SWITCH_CHANNEL _IOW('E', 2, int)
#define HX711_GET_STATUS _IOR('E', 3, int)
#define HX711_GET_DATA _IOWR('E', 4, int)
#define hx711_GET_STRING _IOWR('E', 5, char*)



int get_adc_data(void)
{
	unsigned long Count;
	unsigned char i;
	gpio_set_value(pdata_g->pd_sck_gpio, 0);
	Count = 0;
//	while(ADDO);
	for (i=0; i<24; i++){
		gpio_set_value(pdata_g->pd_sck_gpio, 1);
		Count = Count << 1;
		gpio_set_value(pdata_g->pd_sck_gpio, 0);
		if(gpio_get_value(pdata_g->dout_gpio)) Count++;
	}
	gpio_set_value(pdata_g->pd_sck_gpio, 1);
	Count = Count ^ 0x800000;
	gpio_set_value(pdata_g->pd_sck_gpio, 0);
	return(Count);

}

void F_hx711_Stop(void)
{
	gpio_set_value(pdata_g->pd_sck_gpio, 1);
	usleep_range(60, 500);//这个函数最早会在形参min时间醒来，最晚会在max这个时间醒来。这个函数不是让cpu忙等待，而是让出cpu

}
void F_hx711_Start(void)
{
	gpio_set_value(pdata_g->pd_sck_gpio, 0);
}

//提供用户在read和write功能之外的，与硬件相关的操作
static long hx711_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	int err = 0;
	int v= 0;
//	printk(KERN_CRIT "hx711_ioctl cmd %d\n",cmd);
	switch (cmd)
		{
		case HX711_GET_STATUS:
			v = gpio_get_value(pdata_g->dout_gpio);
			//err = put_user(v, (int *)arg);//将一个简单的值v写进用户空间,写入的地址为arg，
			break;

		case HX711_START:
			{
				F_hx711_Start();
				break;
			}
		case HX711_STOP:
			{
				F_hx711_Stop();
				break;
			}
		case HX711_GET_DATA:
			{

				v = get_adc_data();
				//err =  copy_to_user((int *)arg, &v, sizeof(v));//将内核空间的数据传入用户空间，buffer为用户空间存储adc值的地址
				if(err)
					{
						printk("ret = %d\n", err);//内核通过 printk() 输出相关信息
					}
				break;
			}



		default:
			err = -ENOIOCTLCMD;
			break;
		}

	return err;
}


static int hx711_open(struct inode *inode, struct file *file)
{
    printk("hx711_open\n");
	return 0;
}
static int hx711_release(struct inode *inode, struct file *file)
{
	printk(KERN_CRIT "hx711_release\n");

    return 0;
}

#ifdef CONFIG_OF
static struct of_device_id hx711_match_table[] = {
	{ .compatible = "hx,hx711",},
	{ },
};
MODULE_DEVICE_TABLE(of, hx711_match_table);

#else
#define hx711_match_table NULL
#endif

//面向对象的设计思想，把一个事件抽象为一个对象
//用结构体来表示某一个对象
static const struct file_operations hx711_fops = {
    .owner = THIS_MODULE,
	.open = hx711_open,
	.read = hx711_read,
	.write= hx711_write,
	.release = hx711_release,
	.unlocked_ioctl = hx711_ioctl,

};

static struct miscdevice hx711_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "misc_hx711",
	.fops  = &hx711_fops,
};
//平台设备结构体
static struct platform_device hx711_platform_device = {
    .name   = "hx711",
    .id        = 2,
};
//平台驱动结构体
//注册device之后probe被调用
//注销device之后remove被调用
static struct platform_driver hx711_platform_driver = {
	.probe = hx_probe,
	.remove = hx_remove,
	//.suspend = hx_pm_suspend,
	//.resume = hx_pm_resume,
	.driver = {
		   .name = "hx",//"hx",
		   .owner    = THIS_MODULE,
		   .of_match_table = of_match_ptr(hx711_match_table),
	},
};

static int hx_remove(struct platform_device *dev)
{
	return 0;
}
//1、记录引脚资源：获取使用哪些引脚，这些引脚分别是什么。
//2、device_creat自动创造设备节点，在probe函数中根据平台设备的资源确定创建多少个引脚
static int hx_probe(struct platform_device *dev)
{
	int ret = 0;
//	struct hx711_platform_data *pdata;

	printk( "******** hx driver probe!! ********\n");

	if (dev->dev.of_node) {
		pdata_g = devm_kzalloc(&dev->dev,
			sizeof(struct hx711_platform_data), GFP_KERNEL);
		if (!pdata_g)
			return -ENOMEM;

		ret = hx711_parse_dt(&dev->dev, pdata_g);
		printk( "******** hx711 get infor from device-tree ********\n");
		if (ret) {
			dev_err(&dev->dev, "DT parsing failed\n");
			return ret;
		}
	}
	else
	{
		dev_err(&dev->dev, "hx, no of_node\n");
		pdata_g = dev->dev.platform_data;
	}

	if (!pdata_g) {
		dev_err(&dev->dev, "Invalid pdata\n");
		return -EINVAL;
	}
	
	hx711_GPIO_init();

	ret = misc_register(&hx711_device);
	if(ret)
	{
		printk(KERN_CRIT "******** hx_device registerd failed! ********\n");
		return ret;
	}

	printk(KERN_CRIT  "hx711_init : done\n");

	return ret;

}
//入口函数
static int __init hx711_init(void)
{
	int ret =0;
	printk(KERN_CRIT "hx711_init\n");
//注册platform_device设备结构体到内核
	ret = platform_device_register(&hx711_platform_device);
	if (ret) {
		printk(KERN_CRIT "****[hx711_platform_device] Unable to device register(%d)\n", ret);
		return ret;
	}
//注册platform_driver驱动程序结构体到内核

	ret = platform_driver_register(&hx711_platform_driver);
	if (ret) {
		printk(KERN_CRIT "****[hx711_platform_driver] Unable to register driver (%d)\n", ret);
		return ret;
	}
    return ret;
}
//出口函数
static void __exit hx711_exit(void)
{
	platform_device_unregister(&hx711_platform_device);
	misc_deregister(&hx711_device);
	platform_driver_unregister(&hx711_platform_driver);

}
module_init(hx711_init);//指定hx711_init为入口函数，宏，定义一个结构体，会用到这个函数
module_exit(hx711_exit);//指定hx711_exit为出口函数
MODULE_LICENSE("GPL");//必须指定GPL协议
