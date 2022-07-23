# HX711_kernel_driver
 ## 称重传感器hx711-liunx内核驱动。
若在用户空间驱动IO口，即使不加延时，脉冲的周期也很长，正电平时间远超传感器要求的50us，无法驱动传感器。
下图为HX711传感器的通信时序图。

![在这里插入图片描述](https://img-blog.csdnimg.cn/f0e24db0a2304931a495a0f1eb0d9a21.png)
故编写内核驱动，此处参考了韦东山嵌入式linux的视频。
内核驱动组成：

![在这里插入图片描述](https://img-blog.csdnimg.cn/44b3e9a61f5f4c90a0bd8b85623cd61d.png)

设备树写法：

```c
	 hx711 {
        status = "okay";
        compatible = "hx,hx711"; 
        pd-sclk-gpio = <&gpio3 30 GPIO_ACTIVE_HIGH>;
        pd-ldout-gpio = <&gpio3 28 GPIO_ACTIVE_HIGH>;
		linux,default-trigger = "gpio";
		default-state = "off";
    };
```
读取传感器数据的核心函数
```c
int get_adc_data(void)
{
	unsigned long Count;
	unsigned char i;
	gpio_set_value(pdata_g->pd_sck_gpio, 0);
	Count = 0;
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
```

几个注意点：

 1. 设备树的写法
 2. 从设备树获取信息的函数：获取设备树节点和获取GPIO口的函数，我试了好多种才试对，有时候稍微差一点也不行，大家参照我代码里面的应该没问题。
 3. 该例程注册的是杂项设备：，在用户空间调用的时候要与注册的设备名称一致
 4. 调试时可多用内核打印函数printk打印信息，这样就能发现问题出在哪里。
