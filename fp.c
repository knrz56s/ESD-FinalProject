// �ǿ�t�v9600	����11.0592MHz

//�]�t���Y�ɡA�@�뱡�p���ݭn��ʡA���Y�ɥ]�t�S��\��H�s�����w�q 
#include <reg52.h> 
#include <string.h>                       
#define MAX 30
#define DataPort P0
#define KeyPort  P1

/*P3^0, P3^1 : Rx, Tx*/

sbit LED1=P3^4;
sbit LED2=P3^5;
sbit LED3=P3^6;
sbit LED4=P3^7;

sbit SPK=P2^6;  //�w�q���ֺݿ�X���f
sbit LATCH1 = P2^2;
sbit LATCH2 = P2^3;

unsigned char code WeiMa[]={0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};

unsigned char code alphabet[]={
								0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71,
							 	0x7D, 0x76, 0x06, 0x1E, 0x7A, 0x38,
							 	0x57, 0x37, 0x3F, 0x73, 0x6F, 0x73,
							 	0x6D, 0x78, 0x3E, 0x3E, 0x35, 0x77,
							 	0x6E, 0x5B, 0x3F, 0x06, 0x5B, 0x4F,
							 	0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,
							  }; // a-z0-9

unsigned char code morse[36][6]={
    "dD", "Dddd", "DdDd", "Ddd", "d", "ddDd",
    "DDd", "dddd", "dd", "dDDD", "DdD", "dDdd",
    "DD", "Dd", "DDD", "dDDd", "DDdD", "dDd",
    "ddd", "D", "ddD", "dddD", "dDD", "DddD",
    "DdDD", "DDdd", "dDDDD", "ddDDD", "dddDD", "ddddD",
    "ddddd", "Ddddd", "DDddd", "DDDdd", "DDDDd", "DDDDD"
};

unsigned char TempData[MAX]; // ��ܦb�C�q��ܾ��W

typedef unsigned char byte;
typedef unsigned int  word;

byte buf[MAX]; // Receive Buffer
byte head = 0;

byte get_0d = 0;
byte rec_flag = 0;

//unsigned char Timer0_H,Timer0_L,Time;



/*------------------------------------------------
                   �禡�ŧi
------------------------------------------------*/
void SendStr(unsigned char *s);
void InitUART(void);
void Init_Timer0(void);
void Init_Timer1(void);
void DelayUs2x(unsigned char t);
void DelayMs(unsigned char t);
void Display(unsigned char FirstBit,unsigned char Num);
//unsigned char KeyScanSixteen(void);
//unsigned char KeyPro(void);
unsigned char KeyScanEight(void);

/*------------------------------------------------
                    �D���
------------------------------------------------*/
void main (void){
	//word i;
	unsigned char k, ky, idx, bfr_idx, input_idx; // indexes
	unsigned char bfr[6]; // Code Buffer
	unsigned char inputs[MAX]; // Input character buffer

	// Initialization
	//InitUART();
	Init_Timer0();
	Init_Timer1();

	idx = bfr_idx = input_idx = 0;
	//ES = 1;                  //���}��f���_

	while (1){
		/*if (rec_flag == 1){ // �n����CRLF�~�|�O1
			buf[head] = '\0';
			rec_flag = 0;
			head = 0;
			
			while(buf[head] != '\0'){
				k = ((buf[head] - '0') - 1) + 7;
				//Timer0_H = FREQH[k];
				//Timer0_L = FREQL[k];
				//Song();
				head += 1;
			}

			head = 0; // reset
		}else{ // �S����T���N�i�H����L�ơG*/

			ky = KeyScanEight();
	
			if(ky == 8){ // Send UART
				inputs[input_idx] = '\0';
				SendStr(inputs);
	
				// Clean Input
				for(input_idx = 0; input_idx < MAX; input_idx++) inputs[input_idx] = 0;
				input_idx = 0;
			}else if(ky == 4){ // Finish a character
				
				bfr[bfr_idx] = '\0';
 		    	
				// Code to Alphabet
				for(idx = 0; idx < 36; idx++){
					if(strcmp(bfr, morse[idx]) == 0){
						TempData[input_idx] = alphabet[idx];
						if(idx < 26) // A-Z
							inputs[input_idx] = 'a' + idx;
						else
							inputs[input_idx] = '0' + (idx - 26);
						break;
					}
				}
				input_idx++;
	
				// Clean morse code buffer
				for(bfr_idx = 0; bfr_idx < 6; bfr_idx++) bfr[bfr_idx] = 0;
				bfr_idx = 0;
			}else if(ky >> 1 == 0){ // ky == 0 or 1
				if(ky == 0){ // Dot
					bfr[bfr_idx] = 'd';
				}else if(ky == 1){ // Dash
					bfr[bfr_idx] = 'D';
				}

				bfr_idx = (bfr_idx == 5) ? 5 : bfr_idx + 1;

				// Check dot or dash
				if(bfr[bfr_idx - 1] == 'd') TempData[7] = alphabet[29];
				else TempData[7] = alphabet[30];
			}

		//}
    }
}

/*------------------------------------------------
                    �o�e�@�Ӧ줸��
------------------------------------------------*/
void SendByte(unsigned char dat){
	SBUF = dat;
 	while(!TI);
    TI = 0;
}
/*------------------------------------------------
                    �o�e�@�Ӧr��
------------------------------------------------*/
void SendStr(unsigned char *s){
 	while(*s!='\0'){// \0 ��ܦr�굲���лx�A�q�L�˴��O�_�r�꥽��
		SendByte(*s);
  		s++;
  	}
}
/*------------------------------------------------
                     ��f���_�{��
------------------------------------------------*/
void UART_SER (void) interrupt 4{ //��C���_�A�ȵ{��
	unsigned char tmp;          //�w�q�{���ܼ� 
   
   	if(RI){ //�P�_�O�������_����
		RI=0;                      //�лx�줸�M�s
		tmp=SBUF;                 //Ū�J�w�İϪ���
		if (get_0d == 0){
			if (tmp == 0x0d) get_0d = 1;
			else{
				buf[head]=tmp;              
				head++;
				if (head == MAX) head = 0;	
			}				     
		}else if (get_0d == 1){
			if (tmp != 0x0a){
				head = 0;
				get_0d = 0;		
			}else{
				rec_flag = 1;
				get_0d = 0;
			}
		}
		//	SBUF=tmp;                 //�Ⱶ���쪺�ȦA�o�^�q����
	 }
//   if(TI)                        //�p�G�O�o�e�лx�줸�A�M�s
//     TI=0;
}
/*------------------------------------------------
                    ��f��l��
------------------------------------------------*/
void InitUART  (void){

    SCON  = 0x50;		        // SCON: �Ҧ� 1, 8-bit UART, �ϯ౵��  
    TMOD |= 0x20;               // TMOD: timer 1, mode 2, 8-bit ����
    TH1   = 0xFD;             // TH1:  ���˭� 9600 ��C�ǿ�t�v ���� 11.0592MHz  
    TR1   = 1;                  // TR1:  timer 1 ���}                         
    EA    = 1;                  //���}�`���_
   // ES    = 1;                  //���}��f���_
}
/*------------------------------------------------
                    �p�ɾ���l�ưƵ{��
------------------------------------------------*/
void Init_Timer0(void){
	TMOD |= 0x01;	  //�ϥμҦ�1�A16�줸�p�ɾ��A�ϥ�"|"�Ÿ��i�H�b�ϥΦh�ӭp�ɾ��ɤ����v�T		     
	//TH0=0x00;	      //���w���
	//TL0=0x00;
	EA=1;            //�`���_���}
	ET0=1;           //�p�ɾ����_���}
	TR0=1;           //�p�ɾ��}�����}
}
void Init_Timer1(void){
	TMOD |= 0x10;	  //�ϥμҦ�1�A16�줸�p�ɾ��A�ϥ�"|"�Ÿ��i�H�b�ϥΦh�ӭp�ɾ��ɤ����v�T		     
	//TH1=0x00;	      //���w���
	//TL1=0x00;
	EA=1;            //�`���_���}
	ET1=1;           //�p�ɾ����_���}
	TR1=1;           //�p�ɾ��}�����}
}
/*------------------------------------------------
                 �p�ɾ����_�Ƶ{��
------------------------------------------------*/
void Timer0_isr(void) interrupt 1{	// �C�q��ܾ���ܦr��

	TH0=(65536-2000)/256;		  //���s��� 2ms
	TL0=(65536-2000)%256;

	Display(0, 8); // �եμƽX�ޱ��y
}
void Timer1_isr(void) interrupt 3{ // ���y����x

	
/*	TempData[0] = alphabet[3];
	TempData[1] = alphabet[4];
	TempData[2] = alphabet[11];
	TempData[3] = alphabet[19];
	TempData[4] = alphabet[0];*/

	//unsigned char ky;	

	TH1=(65536-2000)/256;		  //���s��� 2ms
	TL1=(65536-2000)%256;	
	
}
/*------------------------------------------------
 uS���ɨ�ơA�t����J�Ѽ� unsigned char t�A�L��^��
 unsigned char �O�w�q�L�Ÿ��r���ܶq�A��Ȫ��d��O
 0~255 �o�̨ϥδ���12M�A��T���ɽШϥηJ�s,�j�P����
 ���צp�U T=tx2+5 uS 
------------------------------------------------*/
void DelayUs2x(unsigned char t){   
	while(--t);
}
/*------------------------------------------------
 mS���ɨ�ơA�t����J�Ѽ� unsigned char t�A�L��^��
 unsigned char �O�w�q�L�Ÿ��r���ܶq�A��Ȫ��d��O
 0~255 �o�̨ϥδ���12M�A��T���ɽШϥηJ�s
------------------------------------------------*/
void DelayMs(unsigned char t){
	while(t--){
		//�j�P����1mS
		DelayUs2x(245);
		DelayUs2x(245);
	}
}
/*------------------------------------------------
 ��ܨ�ơA�Ω�ʺA���˼ƽX��
 ��J�Ѽ� FirstBit ��ܻݭn��ܪ��Ĥ@��A�p���2��ܱq�ĤT�ӼƽX�޶}�l���
 �p��J0��ܱq�Ĥ@����ܡC
 Num��ܻݭn��ܪ��줸�ơA�p�ݭn���99��줸�ƭȫh�ӭȿ�J2
------------------------------------------------*/
void Display(unsigned char FirstBit,unsigned char Num){
	//�C2ms��ܤ@�ӼƦr�A�]���ӧ֩ҥH����ı�ȯd
	static unsigned char i=0;
	
	
	DataPort=0;   //�M�Ÿ�ơA���������v
	LATCH1=1;     //�q��s
	LATCH1=0;
	
	DataPort=WeiMa[i+FirstBit]; //����X 
	LATCH2=1;     //����s
	LATCH2=0;
	
	DataPort=TempData[i]; //����ܸ�ơA�q�X
	LATCH1=1;     //�q��s
	LATCH1=0;
	
	i++;
	if(i==Num) i=0;
}
/*------------------------------------------------
        ���䱽�˨�ơA��^�������
------------------------------------------------*/
/*unsigned char KeyScanSixteen(void){  //��L���˨�ơA�ϥΦ�C�v�ű��˪k
	unsigned char Val;
	KeyPort=0xf0;//���|��m���A�C�|��ԧC
	if(KeyPort!=0xf0){ //��ܦ�������U
		DelayMs(10);  //�h��
		if(KeyPort!=0xf0){ //��ܦ�������U
			KeyPort=0xfe; //�˴��Ĥ@��
			if(KeyPort!=0xfe){
				Val=KeyPort&0xf0;
		  	    Val+=0x0e;
		  		while(KeyPort!=0xfe);
				DelayMs(10); //�h��
				while(KeyPort!=0xfe);
		     	return Val;
		    }
		    KeyPort=0xfd; //�˴��ĤG��
			if(KeyPort!=0xfd){
				Val=KeyPort&0xf0;
	  	    	Val+=0x0d;
	  			while(KeyPort!=0xfd);
				DelayMs(10); //�h��
				while(KeyPort!=0xfd);
	     		return Val;
		    }
			KeyPort=0xfb; //�˴��ĤT��
			if(KeyPort!=0xfb){
				Val=KeyPort&0xf0;
		  	    Val+=0x0b;
		  		while(KeyPort!=0xfb);
				DelayMs(10); //�h��
				while(KeyPort!=0xfb);
		     	return Val;
		    }
			KeyPort=0xf7; //�˴��ĥ|��
			if(KeyPort!=0xf7){
				Val=KeyPort&0xf0;
		  	    Val+=0x07;
		  		while(KeyPort!=0xf7);
				DelayMs(10); //�h��
				while(KeyPort!=0xf7);
		     	return Val;
		    }
		}
	}
	return 0xff;
}*/
/*------------------------------------------------
         ����ȳB�z��ơA��^�����
------------------------------------------------*/
/*unsigned char KeyPro(void){
	switch(KeyScanSixteen()){
		case 0x7e:return 0;break;//0 ���U����������ܬ۹������X��
		case 0x7d:return 1;break;//1
		case 0x7b:return 2;break;//2
		case 0x77:return 3;break;//3
		case 0xbe:return 4;break;//4
		case 0xbd:return 5;break;//5
		case 0xbb:return 6;break;//6
		case 0xb7:return 7;break;//7
		case 0xde:return 8;break;//8
		case 0xdd:return 9;break;//9
		case 0xdb:return 10;break;//a
		case 0xd7:return 11;break;//b
		case 0xee:return 12;break;//c
		case 0xed:return 13;break;//d
		case 0xeb:return 14;break;//e
		case 0xe7:return 15;break;//f
		default:return 0xff;break;
	}
}*/
/*------------------------------------------------
���䱽�y��ơA��^���y���
------------------------------------------------*/
unsigned char KeyScanEight(void){
	unsigned char keyvalue;
	unsigned char len = 0;
	if(KeyPort!=0xff){
		DelayMs(10);
		if(KeyPort!=0xff){
			keyvalue = KeyPort;

			LED1 = LED2 = LED3 = LED4 = 1;
		    while(KeyPort != 0xff){ // ����o�X�n��
				SPK = !SPK;
				DelayMs(1);

				len++;
				if(len > 10000)len = 9999;

			}			
			LED1 = LED2 = LED3 = LED4 = 0;

			if(keyvalue == 0x7f) return 8;
			if(keyvalue == 0xf7) return 4;
			return len > 800 ? 1 : 0; // �̷Pı


			/*switch(keyvalue){
				case 0xfe:
					return 1;break;
				case 0xfd:
					return 2;break;
				case 0xfb:
					return 3;break;
				case 0xf7:
					return 4;break;
				case 0xef:
					return 5;break;
				case 0xdf:
					return 6;break;
				case 0xbf:
					return 7;break;
				case 0x7f:
					return 8;break;
				default:
					return 0;break;
			}*/
		}
	}
	return 0xff;
}
