#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <linux/hiddev.h>

#define HX711_DEV "/dev/misc_hx711"


#define HX711_STOP _IO('E', 0)
#define HX711_START _IO('E', 1)
#define HX711_SWITCH_CHANNEL _IOW('E', 2, int)
#define HX711_GET_STATUS _IOR('E', 3, int)
#define HX711_GET_DATA _IOWR('E', 4, int)
#define hx711_GET_STRING _IOWR('E', 5, char*)


int power_supply_on(int on)
{
	int ret = 0;
	
	return ret;
}

int get_power_stat()
{
	int ret = 0;
	return ret;

}
int lock_open(int on)
{
	int ret = 0;
	return ret;

}

int get_status(void)
{
	int ret = 0;
	return ret;

}

int main(int argc, char *argv[])
{


  int fd_hx711 = -1;
  int ret = 0;
  int value;
  char c;

    fd_hx711 = open(HX711_DEV,O_RDWR);
	if(fd_hx711 ==  -1)
	{
	       fprintf(stderr,"open %s failure\n",HX711_DEV);
	       return -1;
	}

	if(argc < 2)
	{
		ret = read(fd_hx711, &value, sizeof(value));
		if(ret < 0) {
	     fprintf(stdout, "read_error\n");
	     return -1;
	     }
		printf("adc value = %d\n",value);
	}
	else
	{
		while ((c = getopt(argc, argv, "c:gsd")) != -1)
		{
//			c = getopt(argc, argv, "c:gsd");
			printf("argc = %d\n", argc);
			switch (c) {
				case 'c':
          value = atoi(optarg);
          printf("The argument of -c is %d\n\n", value);
          if((value >= 0) && (value < 16))
          {
	 					ret = ioctl(fd_hx711, HX711_SWITCH_CHANNEL, &value);
						if(ret)
						{
							printf("ioctl error = %d\n", ret);
						}
						else
						{
							printf("set channel to %d\n", value);
						}
					}else{
						printf("channel %d is out of range!!!\n\n", value);
					}      
				break;
	
				case 'g':
					ret = ioctl(fd_hx711, HX711_GET_STATUS, &value);
					if(ret)
					{
						printf("ioctl error = %d\n", ret);
					}
					else
					{
						printf("get status = %d\n", value);
					}
				break;
	
				case 'd':
					ret = ioctl(fd_hx711, HX711_GET_DATA, &value);
					if(ret)
					{
						printf("ioctl error = %d\n", ret);
					}
					else
					{
						printf("get data = %d\n", value);
					}
				break;
				case 's':
					
				break;
				default:
					return 1;
			}
			
			usleep(20000);
		}
	}
	
        

    close(fd_hx711);
	
    fprintf(stdout, "value= %d\n",ret);
    return ret;
}

