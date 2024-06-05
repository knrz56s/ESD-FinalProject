// �ǿ�t�v9600	����11.0592MHz
// port �����ӴN�����

/*
���u�G
UART�GESP32-16, 17�}��8051-P3^0, P3^1
LED�G8051 P3^4, P3^5, P3^6, P3^7 �� 4 �� LED
���s�G8051 P1 �� J26-K1~K8
�n���G8051 P2^6 �� J42-B1
�C�q��ܾ��G8051 P0�� J3 | P2^2, P2^3 �� J2-B, A

�C�q��ܾ��G
�Y�W�L8�Ӧr���A�|��ܫe7��+�̫�@�ӡA���׿�J�α������T��

���s�W�h�G
���U���uK1��J�����K�X
��J���@�Ӧr�������UK4
�S��K4�|�Q���L�B�S���������K�X���]�|�Q�ťը��N(a-z0-9)
���y�ܻ��N�O�n�ťմN������K4
���UK8�e�X
�̫�@�Ӧr���S���UK4�|�Q���L
*/

//�]�t���Y�ɡA�@�뱡�p���ݭn��ʡA���Y�ɥ]�t�S��\��H�s�����w�q 
#include <reg52.h> 
#include <string.h>                       
#define MAX 30
#define DataPort P0
#define KeyPort  P1

/*P3^0, P3^1 : Rx, Tx*/

sbit LED1 = P3^4;
sbit LED2 = P3^5;
sbit LED3 = P3^6;
sbit LED4 = P3^7;

sbit SPK = P2^6;  //�w�q���ֺݿ�X���f
sbit LATCH1 = P2^2;
sbit LATCH2 = P2^3;

unsigned char code WeiMa[]={0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};

unsigned char code alphabet[]={
								0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71,
							 	0x3D, 0x76, 0x0F, 0x0E, 0x75, 0x38,
							 	0x37, 0x54, 0x5C, 0x73, 0x67, 0x31,
							 	0x49, 0x78, 0x3E, 0x1C, 0x7E, 0x64,
							 	0x6E, 0x5A, 0x3F, 0x06, 0x5B, 0x4F,
							 	0x66, 0x6D, 0x7D, 0x27, 0x7F, 0x6F,
							  }; // a-z0-9

unsigned char code morse[36][6]={
    "dD", "Dddd", "DdDd", "Ddd", "d", "ddDd",				// A, B, C, D, E, F
    "DDd", "dddd", "dd", "dDDD", "DdD", "dDdd",				// G, H, I, J, K, L
    "DD", "Dd", "DDD", "dDDd", "DDdD", "dDd",				// M, N, O, P, Q, R
    "ddd", "D", "ddD", "dddD", "dDD", "DddD",				// S, T, U, V, W, X
    "DdDD", "DDdd", "dDDDD", "ddDDD", "dddDD", "ddddD",		// Y, Z, 0, 1, 2, 3
    "ddddd", "Ddddd", "DDddd", "DDDdd", "DDDDd", "DDDDD"	// 4, 5, 6, 7, 8, 9
};

unsigned char TempData[MAX]; // ��ܦb�C�q��ܾ��W

typedef unsigned char byte;
typedef unsigned int  word;

byte buf[MAX]; // Receive Buffer
byte head = 0, rcv_idx = 0;
byte inputs[MAX]; // Input character buffer
byte input_idx = 0;

byte get_0d = 0;
byte rec_flag = 0;
byte was_rec_flag = 0;

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
unsigned char KeyScanEight(void);
void ShowReceivedSymbol(void);
void alert(void);
/*------------------------------------------------
                    �D���
------------------------------------------------*/
void main (void){
	unsigned char k, ky, idx, bfr_idx; // indexes
	unsigned char bfr[6]; // Code Buffer

	// Initialization
	InitUART();
	Init_Timer0();
	//Init_Timer1(); // �}�F�|�z��

	SendStr("System launched. -w-\n");

	idx = bfr_idx = 0;
	ES = 1;                  //���}��f���_


	while (1){
		if (rec_flag == 1){ // �n����CRLF�~�|�O1
			buf[head] = '\0';
			rec_flag = 0;
			head = 0;
			rcv_idx = 0;
			
			// Show a symbol
			ShowReceivedSymbol();		

			while(buf[head] != '\0'){
				if((buf[head] - 'a') >= 0 && (buf[head] - 'a') <= 25) // a-z
					TempData[rcv_idx] = alphabet[buf[head] - 'a'];
				else if((buf[head] - '0') >= 0 && (buf[head] - '0') <= 9) // 0-9
					TempData[rcv_idx] = alphabet[(buf[head] - '0') + 26];
				else
					TempData[rcv_idx] = 0;

				rcv_idx += 1;
				if(rcv_idx >= 8) rcv_idx = 7;
				head += 1;
			}

			// If received 'SOS'
			if(buf[0] == 's' && buf[1] == 'o' && buf[2] == 's')
				alert();

			// reset
			head = rcv_idx = 0;

			// Record
			was_rec_flag = 1;

		}else{ // �S����T���N�i�H����L�ơG
		
			ky = KeyScanEight();

			if(ky != 0xff && was_rec_flag){ // button pressed -> clean received message
				was_rec_flag = 0;
				TempData[0] = TempData[1] = TempData[2] = TempData[3] = 0;
				TempData[4] = TempData[5] = TempData[6] = TempData[7] = 0;
			}
	
			if(ky == 8){ // Send UART

				// Add terminator and send
				inputs[input_idx] = '\0';
				SendStr(inputs);
				SendStr("\r\n"); // not necessary
	
				// Clean Input
				for(input_idx = 0; input_idx < MAX; input_idx++)
					inputs[input_idx] = 0;

				// Reset indexes
				bfr_idx = input_idx = 0;

				// Clean demonstrate characters
				TempData[0] = TempData[1] = TempData[2] = TempData[3] = 0;
				TempData[4] = TempData[5] = TempData[6] = TempData[7] = 0;

			}else if(ky == 4){ // Finish a character
				
				// Add terminator
				bfr[bfr_idx] = '\0';
 		    	
				// Morse code to alphabet
				for(idx = 0; idx < 36; idx++){
					if(strcmp(bfr, morse[idx]) == 0){
						TempData[input_idx % 8] = alphabet[idx]; // Looply display
						if(idx < 26) // a-z
							inputs[input_idx] = 'a' + idx;
						else // 0-9
							inputs[input_idx] = '0' + (idx - 26);
						break;
					}
				}

				// If not in a-z0-9 or not a normal character -> space replace
				if(idx == 36)
					inputs[input_idx] = ' ';

				// Avoid index OutOfBound
				input_idx = (input_idx >= MAX) ? MAX - 1 : input_idx + 1;
	
				// Clean morse code buffer
				for(bfr_idx = 0; bfr_idx < 6; bfr_idx++)
					bfr[bfr_idx] = 0;
				bfr_idx = 0;

			}else if(ky >> 1 == 0){ // ky == (0: dot) or (1: dash)
				
				if(ky == 0){ // Dot
					bfr[bfr_idx] = 'd';
				}else if(ky == 1){ // Dash
					bfr[bfr_idx] = 'D';
				}
				
				// Avoid index OutOfBound
				bfr_idx = (bfr_idx == 5) ? 5 : bfr_idx + 1;

				// Check dot or dash
				//TempData[7] = (bfr[bfr_idx - 1] == 'd') ? alphabet[32] : alphabet[35];
			}
		
		}
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
		if (get_0d == 0){ // �٨S����0x0d
			if (tmp == 0x0d) get_0d = 1; // ����F 0x0d
			else{ // �N��Ƽg�J buffer
				buf[head]=tmp;              
				head++;
				if (head == MAX) head = 0;	
			}				     
		}else if (get_0d == 1){
			if (tmp != 0x0a){ // �Y 0x0d �ᤣ�O 0x0a ����ƥ������n
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
void Timer1_isr(void) interrupt 3{

	TH1=(65536-2000)/256;		  //���s��� 2ms
	TL1=(65536-2000)%256;	

	// �������h�[��
	/*
	TempData[0] = alphabet[26 + len/1000];
	TempData[1] = alphabet[26 + (len/100)%10];
	TempData[2] = alphabet[26 + (len/10)%10];
	TempData[3] = alphabet[26 + len%10];
	*/
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
���䱽�y��ơA��^���y���
------------------------------------------------*/
unsigned char KeyScanEight(void){
	unsigned char keyvalue;
	unsigned int len = 0;
	if(KeyPort!=0xff){
		DelayMs(10);
		if(KeyPort!=0xff){
			keyvalue = KeyPort;

			LED1 = LED2 = LED3 = LED4 = 0; // �G��
		    while(KeyPort != 0xff){ // ����o�X�n��
				SPK = !SPK;
				DelayMs(1);

				len++;
				if(len > 10000)len = 9999;

			}			
			LED1 = LED2 = LED3 = LED4 = 1; // ����

			if(keyvalue == 0x7f) return 8; // ���U K8
			if(keyvalue == 0xf7) return 4; // ���U K4
			return (len > 300) ? 1 : 0; // ���ի�Pı���n
		}
	}
	return 0xff;
}


void ShowReceivedSymbol(void){
	unsigned char times, i, j;
	i = j = times = 0;
	
	// Make sound twice
	for(i = 0; i < 2; i++){
		while(j < 200){
			SPK = !SPK;
			DelayMs(1);
			j++;
		}
		DelayMs(200);
		j = 0;
	}

	for(times = 0; times < 2; times++){
		
		// Clean current demonstrate characters
		TempData[0] = TempData[1] = TempData[2] = TempData[3] = 0;
		TempData[4] = TempData[5] = TempData[6] = TempData[7] = 0;
		DelayMs(500);

		TempData[0] = alphabet[17]; // r
		TempData[1] = alphabet[4];  // e
		TempData[2] = alphabet[2];  // c
		TempData[3] = alphabet[4];  // e
		TempData[4] = alphabet[8];  // i
		TempData[5] = alphabet[21]; // v
		TempData[6] = alphabet[4];  // e
		TempData[7] = alphabet[3];  // d
		DelayMs(500);		
	}

	// Clean current demonstrate characters
	TempData[0] = TempData[1] = TempData[2] = TempData[3] = 0;
	TempData[4] = TempData[5] = TempData[6] = TempData[7] = 0;
	DelayMs(500);
}

void alert(void){
	unsigned char k, i, j;
	i = j = k = 0;
	
	// Make sound three times * 3
	for(i = 0; i < 3; i++){
		LED1 = LED2 = LED3 = LED4 = 0;
		for(k = 0; k < 3; k++){
			while(j < 200){
				SPK = !SPK;
				DelayMs(1);
				j++;
			}
			DelayMs(25);
			j = 0;
		}
		LED1 = LED2 = LED3 = LED4 = 1;
		j = 0;
		DelayMs(400);
	}
}
