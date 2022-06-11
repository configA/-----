#include "STDIO.H"
#include "REG52.H"
#include "motor.h"
#include "bsp_key.h"
#include "delay.h"
#include "lcd1602.h"
#include "bsp_time.h"
#include "24c02.h"


//-----主要函数
void key_proc(void);
void seg_proc(void);
//-----定时器变量
long time_tick=0; 
unsigned int key_delay;
unsigned int seg_delay;
unsigned int motor_delay;
unsigned int count;//运行时间
unsigned int count_down;//每个过程的倒计时
xdata unsigned int Start_Time;
//------1602变量
unsigned char seg_sring_det[17]; //第一行变量
unsigned char seg_sring_tasks[17];//第二行变量
//------按键变量
unsigned char key_val;
unsigned char key_old,key_down;
//------电机变量
xdata unsigned char step=0;
xdata unsigned char dir=0;//电机方向
xdata unsigned int motor_num;//表示是否需要改变一次方向
//-----全局变量定义
unsigned char key_num=0;
unsigned char pattern;//模式选择
unsigned char interface=0x01;//界面选择
xdata unsigned char WS_PT[4][4]={{5,15,7,7},{5,30,15,15},{5,40,20,20},{5,5,5,5}}; //洗涤 漂洗 脱水  定时时间初始化
unsigned char WS_PT_4[3];
unsigned int Process=0;//0无  1进水 2洗涤 3漂洗 4脱水 
bit flag;//1 代表完成本过程
bit Refresh_flag;//但遇到9或99时只刷新一次
unsigned char custom_time[5];//自定义的时间
unsigned char custom_num;//输入的位数
bit custom_flag;//表示按下去了
/*******************************************************************************
* 函 数 名       : main
* 函数功能		 : 主函数
* 输    入       : 无
* 输    出    	 : 无
*******************************************************************************/
void main()
{	
	lcd1602_init();//LCD1602初始化
	time0_init();
	WS_PT[3][1] = at24c02_read_one_byte(1); //1洗涤时间 2漂洗时间 3脱水时间
	delay_ms(10);
	WS_PT[3][2] = at24c02_read_one_byte(2); //1洗涤时间 2漂洗时间 3脱水时间
	delay_ms(10);
	WS_PT[3][3] = at24c02_read_one_byte(3); //1洗涤时间 2漂洗时间 3脱水时间
	delay_ms(10);
	while(1)
	{			
		key_proc();	
		seg_proc();		
	}		
}



/***********************定时器0中断函数(1ms)***********************/
void time0() interrupt 1 
{
	time_tick ++;
	TH0=0XFC;	//给定时器赋初值，定时1ms
	TL0=0X18;
	
	if(key_delay)//只有不等于0的时候可以加
		if(++key_delay == 6) key_delay = 0;
	
	if(seg_delay)//只有不等于0的时候可以加
		if(++seg_delay == 300) seg_delay = 0;
	
	if(!(time_tick % 1000))
		count++;//运行时间
}

/***********************按键函数***********************/
void key_proc(void)
{
	if(key_delay) return;
		else key_delay = 1;
	key_val = key_read();
	key_down = key_val & (key_val ^ key_old);
	motor_num++;
	if(motor_num >= 300)
	{
		motor_num = 0;
		dir?(dir=0):(dir=1);
	}
	//界面  0x01:选择洗涤模式  0x02:洗涤模式确认  0x03:进水状态 
	//自定义时间界面 0x10:自定义模式确认  0x20:使用上一次的设置值还是重新设置  0x30:设置洗涤时间
	switch(key_down)
  {
		case 1:
			key_num = 1;
			custom_flag = 1;
			if(interface == 0x01)
			{
				pattern = 1;
				interface = 0x02;
			}
		break;
		case 2:
			key_num = 2;
			custom_flag = 1;
			if(interface == 0x01)
			{
				pattern = 2;
				interface = 0x02;
			}
			
		break;
		case 3:
			key_num = 3;
			if(interface == 0x01)
			{
				pattern = 3;
				interface = 0x02;
			}
			custom_flag = 1;
		break;
		case 4:
			key_num = 4;
			if(interface == 0x01)
			{
				interface = 0x10;
			}
			if(interface == 0x30)
				custom_flag = 1;
		break;
		
		case 5: key_num = 5;  custom_flag = 1; break;
		case 6: key_num = 6;  custom_flag = 1; break;
		case 7: key_num = 7;  custom_flag = 1; break;
		case 8: key_num = 8;  custom_flag = 1; break;	
		case 9: key_num = 9;  custom_flag = 1; break;
		case 10: key_num = 0;  custom_flag = 1; break;
			
		case 15:
			if(interface == 0x10)
			{
				interface = 0x20;//0x20:使用上一次的设置值还是重新设置
				lcd1602_clear();
				break; //不干扰到下面
			}
			
			if(interface == 0x02)
			{
				interface = 0x03;
				lcd1602_clear();
				Process = 1;//表示进入进水模式
				Start_Time = count;//提供开始的时间
				seg_sring_tasks[0] = '\0';//清空数组
				flag = 0;//表示过程未完成
			}
			
			if(interface == 0x20)
			{
				pattern = 4; //第四个
				Process = 1;//表示进入进水模式
				Start_Time = count;//提供开始的时间
				seg_sring_tasks[0] = '\0';//清空数组
				flag = 0;//表示过程未完成
				lcd1602_clear();
				interface = 0x03;
			}
		break;
			
		case 16:
			if((interface == 0x02) || (interface == 0x10))
			{
				interface = 0x01;
				lcd1602_clear();
			}
			if(interface == 0x07)
			{
				interface = 0x01;//重新开始
				lcd1602_clear();
			}
			if(interface == 0x20)
			{
				interface = 0x30;//设置洗涤时间
				custom_time[0] = 0;
				custom_time[1] = 0;
				custom_time[4] = 0;
			}
		break;
			
		case 13: //0x30:设置洗涤时间  
			if(interface == 0x30)
			{
				WS_PT[3][1] = custom_time[4];
				at24c02_write_one_byte(1,WS_PT[3][1]);
				interface = 0x40;
				custom_time[0] = 0;
				custom_time[1] = 0;
				custom_time[4] = 0;
				custom_num = 0;
				lcd1602_clear();
			}
			if((interface == 0x40) && (custom_num))
			{
				WS_PT[3][2] = custom_time[4];
				at24c02_write_one_byte(2,WS_PT[3][2]);
				interface = 0x50;
				custom_time[0] = 0;
				custom_time[1] = 0;
				custom_time[4] = 0;
				custom_num = 0;
				lcd1602_clear();
			}
			if((interface == 0x50) && (custom_num))
			{
				WS_PT[3][3] = custom_time[4];
				at24c02_write_one_byte(3,WS_PT[3][3]);
				custom_time[0] = 0;
				custom_time[1] = 0;
				custom_time[4] = 0;
				pattern = 4; //第四个
				Process = 1;//表示进入进水模式
				Start_Time = count;//提供开始的时间
				seg_sring_tasks[0] = '\0';//清空数组
				flag = 0;//表示过程未完成
				lcd1602_clear();
				interface = 0x03;
			}
		break;
		case 14:
			if(interface == 0x30)
			{
					custom_time[custom_num] = 0;
					custom_num--;
					lcd1602_clear();
			}
		break;
	}
	
	key_old = key_val;
	//speed速度值越小速度越快(1-5)  dir方向(0逆时针 其他顺时针)
		if((Process == 0) || (interface == 0x07))//无 或者结束
			return;//返回
		else if(Process == 1)
			step_motor_send_pulse(step++,0);	
		else if((Process == 4) || (Process == 2) || (Process == 3))
			step_motor_send_pulse(step++,dir);	
			if(step==8)step=0;
}

/***********************1602函数***********************/
void seg_proc(void)
{
	if(seg_delay) return;
		else seg_delay = 1;
	if(flag)//当完成过程时
	{
		lcd1602_clear();//清屏
		Process++;//最大只有4
		interface++;//最大只有0x06
		Start_Time = count;//提供开始的时间
		seg_sring_tasks[0] = '\0';//清空数组
		seg_sring_tasks[1] = '\0';//清空数组
		flag = 0;//表示过程未完成
	}
	count_down = WS_PT[(unsigned int)pattern-1][Process-1]-(count-Start_Time);
	if(((count_down == 9 )|| (count_down == 99)) && (!Refresh_flag))
	{
		lcd1602_clear();//清屏
		Refresh_flag = 1;
	}
	
	if(!(count_down == 9 )|| (count_down == 99))
		Refresh_flag = 0;//不是9或99时标志位为0
	
	if((custom_flag) && ((interface == 0x30) || (interface == 0x40) || (interface == 0x50)))
	{
		custom_time[custom_num] = key_num;
		if(++custom_num == 3) custom_num =2;//只能设置两位数
		lcd1602_clear();
	} 
	if(custom_num == 0)
		custom_time[4] = 0;//第四位表示输入的时间
	else if(custom_num == 1)
		custom_time[4] = custom_time[0];//第四位表示输入的时间
	else if(custom_num == 2)
		custom_time[4] = (custom_time[0] * 10) + custom_time[1];//第四位表示输入的时间
		custom_flag = 0;
	
	//界面  0x01:选择洗涤模式  0x02:洗涤模式确认  0x03:进水状态  0x04:洗涤  0x05:漂洗  0x06脱水
	//自定义时间界面 0x10:自定义模式确认  0x20:使用上一次的设置值还是重新设置  0x30:设置洗涤时间
	switch(interface)
  {
		case 0x01:  
			sprintf(seg_sring_det,"Select Mode:"); //第一行显示请选择模式
			//sprintf(seg_sring_det,"%d",(unsigned int)key_num);//调试
			sprintf(seg_sring_tasks,"%d",(unsigned int)count);
		break;
		
		//下面是普通模式界面
		case 0x02:
			sprintf(seg_sring_det,"Select Mode:%d?",(unsigned int)pattern); //第一行显示选择的模式
			sprintf(seg_sring_tasks,"S15:ACK S16:NACK");
		break;
		
		case 0x03://进水
			sprintf(seg_sring_det,"Add water: %d",(unsigned int)count_down); //第一行显示正在进水
			//sprintf(seg_sring_det,"%d %d  %d",Start_Time,count,(unsigned int)WS_PT[(unsigned int)pattern-1][Process-1]); //调试
			//开始的时间  所需的时间  现在的时间 输出的数组
			Progress_Bar(Start_Time,(float)WS_PT[(unsigned int)pattern-1][Process-1],count,seg_sring_tasks);//进度条
		break;
		
		case 0x04://洗涤
			sprintf(seg_sring_det,"Washing:%d %d",(unsigned int)count_down,(unsigned int)dir); //第一行显示正在洗涤
			//sprintf(seg_sring_det,"%d %d  %d %d",Start_Time,count,(unsigned int)WS_PT[(unsigned int)pattern-1][Process-1],Process); //调试
			Progress_Bar(Start_Time,(float)WS_PT[(unsigned int)pattern-1][Process-1],count,seg_sring_tasks);//进度条
		break;
		
		case 0x05://漂洗
			sprintf(seg_sring_det,"Rinse:%d",(unsigned int)count_down); //第一行显示正在漂洗
			//sprintf(seg_sring_det,"%d %d  %d",Start_Time,count,(unsigned int)WS_PT[(unsigned int)pattern-1][Process-1]); //调试
			Progress_Bar(Start_Time,(float)WS_PT[(unsigned int)pattern-1][Process-1],count,seg_sring_tasks);//进度条
		break;
		
		case 0x06://脱水
			sprintf(seg_sring_det,"Dehydration:%d",(unsigned int)count_down); //第一行显示正在脱水
		//sprintf(seg_sring_det,"%d %d  %d",Start_Time,count,(unsigned int)WS_PT[(unsigned int)pattern-1][Process-1]); //调试
			Progress_Bar(Start_Time,(float)WS_PT[(unsigned int)pattern-1][Process-1],count,seg_sring_tasks);//进度条
		break;
		
		case 0x07://完成
			sprintf(seg_sring_det,"Complete!!!"); //第一行显示完成
			//sprintf(seg_sring_det,"%d %d  %d",Start_Time,count,(unsigned int)WS_PT[(unsigned int)pattern-1][Process-1]); //调试
			sprintf(seg_sring_tasks,"S16:Res"); //第一行显示完成
		break;
		
		
		//下面是自定义模式界面
		case 0x10:
			sprintf(seg_sring_det,"Custom mode?"); //第一行显示是否进入自定义模式
			sprintf(seg_sring_tasks,"S15:ACK S16:NACK");
		break;
		
		case 0x20:
			sprintf(seg_sring_det,"S15:Last"); //第一行显示是否进入自定义模式
			sprintf(seg_sring_tasks,"S16:Reset");
		break;
		
		case 0x30:
			sprintf(seg_sring_det,"Washing_set:%d",(unsigned int)custom_time[4]); //第一行显示洗涤时间设置
			//sprintf(seg_sring_det,"%d %d %d %d",(unsigned int)custom_time[4],(unsigned int)custom_time[0],(unsigned int)custom_time[1],(unsigned int)custom_num); //调试
			sprintf(seg_sring_tasks,"S13:Ack S14:Esc");
		break;
		
		case 0x40:
			sprintf(seg_sring_det,"Rinse_set:%d",(unsigned int)custom_time[4]); //第一行显示漂洗时间设置
			//sprintf(seg_sring_det,"%d %d %d %d",(unsigned int)custom_time[4],(unsigned int)custom_time[0],(unsigned int)custom_time[1],(unsigned int)custom_num); //调试
			sprintf(seg_sring_tasks,"S13:Ack S14:Esc");
		break;
		
		case 0x50:
			sprintf(seg_sring_det,"Dehy_set:%d",(unsigned int)custom_time[4]); //第一行显示漂洗时间设置
			//sprintf(seg_sring_det,"%d %d %d %d",(unsigned int)custom_time[4],(unsigned int)custom_time[0],(unsigned int)custom_time[1],(unsigned int)custom_num); //调试
			sprintf(seg_sring_tasks,"S13:Ack S14:Esc");
		break;
		
		
		default:
			sprintf(seg_sring_det,"       "); 
			sprintf(seg_sring_tasks,"       ");
		break;
	}
	lcd1602_show_string(0,0,seg_sring_det);//第一行显示
	lcd1602_show_string(0,1,seg_sring_tasks);//第二行显示
}


