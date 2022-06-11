#include "STDIO.H"
#include "REG52.H"
#include "motor.h"
#include "bsp_key.h"
#include "delay.h"
#include "lcd1602.h"
#include "bsp_time.h"
#include "24c02.h"


//-----��Ҫ����
void key_proc(void);
void seg_proc(void);
//-----��ʱ������
long time_tick=0; 
unsigned int key_delay;
unsigned int seg_delay;
unsigned int motor_delay;
unsigned int count;//����ʱ��
unsigned int count_down;//ÿ�����̵ĵ���ʱ
xdata unsigned int Start_Time;
//------1602����
unsigned char seg_sring_det[17]; //��һ�б���
unsigned char seg_sring_tasks[17];//�ڶ��б���
//------��������
unsigned char key_val;
unsigned char key_old,key_down;
//------�������
xdata unsigned char step=0;
xdata unsigned char dir=0;//�������
xdata unsigned int motor_num;//��ʾ�Ƿ���Ҫ�ı�һ�η���
//-----ȫ�ֱ�������
unsigned char key_num=0;
unsigned char pattern;//ģʽѡ��
unsigned char interface=0x01;//����ѡ��
xdata unsigned char WS_PT[4][4]={{5,15,7,7},{5,30,15,15},{5,40,20,20},{5,5,5,5}}; //ϴ�� Ưϴ ��ˮ  ��ʱʱ���ʼ��
unsigned char WS_PT_4[3];
unsigned int Process=0;//0��  1��ˮ 2ϴ�� 3Ưϴ 4��ˮ 
bit flag;//1 ������ɱ�����
bit Refresh_flag;//������9��99ʱֻˢ��һ��
unsigned char custom_time[5];//�Զ����ʱ��
unsigned char custom_num;//�����λ��
bit custom_flag;//��ʾ����ȥ��
/*******************************************************************************
* �� �� ��       : main
* ��������		 : ������
* ��    ��       : ��
* ��    ��    	 : ��
*******************************************************************************/
void main()
{	
	lcd1602_init();//LCD1602��ʼ��
	time0_init();
	WS_PT[3][1] = at24c02_read_one_byte(1); //1ϴ��ʱ�� 2Ưϴʱ�� 3��ˮʱ��
	delay_ms(10);
	WS_PT[3][2] = at24c02_read_one_byte(2); //1ϴ��ʱ�� 2Ưϴʱ�� 3��ˮʱ��
	delay_ms(10);
	WS_PT[3][3] = at24c02_read_one_byte(3); //1ϴ��ʱ�� 2Ưϴʱ�� 3��ˮʱ��
	delay_ms(10);
	while(1)
	{			
		key_proc();	
		seg_proc();		
	}		
}



/***********************��ʱ��0�жϺ���(1ms)***********************/
void time0() interrupt 1 
{
	time_tick ++;
	TH0=0XFC;	//����ʱ������ֵ����ʱ1ms
	TL0=0X18;
	
	if(key_delay)//ֻ�в�����0��ʱ����Լ�
		if(++key_delay == 6) key_delay = 0;
	
	if(seg_delay)//ֻ�в�����0��ʱ����Լ�
		if(++seg_delay == 300) seg_delay = 0;
	
	if(!(time_tick % 1000))
		count++;//����ʱ��
}

/***********************��������***********************/
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
	//����  0x01:ѡ��ϴ��ģʽ  0x02:ϴ��ģʽȷ��  0x03:��ˮ״̬ 
	//�Զ���ʱ����� 0x10:�Զ���ģʽȷ��  0x20:ʹ����һ�ε�����ֵ������������  0x30:����ϴ��ʱ��
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
				interface = 0x20;//0x20:ʹ����һ�ε�����ֵ������������
				lcd1602_clear();
				break; //�����ŵ�����
			}
			
			if(interface == 0x02)
			{
				interface = 0x03;
				lcd1602_clear();
				Process = 1;//��ʾ�����ˮģʽ
				Start_Time = count;//�ṩ��ʼ��ʱ��
				seg_sring_tasks[0] = '\0';//�������
				flag = 0;//��ʾ����δ���
			}
			
			if(interface == 0x20)
			{
				pattern = 4; //���ĸ�
				Process = 1;//��ʾ�����ˮģʽ
				Start_Time = count;//�ṩ��ʼ��ʱ��
				seg_sring_tasks[0] = '\0';//�������
				flag = 0;//��ʾ����δ���
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
				interface = 0x01;//���¿�ʼ
				lcd1602_clear();
			}
			if(interface == 0x20)
			{
				interface = 0x30;//����ϴ��ʱ��
				custom_time[0] = 0;
				custom_time[1] = 0;
				custom_time[4] = 0;
			}
		break;
			
		case 13: //0x30:����ϴ��ʱ��  
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
				pattern = 4; //���ĸ�
				Process = 1;//��ʾ�����ˮģʽ
				Start_Time = count;//�ṩ��ʼ��ʱ��
				seg_sring_tasks[0] = '\0';//�������
				flag = 0;//��ʾ����δ���
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
	//speed�ٶ�ֵԽС�ٶ�Խ��(1-5)  dir����(0��ʱ�� ����˳ʱ��)
		if((Process == 0) || (interface == 0x07))//�� ���߽���
			return;//����
		else if(Process == 1)
			step_motor_send_pulse(step++,0);	
		else if((Process == 4) || (Process == 2) || (Process == 3))
			step_motor_send_pulse(step++,dir);	
			if(step==8)step=0;
}

/***********************1602����***********************/
void seg_proc(void)
{
	if(seg_delay) return;
		else seg_delay = 1;
	if(flag)//����ɹ���ʱ
	{
		lcd1602_clear();//����
		Process++;//���ֻ��4
		interface++;//���ֻ��0x06
		Start_Time = count;//�ṩ��ʼ��ʱ��
		seg_sring_tasks[0] = '\0';//�������
		seg_sring_tasks[1] = '\0';//�������
		flag = 0;//��ʾ����δ���
	}
	count_down = WS_PT[(unsigned int)pattern-1][Process-1]-(count-Start_Time);
	if(((count_down == 9 )|| (count_down == 99)) && (!Refresh_flag))
	{
		lcd1602_clear();//����
		Refresh_flag = 1;
	}
	
	if(!(count_down == 9 )|| (count_down == 99))
		Refresh_flag = 0;//����9��99ʱ��־λΪ0
	
	if((custom_flag) && ((interface == 0x30) || (interface == 0x40) || (interface == 0x50)))
	{
		custom_time[custom_num] = key_num;
		if(++custom_num == 3) custom_num =2;//ֻ��������λ��
		lcd1602_clear();
	} 
	if(custom_num == 0)
		custom_time[4] = 0;//����λ��ʾ�����ʱ��
	else if(custom_num == 1)
		custom_time[4] = custom_time[0];//����λ��ʾ�����ʱ��
	else if(custom_num == 2)
		custom_time[4] = (custom_time[0] * 10) + custom_time[1];//����λ��ʾ�����ʱ��
		custom_flag = 0;
	
	//����  0x01:ѡ��ϴ��ģʽ  0x02:ϴ��ģʽȷ��  0x03:��ˮ״̬  0x04:ϴ��  0x05:Ưϴ  0x06��ˮ
	//�Զ���ʱ����� 0x10:�Զ���ģʽȷ��  0x20:ʹ����һ�ε�����ֵ������������  0x30:����ϴ��ʱ��
	switch(interface)
  {
		case 0x01:  
			sprintf(seg_sring_det,"Select Mode:"); //��һ����ʾ��ѡ��ģʽ
			//sprintf(seg_sring_det,"%d",(unsigned int)key_num);//����
			sprintf(seg_sring_tasks,"%d",(unsigned int)count);
		break;
		
		//��������ͨģʽ����
		case 0x02:
			sprintf(seg_sring_det,"Select Mode:%d?",(unsigned int)pattern); //��һ����ʾѡ���ģʽ
			sprintf(seg_sring_tasks,"S15:ACK S16:NACK");
		break;
		
		case 0x03://��ˮ
			sprintf(seg_sring_det,"Add water: %d",(unsigned int)count_down); //��һ����ʾ���ڽ�ˮ
			//sprintf(seg_sring_det,"%d %d  %d",Start_Time,count,(unsigned int)WS_PT[(unsigned int)pattern-1][Process-1]); //����
			//��ʼ��ʱ��  �����ʱ��  ���ڵ�ʱ�� ���������
			Progress_Bar(Start_Time,(float)WS_PT[(unsigned int)pattern-1][Process-1],count,seg_sring_tasks);//������
		break;
		
		case 0x04://ϴ��
			sprintf(seg_sring_det,"Washing:%d %d",(unsigned int)count_down,(unsigned int)dir); //��һ����ʾ����ϴ��
			//sprintf(seg_sring_det,"%d %d  %d %d",Start_Time,count,(unsigned int)WS_PT[(unsigned int)pattern-1][Process-1],Process); //����
			Progress_Bar(Start_Time,(float)WS_PT[(unsigned int)pattern-1][Process-1],count,seg_sring_tasks);//������
		break;
		
		case 0x05://Ưϴ
			sprintf(seg_sring_det,"Rinse:%d",(unsigned int)count_down); //��һ����ʾ����Ưϴ
			//sprintf(seg_sring_det,"%d %d  %d",Start_Time,count,(unsigned int)WS_PT[(unsigned int)pattern-1][Process-1]); //����
			Progress_Bar(Start_Time,(float)WS_PT[(unsigned int)pattern-1][Process-1],count,seg_sring_tasks);//������
		break;
		
		case 0x06://��ˮ
			sprintf(seg_sring_det,"Dehydration:%d",(unsigned int)count_down); //��һ����ʾ������ˮ
		//sprintf(seg_sring_det,"%d %d  %d",Start_Time,count,(unsigned int)WS_PT[(unsigned int)pattern-1][Process-1]); //����
			Progress_Bar(Start_Time,(float)WS_PT[(unsigned int)pattern-1][Process-1],count,seg_sring_tasks);//������
		break;
		
		case 0x07://���
			sprintf(seg_sring_det,"Complete!!!"); //��һ����ʾ���
			//sprintf(seg_sring_det,"%d %d  %d",Start_Time,count,(unsigned int)WS_PT[(unsigned int)pattern-1][Process-1]); //����
			sprintf(seg_sring_tasks,"S16:Res"); //��һ����ʾ���
		break;
		
		
		//�������Զ���ģʽ����
		case 0x10:
			sprintf(seg_sring_det,"Custom mode?"); //��һ����ʾ�Ƿ�����Զ���ģʽ
			sprintf(seg_sring_tasks,"S15:ACK S16:NACK");
		break;
		
		case 0x20:
			sprintf(seg_sring_det,"S15:Last"); //��һ����ʾ�Ƿ�����Զ���ģʽ
			sprintf(seg_sring_tasks,"S16:Reset");
		break;
		
		case 0x30:
			sprintf(seg_sring_det,"Washing_set:%d",(unsigned int)custom_time[4]); //��һ����ʾϴ��ʱ������
			//sprintf(seg_sring_det,"%d %d %d %d",(unsigned int)custom_time[4],(unsigned int)custom_time[0],(unsigned int)custom_time[1],(unsigned int)custom_num); //����
			sprintf(seg_sring_tasks,"S13:Ack S14:Esc");
		break;
		
		case 0x40:
			sprintf(seg_sring_det,"Rinse_set:%d",(unsigned int)custom_time[4]); //��һ����ʾƯϴʱ������
			//sprintf(seg_sring_det,"%d %d %d %d",(unsigned int)custom_time[4],(unsigned int)custom_time[0],(unsigned int)custom_time[1],(unsigned int)custom_num); //����
			sprintf(seg_sring_tasks,"S13:Ack S14:Esc");
		break;
		
		case 0x50:
			sprintf(seg_sring_det,"Dehy_set:%d",(unsigned int)custom_time[4]); //��һ����ʾƯϴʱ������
			//sprintf(seg_sring_det,"%d %d %d %d",(unsigned int)custom_time[4],(unsigned int)custom_time[0],(unsigned int)custom_time[1],(unsigned int)custom_num); //����
			sprintf(seg_sring_tasks,"S13:Ack S14:Esc");
		break;
		
		
		default:
			sprintf(seg_sring_det,"       "); 
			sprintf(seg_sring_tasks,"       ");
		break;
	}
	lcd1602_show_string(0,0,seg_sring_det);//��һ����ʾ
	lcd1602_show_string(0,1,seg_sring_tasks);//�ڶ�����ʾ
}


