// 傳輸速率9600	晶振11.0592MHz

//包含標頭檔，一般情況不需要改動，標頭檔包含特殊功能寄存器的定義 
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

sbit SPK=P2^6;  //定義音樂端輸出接口
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

unsigned char TempData[MAX]; // 顯示在七段顯示器上

typedef unsigned char byte;
typedef unsigned int  word;

byte buf[MAX]; // Receive Buffer
byte head = 0;

byte get_0d = 0;
byte rec_flag = 0;

//unsigned char Timer0_H,Timer0_L,Time;



/*------------------------------------------------
                   函式宣告
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
                    主函數
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
	//ES = 1;                  //打開串口中斷

	while (1){
		/*if (rec_flag == 1){ // 要收到CRLF才會是1
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
		}else{ // 沒收到訊息就可以做其他事：*/

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
                    發送一個位元組
------------------------------------------------*/
void SendByte(unsigned char dat){
	SBUF = dat;
 	while(!TI);
    TI = 0;
}
/*------------------------------------------------
                    發送一個字串
------------------------------------------------*/
void SendStr(unsigned char *s){
 	while(*s!='\0'){// \0 表示字串結束標誌，通過檢測是否字串末尾
		SendByte(*s);
  		s++;
  	}
}
/*------------------------------------------------
                     串口中斷程式
------------------------------------------------*/
void UART_SER (void) interrupt 4{ //串列中斷服務程式
	unsigned char tmp;          //定義臨時變數 
   
   	if(RI){ //判斷是接收中斷產生
		RI=0;                      //標誌位元清零
		tmp=SBUF;                 //讀入緩衝區的值
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
		//	SBUF=tmp;                 //把接收到的值再發回電腦端
	 }
//   if(TI)                        //如果是發送標誌位元，清零
//     TI=0;
}
/*------------------------------------------------
                    串口初始化
------------------------------------------------*/
void InitUART  (void){

    SCON  = 0x50;		        // SCON: 模式 1, 8-bit UART, 使能接收  
    TMOD |= 0x20;               // TMOD: timer 1, mode 2, 8-bit 重裝
    TH1   = 0xFD;             // TH1:  重裝值 9600 串列傳輸速率 晶振 11.0592MHz  
    TR1   = 1;                  // TR1:  timer 1 打開                         
    EA    = 1;                  //打開總中斷
   // ES    = 1;                  //打開串口中斷
}
/*------------------------------------------------
                    計時器初始化副程式
------------------------------------------------*/
void Init_Timer0(void){
	TMOD |= 0x01;	  //使用模式1，16位元計時器，使用"|"符號可以在使用多個計時器時不受影響		     
	//TH0=0x00;	      //給定初值
	//TL0=0x00;
	EA=1;            //總中斷打開
	ET0=1;           //計時器中斷打開
	TR0=1;           //計時器開關打開
}
void Init_Timer1(void){
	TMOD |= 0x10;	  //使用模式1，16位元計時器，使用"|"符號可以在使用多個計時器時不受影響		     
	//TH1=0x00;	      //給定初值
	//TL1=0x00;
	EA=1;            //總中斷打開
	ET1=1;           //計時器中斷打開
	TR1=1;           //計時器開關打開
}
/*------------------------------------------------
                 計時器中斷副程式
------------------------------------------------*/
void Timer0_isr(void) interrupt 1{	// 七段顯示器顯示字形

	TH0=(65536-2000)/256;		  //重新賦值 2ms
	TL0=(65536-2000)%256;

	Display(0, 8); // 調用數碼管掃描
}
void Timer1_isr(void) interrupt 3{ // 掃描按鍵x

	
/*	TempData[0] = alphabet[3];
	TempData[1] = alphabet[4];
	TempData[2] = alphabet[11];
	TempData[3] = alphabet[19];
	TempData[4] = alphabet[0];*/

	//unsigned char ky;	

	TH1=(65536-2000)/256;		  //重新賦值 2ms
	TL1=(65536-2000)%256;	
	
}
/*------------------------------------------------
 uS延時函數，含有輸入參數 unsigned char t，無返回值
 unsigned char 是定義無符號字符變量，其值的範圍是
 0~255 這裡使用晶振12M，精確延時請使用彙編,大致延時
 長度如下 T=tx2+5 uS 
------------------------------------------------*/
void DelayUs2x(unsigned char t){   
	while(--t);
}
/*------------------------------------------------
 mS延時函數，含有輸入參數 unsigned char t，無返回值
 unsigned char 是定義無符號字符變量，其值的範圍是
 0~255 這裡使用晶振12M，精確延時請使用彙編
------------------------------------------------*/
void DelayMs(unsigned char t){
	while(t--){
		//大致延時1mS
		DelayUs2x(245);
		DelayUs2x(245);
	}
}
/*------------------------------------------------
 顯示函數，用於動態掃瞄數碼管
 輸入參數 FirstBit 表示需要顯示的第一位，如賦值2表示從第三個數碼管開始顯示
 如輸入0表示從第一個顯示。
 Num表示需要顯示的位元數，如需要顯示99兩位元數值則該值輸入2
------------------------------------------------*/
void Display(unsigned char FirstBit,unsigned char Num){
	//每2ms顯示一個數字，因為太快所以有視覺暫留
	static unsigned char i=0;
	
	
	DataPort=0;   //清空資料，防止有交替重影
	LATCH1=1;     //段鎖存
	LATCH1=0;
	
	DataPort=WeiMa[i+FirstBit]; //取位碼 
	LATCH2=1;     //位鎖存
	LATCH2=0;
	
	DataPort=TempData[i]; //取顯示資料，段碼
	LATCH1=1;     //段鎖存
	LATCH1=0;
	
	i++;
	if(i==Num) i=0;
}
/*------------------------------------------------
        按鍵掃瞄函數，返回掃瞄鍵值
------------------------------------------------*/
/*unsigned char KeyScanSixteen(void){  //鍵盤掃瞄函數，使用行列逐級掃瞄法
	unsigned char Val;
	KeyPort=0xf0;//高四位置高，低四位拉低
	if(KeyPort!=0xf0){ //表示有按鍵按下
		DelayMs(10);  //去抖
		if(KeyPort!=0xf0){ //表示有按鍵按下
			KeyPort=0xfe; //檢測第一行
			if(KeyPort!=0xfe){
				Val=KeyPort&0xf0;
		  	    Val+=0x0e;
		  		while(KeyPort!=0xfe);
				DelayMs(10); //去抖
				while(KeyPort!=0xfe);
		     	return Val;
		    }
		    KeyPort=0xfd; //檢測第二行
			if(KeyPort!=0xfd){
				Val=KeyPort&0xf0;
	  	    	Val+=0x0d;
	  			while(KeyPort!=0xfd);
				DelayMs(10); //去抖
				while(KeyPort!=0xfd);
	     		return Val;
		    }
			KeyPort=0xfb; //檢測第三行
			if(KeyPort!=0xfb){
				Val=KeyPort&0xf0;
		  	    Val+=0x0b;
		  		while(KeyPort!=0xfb);
				DelayMs(10); //去抖
				while(KeyPort!=0xfb);
		     	return Val;
		    }
			KeyPort=0xf7; //檢測第四行
			if(KeyPort!=0xf7){
				Val=KeyPort&0xf0;
		  	    Val+=0x07;
		  		while(KeyPort!=0xf7);
				DelayMs(10); //去抖
				while(KeyPort!=0xf7);
		     	return Val;
		    }
		}
	}
	return 0xff;
}*/
/*------------------------------------------------
         按鍵值處理函數，返回掃鍵值
------------------------------------------------*/
/*unsigned char KeyPro(void){
	switch(KeyScanSixteen()){
		case 0x7e:return 0;break;//0 按下相應的鍵顯示相對應的碼值
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
按鍵掃描函數，返回掃描鍵值
------------------------------------------------*/
unsigned char KeyScanEight(void){
	unsigned char keyvalue;
	unsigned char len = 0;
	if(KeyPort!=0xff){
		DelayMs(10);
		if(KeyPort!=0xff){
			keyvalue = KeyPort;

			LED1 = LED2 = LED3 = LED4 = 1;
		    while(KeyPort != 0xff){ // 壓住發出聲音
				SPK = !SPK;
				DelayMs(1);

				len++;
				if(len > 10000)len = 9999;

			}			
			LED1 = LED2 = LED3 = LED4 = 0;

			if(keyvalue == 0x7f) return 8;
			if(keyvalue == 0xf7) return 4;
			return len > 800 ? 1 : 0; // 憑感覺


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
