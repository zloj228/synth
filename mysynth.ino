#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <LowPassFilter.h>
#include <StateVariable.h>
#include <mozzi_rand.h>
#include <tables/cos2048_int8.h> // for filter modulation
#include <tables/sin2048_int8.h>
#include <tables/triangle_valve_2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/square_analogue512_int8.h>
#include <Line.h>
#include <Keypad.h>
#include <IntMap.h>
#define CONTROL_RATE 256

 
const byte ROWS = 5; //four rows
const byte COLS = 7; //four columns
char hexaKeys[ROWS][COLS] = {
    {8, 9, 10,11,12,13,14},
    {1, 2, 3, 4, 5, 6, 7},
    {15,16,17,18,19,20,21},
    {22,23,24,25,26,27,28},
    {29,30,31,32,33,34,35},
};

byte rowPins[ROWS] = {0, 1, 2, 3, 4}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 6, 7, 8, 10, 11, 12}; 
Keypad kpd = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
IntMap mapThis(0,1023,25,250);
IntMap mapFreq(0,255,0,3700);
IntMap kMapS(0, 1023, 0, 255);
IntMap kMapHzBytes(0 , 8191, 0, 255);
byte osc=0;
Oscil<2048, AUDIO_RATE>aOscil[5];
Oscil<2048, AUDIO_RATE>bSin(SAW2048_DATA);
Oscil<2048, AUDIO_RATE>cSin(SAW2048_DATA);
Oscil<2048, AUDIO_RATE>dSin(SIN2048_DATA);
Oscil<2048, AUDIO_RATE>eSin(SAW2048_DATA);
Oscil<SQUARE_ANALOGUE512_NUM_CELLS, AUDIO_RATE>fSin(SQUARE_ANALOGUE512_DATA);
Oscil<SAW2048_NUM_CELLS, CONTROL_RATE> LFO(SAW2048_DATA);
LowPassFilter lpf;

int zeroSum = 0;

int subPos = 0;
byte indx;

int gain[5] = {0, 0, 0, 0, 0};

int lpf_mode = 0;
int lpf_speed;
int cutOffFreq;
int Reson=200;

float A_1 = 110.00;
int notes[73];
int freq[5] ={0, 0, 0, 0, 0};
int playedNote[5] = {0, 0, 0, 0, 0};
int playedNoteIndex;
int butNow[5] = {0, 0, 0, 0, 0};
int butWas[5] = {0, 0, 0, 0, 0};

byte LPFpot = 1;
byte LPRpot = 2;




void setTables() {
  for(int i = 0;i<5;i++){
    if(osc==0)aOscil[i].setTable(SAW2048_DATA);
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
  LFO.setFreq(5.3f);
  LFO.next();
}


void updateControl(){ 
  
  collectingNotes();


  
  for(int i = 0;i< 5;i++){
    if(butNow[i]==0){
      gain[i]=0;
    }
    else{
      freq[i]=notes[butNow[i]];
      aOscil[i].setFreq(freq[i]);
      gain[i]=80;
    }
    
  }
  
  //aOscil[0].setFreq(440);


  cutOffFreq = kMapHzBytes(freq[0]) + kMapS(mozziAnalogRead(LPFpot));
  if(cutOffFreq>200)cutOffFreq=200;
  lpf.setCutoffFreqAndResonance(cutOffFreq, Reson);
  
}


int updateAudio(){
  int asig = 
  lpf.next(((aOscil[0].next() * gain[0]) >> 8) +
           ((aOscil[1].next() * gain[1]) >> 8) +
           ((aOscil[2].next() * gain[2]) >> 8));
           

    
  return asig;
 }


void loop(){
  audioHook(); // required here
}
void collectingNotes(){
  subPos = 0;
  zeroSum = 0;
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

int indexOfButton(byte key){
    for (int i=0;i<5;i++){
      if (butNow[i]==key)return i;
    }
    return -1;
}
int indexOfEmptyButton(){
  for (int i=0;i<5;i++){
      if (butNow[i]==0)return i;
    }
    return -1;
}
