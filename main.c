// PIC16F1705 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = OFF       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover Mode (Internal/External Switchover Mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PPS1WAY = OFF    // Peripheral Pin Select one-way control (The PPSLOCK bit can be set and cleared repeatedly by software)
#pragma config ZCDDIS = OFF     // Zero-cross detect disable (Zero-cross detect circuit is enabled at POR)
#pragma config PLLEN = ON       // Phase Lock Loop enable (4x PLL is always enabled)
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will not cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#define _XTAL_FREQ 32000000

#define _16NOTE 3000
#define _16NOTE_DOT 1500
#define _16NOTE_TRIPLET 2000

unsigned char pr2value=0;                           //PR2の値のバッファ。割り込み時のPR2の代入時に用いる
unsigned char phase = 0;                            //位相をずらすための変数。減衰に用いる
int tmr4time,tmr43time,tmr4time_dot,tmr4flag=0;     //timer4関連の変数。timer4は音符の長さを測る。
unsigned char tmr2flag = 0,tmr1flag = 0;            //timer1,2のフラグ

unsigned char sinwave96_envelope[144]={
    64,68,72,76,80,84,88,92,96,99,102,106,109,111,114,116,
    119,121,122,124,125,126,126,127,127,127,126,126,125,124,122,121,
    119,116,114,111,109,106,102,99,96,92,88,84,80,76,72,68,
    64,60,56,52,48,44,40,36,33,29,26,22,19,17,14,12,
    9,7,6,4,3,2,2,1,1,1,2,2,3,4,6,7,
    9,12,14,17,19,22,26,29,33,36,40,44,48,52,56,60,
    
    64,68,72,76,80,84,88,92,96,99,102,106,109,111,114,116,
    119,121,122,124,125,126,126,127,127,127,126,126,125,124,122,121,
    119,116,114,111,109,106,102,99,96,92,88,84,80,76,72,68
};

void tempo(float tempo);
void note(unsigned char scale,unsigned char length,unsigned char octave);
void rest(unsigned char length);

void haruwa_pastel(void);

void interrupt isr(void){
    if(TMR2IF == 1){
        PR2 = pr2value;
        TMR2IF = 0;
        tmr2flag = 1; 
    }
    else if(TMR4IF == 1){
        PR4 = 250;      //5ms
        TMR4IF = 0;
        tmr4flag++;
    }
    else if(TMR1IF == 1){
        TMR1H = 0x3C;   //50ms or 100ms
        TMR1L = 0xB0;
        TMR1IF = 0;
        tmr1flag++;
    }
}

void main(void){
    OSCCON = 0b01110000;    //内部クロック　32MHz
    ANSELA = ANSELC = 0;
    TRISA = 0;
    TRISC = 0;
    
    DAC1CON0 = 0b10000000;  
    DAC1CON1 = 0;
    OPA1CON = 0b11010010;   //オペアンプはバッファ出力、DACから入力
    
    T1CON = 0x21;       
    TMR1H = 0x3C;
    TMR1L = 0xB0;
    
    T4CON = 0x4E;           //5ms
    PR4 = 250;
    
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
    
    tempo(120);
    
    while(1){
        haruwa_pastel();
    }
    
}

/*
 * 音を出力する関数
 * scale:C=0,D=1...B=6,先頭に1が付く1以外の数値は#を表す
 * length:4分音符=4,8分音符=8...、全音符～16分音符まで、
 *        先頭に1がつく1以外の数値は付点を、2だと3連符を表す
 * octave:5オクターブ=5,6オクターブ=6...、3～7
 */
void note(unsigned char scale,unsigned char length,unsigned char octave){
    int note_length;
    unsigned char octave_high=0,octave_low=0;
    
    phase = 0;
    
    switch(octave){
        case 3:
            octave_high = 1;
            octave_low = 3;
            break;
            
        case 4:
            octave_high = 1;
            octave_low = 1;
            break;
            
        case 5:
            octave_high = 1;
            octave_low = 0;
            break;
            
        case 6:
            octave_high = 2;
            octave_low = 0;
            break;
            
        case 7:
            octave_high = 4;
            octave_low = 0;
            break;
            
        default:
            octave_high = 1;
            octave_low = 0;
            break;
    }
    
    switch(length){
        case 16:
            T1CON = 0x11;
            note_length = tmr4time;
            break;
            
        case 116:
            T1CON = 0x11;
            note_length = tmr4time + tmr4time_dot;
            break;
            
        case 216:
            T1CON = 0x11;
            note_length = tmr43time;
            break;
            
        case 8:
            T1CON = 0x11;
            note_length = tmr4time * 2;
            break;
            
        case 18:
            T1CON = 0x11;
            note_length = tmr4time * 3;
            break;
            
        case 28:
            T1CON = 0x11;
            note_length = tmr43time * 2;
            break;
            
        case 4:
            T1CON = 0x11;
            note_length = tmr4time * 4;
            break;
            
        case 14:
            T1CON = 0x11;
            note_length = tmr4time * 6;
            break;
            
        case 24:    
            T1CON = 0x11;
            note_length = tmr43time * 4;
            break;
            
        case 2:
            T1CON = 0x21;
            note_length = tmr4time * 8;
            break;
            
        case 12:
            T1CON = 0x21;
            note_length = tmr4time * 12;
            break; 
            
        case 22:
            T1CON = 0x21;
            note_length = tmr43time * 8;
            break;
            
        case 1:
            T1CON = 0x21;
            note_length = tmr4time * 16;
            break;
            
        default:
            T1CON = 0x11;
            note_length = tmr4time * 4;
            break;
    }
    
    switch(scale){
        case 0:
            T2CON = 0x04;   //C
            PR2 = pr2value = 158;
            break;
            
        case 10:
            T2CON = 0x04;   //C#
            PR2 = pr2value = 149;
            break;
            
        case 1:
            T2CON = 0x04;   //D
            PR2 = pr2value = 141;
            break;
            
        case 11:
        	T2CON = 0x04;   //D#(Eflat)
        	PR2 = pr2value = 133;
        	break;
        	
      	case 2:
            T2CON = 0x04;   //E
            PR2 = pr2value = 125;
            break;
            
        case 3:
            T2CON = 0x04;   //F
            PR2 = pr2value = 118;
            break;
            
        case 13:
            T2CON = 0x04;   //F#
            PR2 = pr2value = 112;
            break;
            
        case 4:
            T2CON = 0x04;   //G
            PR2 = pr2value = 105;
            break;
            
        case 14:
            T2CON = 0x04;   //G#
            PR2 = pr2value = 99;
            break;
            
        case 5:
            T2CON = 0x04;   //A
            PR2 = pr2value = 94;
            break;
            
        case 15:
            T2CON = 0x04;   //A#(Bflat)
            PR2 = pr2value = 88;
            break;
            
        case 6:
            T2CON = 0x04;   //B
            PR2 = pr2value = 83;
            break;
            
        default:
            T2CON = 0x04;   //A
            PR2 = pr2value = 94;
        	break;
    }

    PIR1bits.TMR2IF = 0;
    PIR1bits.TMR1IF = 0;
    PIR2bits.TMR4IF = 0;
    PIE1bits.TMR2IE = 1;
    PIE1bits.TMR1IE = 1;
    PIE2bits.TMR4IE = 1;
        
    do{            
        for(unsigned char n=0;n<96;n=n+octave_high){
            for(unsigned char m=0;m<=octave_low;m++){
                DAC1CON1 = sinwave96_envelope[n] + sinwave96_envelope[n+phase];
                while(!tmr2flag);
                tmr2flag = 0; 
            }
        }
            
        //50msごとに位相をずらして減衰させる        
        if(tmr1flag >= 1){
            if(phase == 48){
                TMR1IE = 0;
            }      
            else{
                phase++;
            }       
            tmr1flag = 0;
        }               
    }while(tmr4flag <= note_length);

    TMR4IE = 0;
    tmr4flag = 0;
    phase = 0;

}

void rest(unsigned char length){
    int rest_length;    //基本的な考え方はnote_lengthと同じ。先頭に1が付く1以外の数値は付点x分音符,先頭2だと3連符
    switch(length){
        case 16:
            rest_length = tmr4time;
            break;
            
        case 116:
            rest_length = tmr4time + tmr4time_dot;
            break;
            
        case 8:
            rest_length = tmr4time * 2;
            break;
            
        case 18:
            rest_length = tmr4time * 3;
            break;
            
        case 4:
            rest_length = tmr4time * 4;
            break;
            
        case 14:
            rest_length = tmr4time * 6;
            break;
            
        case 2:
            rest_length = tmr4time * 8;
            break;
            
        case 12:
            rest_length = tmr4time * 12;
            break;
            
        case 1:
            rest_length = tmr4time * 16;
            break;
            
        default:
            rest_length = tmr4time;
            break;
    }    
    
    PIR2bits.TMR4IF = 0;
    PIE2bits.TMR4IE = 1;
    while(tmr4flag <= rest_length);
    PIE2bits.TMR4IE = 0;
    tmr4flag = 0;
}

void tempo(float tempo){
    tmr4time = (int)(_16NOTE/tempo);            //16分音符のtimer4の割り込み回数(tmr4、1回＝5ms)
    tmr43time = (int)(_16NOTE_TRIPLET/tempo);   //3連16分音符のtimer4の割り込み回数
    tmr4time_dot = (int)(_16NOTE_DOT/tempo);    //32分音符、16分音符の付点用
}


/*デモ曲：春はパステル*/
void haruwa_pastel(void){
    tempo(120);
    
    note(2,8,5);note(3,8,5);                                                                        //のは
    note(4,4,5);note(2,4,5);note(3,4,5);note(2,8,5);note(1,8,5);                                    //らのはなは
    note(0,8,5);note(0,4,5);note(2,8,5);note(4,4,5);note(4,8,5);note(4,8,5);                        //あかるい　ほほ
    note(5,4,5);note(3,4,5);note(4,8,5);note(4,8,5);note(3,8,5);note(2,8,5);                        //えみあつめて
    note(3,8,5);note(3,4,5);note(2,8,5);note(1,4,5);                                                //ひかるよ
    note(2,8,5);note(3,8,5);                                                                        //はる
    note(4,4,5);note(2,4,5);note(3,8,5);note(3,8,5);note(2,8,5);note(1,8,5);                        //のあたたかい
    note(0,14,5);note(2,8,5);note(4,4,5);note(0,8,6);note(6,8,5);                                   //かぜが　やさ
    note(5,4,5);note(0,4,6);note(4,8,5);note(4,8,5);note(0,8,6);note(0,8,6);                        //しくえがおを
    note(6,8,5);note(0,4,6);note(1,8,6);note(0,4,6);                                                //ゆらすよ
    
    note(0,8,6);note(6,8,5);                                                                        //たく
    note(5,8,5);note(5,8,5);note(5,8,5);note(5,8,5);note(6,8,5);note(5,8,5);note(4,8,5);note(3,8,5);//さんのしろいはな
    note(4,18,5);note(4,16,5);note(14,8,5);note(5,8,5);note(6,8,5);note(0,4,6);note(4,8,5);         //きいろいはな　し
    note(5,8,5);note(5,8,5);note(5,8,5);note(5,8,5);note(5,8,5);note(5,8,5);note(6,8,5);note(5,8,5);//がつをうめつくし
    note(4,2,5);rest(4);                                                                            //て
    
    note(2,8,5);note(3,8,5);                                                                        //のは
    note(4,4,5);note(0,4,6);note(6,8,5);note(6,4,5);note(0,8,6);                                    //らが　あわく
    note(6,8,5);note(0,8,6);note(1,8,6);note(1,8,6);note(0,4,6);note(4,8,5);note(4,8,5);            //いろづくよ　はる
    note(5,4,5);note(0,4,6);note(6,14,5);note(6,8,5);note(0,2,6);rest(4);                           //はパステル
}
