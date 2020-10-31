#include <ShiftRegisterPWM.h>

ShiftRegisterPWM sr(3, 16);

#define totalMagnets 9

void setMagnet(byte magnet, int sped){
  Serial.println(String(magnet)+":  "+String(sped));
  if(totalMagnets<=magnet || magnet<0){
    Serial.println("Magnet out of range ("+String(magnet)+")");
  }
  
  byte dir1 = sped>0?sped:0;
  byte dir2 = sped>0?0:-sped;

  //Ordering scheme:
  // Magnets:      ----1----  ----2----   3 ...
  // Pins:         dir1 dir2  dir1 dir2     ...
  // Positions:    0    1     2    3        ...
  
  byte dir1Pos = magnet*2;
  byte dir2Pos = magnet*2+1;
  
  sr.set(dir1Pos, dir1);
  sr.set(dir2Pos, dir2);
}

void stopAllMagnets(){
  for(int i = 0; i<totalMagnets; i++){
    setMagnet(i,0);
  }
}

void transferMagnet(int source, int destination, bool polarity = true){ //true polarity == positive power == South pole at magnet tip.   That's just how it worked out
  int polaritySign = polarity? 1 : -1;
  setMagnet(source, 0);
  setMagnet(destination, 255*polaritySign);
  delay(175);
}



void setup() {  // Setup runs once per reset
  // initialize serial communication @ 9600 baud:
  Serial.begin(9600);
  Serial.println("MagneMatrix 1000:  ENGAGE");
  pinMode(2, OUTPUT); // sr data pin
  pinMode(3, OUTPUT); // sr clock pin
  pinMode(4, OUTPUT); // sr latch pin
  pinMode(5, INPUT_PULLUP);

  sr.interrupt(ShiftRegisterPWM::UpdateFrequency::Medium);
  stopAllMagnets();
}

//Note:  It'd be preferable to use interrupts for this, but alas, my Shift Registers ate all my interrupt pins.  So we poll.
bool nextProgram = false;
bool lastPushed = false;
bool switchButton(){
  bool pushed = !digitalRead(5);
  if(pushed&&!lastPushed){
    nextProgram = true; //This is a seperate variable in case this function is called again,
    //before the progam is switched, and after the button is released.  If that makes any sense.
    Serial.println("Next Program!");
  }
  lastPushed = pushed;
  return nextProgram;
}

int lastMag = 0;
int lastDualMag = 0;
void runProgram(int * program, int len, int * dualProgram = NULL){ //Dual-Program is optional.  
  //You can theoretically control two balls at once with the power supplies limitations, but not super reliably.

  for(int i = 0; i<len; i++){
    transferMagnet(lastMag, program[i]);
    lastMag = program[i];
    if(dualProgram!=NULL){
      transferMagnet(lastDualMag, dualProgram[i], false); //Dual program must be of same length as primary. 
      //(also of note, yes, I did intentionaly reverse this one's polarity, just to make things more interesting
      //when the balls aren't doing what they're supposed to.  Otherwise, this shouldn't change how the magnets move.
      lastDualMag = dualProgram[i];
    }
    if(switchButton()){
      lastMag = 0;
      lastDualMag = 0;    
      transferMagnet(program[i],0);
      if(dualProgram!=NULL){
        transferMagnet(dualProgram[i],8); 
      }
      return;
    }
  }
}

const int circleProgram[8] = {0,1,2,5,8,7,6,3};
void circle(){
  runProgram(circleProgram,8);
}

const int plusProgram[8] = {4,1,4,5,4,7,4,3}; 
void plus(){
  runProgram(plusProgram,8);
}

const int spinnerProgram[8] = {0,1,2,5,8,7,6,3};
const int spinnerProgramDual[8] = {8,7,6,3,0,1,2,5};
void spinner(){
  runProgram(spinnerProgram, 8, spinnerProgramDual); //Make use of the dual-program thing I added.
}


int currentProgram = 0;
void loop() {
  switch(currentProgram){
    case 0:
      Serial.println("Circle");
      circle();
      break;
    case 1:
      Serial.println("Plus");
      plus();
      break;
//    case 2:  //interestingly, powering two magnets at once pulls so much current it underpowers the arduino, who controls the magnets.  It's like arduino suicide.
//      Serial.println("Spinner");
//      spinner();
//      break;
    default:
      currentProgram = 0; //Just an odd way of looping through the program list.  It works nicely, however.
      break;
  }
  if(nextProgram){
    currentProgram++;
    nextProgram = false;
  }
}
