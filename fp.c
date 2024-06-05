// 傳輸速率9600	晶振11.0592MHz
// port 插哪個就選哪個

/*
接線：
UART：ESP32-16, 17腳接8051-P3^0, P3^1
LED：8051 P3^4, P3^5, P3^6, P3^7 接 4 個 LED
按鈕：8051 P1 接 J26-K1~K8
聲音：8051 P2^6 接 J42-B1
七段顯示器：8051 P0接 J3 | P2^2, P2^3 接 J2-B, A

七段顯示器：
若超過8個字母，會顯示前7個+最後一個，不論輸入或接收的訊息

按鈕規則：
壓下長短K1輸入摩斯密碼
輸入完一個字元後壓下K4
沒壓K4會被跳過、沒有此摩斯密碼的也會被空白取代(a-z0-9)
換句話說就是要空白就直接按K4
壓下K8送出
最後一個字元沒壓下K4會被略過
*/

//包含標頭檔，一般情況不需要改動，標頭檔包含特殊功能寄存器的定義 
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

sbit SPK = P2^6;  //定義音樂端輸出接口
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

unsigned char TempData[MAX]; // 顯示在七段顯示器上

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
                   函式宣告
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
                    主函數
------------------------------------------------*/
void main (void){
	unsigned char k, ky, idx, bfr_idx; // indexes
	unsigned char bfr[6]; // Code Buffer

	// Initialization
	InitUART();
	Init_Timer0();
	//Init_Timer1(); // 開了會爆炸

	SendStr("System launched. -w-\n");

	idx = bfr_idx = 0;
	ES = 1;                  //打開串口中斷


	while (1){
		if (rec_flag == 1){ // 要收到CRLF才會是1
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

		}else{ // 沒收到訊息就可以做其他事：
		
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
		if (get_0d == 0){ // 還沒收到0x0d
			if (tmp == 0x0d) get_0d = 1; // 收到了 0x0d
			else{ // 將資料寫入 buffer
				buf[head]=tmp;              
				head++;
				if (head == MAX) head = 0;	
			}				     
		}else if (get_0d == 1){
			if (tmp != 0x0a){ // 若 0x0d 後不是 0x0a 那資料全部不要
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
void Timer1_isr(void) interrupt 3{

	TH1=(65536-2000)/256;		  //重新賦值 2ms
	TL1=(65536-2000)%256;	

	// 測試壓多久用
	/*
	TempData[0] = alphabet[26 + len/1000];
	TempData[1] = alphabet[26 + (len/100)%10];
	TempData[2] = alphabet[26 + (len/10)%10];
	TempData[3] = alphabet[26 + len%10];
	*/
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
按鍵掃描函數，返回掃描鍵值
------------------------------------------------*/
unsigned char KeyScanEight(void){
	unsigned char keyvalue;
	unsigned int len = 0;
	if(KeyPort!=0xff){
		DelayMs(10);
		if(KeyPort!=0xff){
			keyvalue = KeyPort;

			LED1 = LED2 = LED3 = LED4 = 0; // 亮光
		    while(KeyPort != 0xff){ // 壓住發出聲音
				SPK = !SPK;
				DelayMs(1);

				len++;
				if(len > 10000)len = 9999;

			}			
			LED1 = LED2 = LED3 = LED4 = 1; // 熄滅

			if(keyvalue == 0x7f) return 8; // 按下 K8
			if(keyvalue == 0xf7) return 4; // 按下 K4
			return (len > 300) ? 1 : 0; // 測試後感覺較好
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
