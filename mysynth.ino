#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <LowPassFilter.h>
#include <StateVariable.h>
#include <mozzi_rand.h>
#include <tables/sin256_int8.h>
#include <tables/saw256_int8.h>
#include <AudioDelay.h>
#include <Line.h>
#include <Keypad.h>
#include <IntMap.h>
#define CONTROL_RATE 64

 
const byte ROWS = 5; //four rows
const byte COLS = 7; //four columns
char hexaKeys[ROWS][COLS] = {
    {1, 2, 3, 4, 5, 6, 7},
    {8, 9, 10,11,12,13,14},
    {15,16,17,18,19,20,21},
    {22,23,24,25,26,27,28},
    {29,30,31,32,33,34,35},
};

byte rowPins[ROWS] = { 1,12, 11, 10,8}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {7, 6, 5, 4, 3, 2,0}; 
Keypad kpd = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
IntMap kMapFeed(0,1023,0,127);
IntMap kMapS(0, 1023, 0, 255);
IntMap kMapHzBytes(0 , 8191, 0, 255);
byte osc=0;
Oscil<256, AUDIO_RATE>aOscil[5];
Oscil<256, AUDIO_RATE>bOscil[5];
Oscil<256, AUDIO_RATE>cOscil[5];

Oscil<SIN256_NUM_CELLS, CONTROL_RATE> LFO(SIN256_DATA);

AudioDelay <128> aDel;
LowPassFilter lpf;


byte osc_sum=0;

byte indx;
byte del_samps;

byte gain[5] = {0, 0, 0, 0, 0};
byte bgain[5]={0,0,0,0,0};
byte cgain[5]={0,0,0,0,0};

byte maxgain=0;
byte maxbgain=0;
byte maxcgain=0;

byte cutOffFreq;
byte Reson=200;

float A_1 = 27.50;
int notes[73];
int freq[5] ={0, 0, 0, 0, 0};
int playedNote[5] = {0, 0, 0, 0, 0};

int butNow[5] = {0, 0, 0, 0, 0};


byte LPFpot = 1;
byte VBRpot = 2;
byte CHRpot = 5;


Q16n16 deltime;

void setTables() {
  for(int i = 0;i<5;i++){
    aOscil[i].setTable(SAW256_DATA);
    bOscil[i].setTable(SAW256_DATA);
    cOscil[i].setTable(SAW256_DATA);
    
  }
}
void setNotes(){
    for (int i = 3; i <= 73 + 3; i++) {
      notes[i - 2] = A_1 * pow(2 , (i / 12.00));
    }
}


void setup(){
  setTables();
  setNotes();
  startMozzi(CONTROL_RATE); 
  LFO.setFreq(1.3f);
  }


void updateControl(){ 
  
  collectingNotes();
  collectingAnalogs();

  
  for(int i = 0;i< 5;i++){
    if(butNow[i]==0){
      gain[i]=0;
      bgain[i]=0;
    }
    else{
      aOscil[i].setFreq(notes[butNow[i]]);
      bOscil[i].setFreq(notes[butNow[i]+24]);
      gain[i]=maxgain*(4-getButtonCount());
      bgain[i]=maxbgain*(4-getButtonCount());
    }
    
  }
  
  //aOscil[0].setFreq(440);


  
  //cutOffFreq = LFO.next();
  
  if(cutOffFreq>200)cutOffFreq=200;
  lpf.setCutoffFreqAndResonance(cutOffFreq, Reson);
  

}


int updateAudio(){
  int asig = 
  lpf.next(((aOscil[0].next() * gain[0]) >> 8) +
           ((aOscil[1].next() * gain[1]) >> 8) +
           ((aOscil[2].next() * gain[2]) >> 8)+
           ((bOscil[0].next() * bgain[0]) >> 8)+
           ((bOscil[1].next() * bgain[1]) >> 8)+
           ((bOscil[2].next() * bgain[2]) >> 8));
           

    
  return asig+ aDel.next(asig, deltime);
 }


void loop(){
  audioHook(); // required here
}
void collectingNotes(){


   if (kpd.getKeys())
    {
        for (int i=0; i<LIST_MAX; i++)
        {
            if(kpd.key[i].kstate==PRESSED) {
              if(indexOfButton(kpd.key[i].kchar)==-1){
                indx=indexOfEmptyButton();
                if(indx!=-1){
                  butNow[indx]=kpd.key[i].kchar;
                }
              }
            }
            if(kpd.key[i].kstate==RELEASED) {
              indx=indexOfButton(kpd.key[i].kchar);
              if(indx!=-1){
                butNow[indx]=0;
              }
            }
            
        }
    }
}

void collectingAnalogs(){
  cutOffFreq = kMapHzBytes(freq[0]) + kMapS(mozziAnalogRead(LPFpot));
  deltime = kMapFeed(mozziAnalogRead(CHRpot));
  byte count = getOscCount();
  if(count>0){
  if (mozziAnalogRead(7)>100){
    maxgain=50/count;
  }else{
    maxgain=0;
  }
  if (mozziAnalogRead(6)>100){
    maxbgain=50/count;
  }else{
    maxbgain=0;
  }
  if (mozziAnalogRead(0)>100){
    maxcgain=50/count;
  }else{
    maxcgain=0;
  }
  }else{
    maxgain=0;
    maxbgain=0;
    maxcgain=0;
  }
}

int indexOfButton(byte key){
    for (int i=0;i<3;i++){
      if (butNow[i]==key)return i;
    }
    return -1;
}
int indexOfEmptyButton(){
  for (int i=0;i<3;i++){
      if (butNow[i]==0)return i;
    }
    return -1;
}
byte getButtonCount(){
  byte c = 0;
  for (int i=0;i<3;i++){
      if (butNow[i]>0)c++;
    }
    return c;
}
byte getOscCount(){
  byte c=0;
  if((mozziAnalogRead(7)>100)>0)c++;
  if((mozziAnalogRead(6)>100)>0)c++;
  if((mozziAnalogRead(0)>100)>0)c++;
 return c;
}
