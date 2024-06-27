/*     Filament Tensioner for Arduino by Glowpon3 now for big spool
 *      
 *  11-6-2023   www.glowpon3.com
 *   5-27-2024 updated by Glowpon3
 *   6-26-2024 updated by Glowpon3 for serial control
 */

// defines pins numbers
int feedDebugPin = 4;  //output pin for SIO to show stepper activity
int stepPin = 13; //Output pin for stepper driver STEP pin
int dirPin = 12; //Output pin for stepper driver DIR pin
int enPin = 11;  //Output pin for stepper driver EN pin
int inputTensionLow = A1;  //Input pin for filament tension switch with integrated pull-up resistor enabled
int inputVref = A3; //Input pin for motor driver Vref pin for adjusting stepper current via serial debug
int inputSIO = A5;  //Input from SIO passed to stepper driver enable pin

int tensionLow;  //variable to store state of tension pin
int SIOenable;  //variable to store sio pin state
int vref1;  //variable to store vref pin value
float vref2;  //variable to store vref calculated as Volts

int delayStep = 1000;  //default speed when starting up
int minDelay = 200;  //minimum delay (fastest feedrate)
int maxDelay = 1000;  //maximum delay (slowest feedrate and idle trigger)

int p=10;  //used to speed up feedrate using equation delayStep -= (delaystep/p)
int i=2;  //depreciated
int d=10;  //used to slow down feedrate using equation delayStep += d
int j;  //for loop variable
int stepMode = 8;  //microstepping mode used in for loops

String oldAck = "";  //serial report handling
String newAck = "";  //serial report handling
int debugV = 0;  //for vref debug cleanup
int debugD = 0;  //for delay debug cleanup
int debugM = 0;  //for simple debug
bool flock = false;  //for manual control of enable pin
unsigned int stepRoll = 0;  //for debug output of distance travelled.
bool dir = true;  //for filament change and direction control


void setup() {
  pinMode(feedDebugPin,OUTPUT);
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(enPin,OUTPUT);
  pinMode(inputTensionLow,INPUT_PULLUP);  //pullup resister enabled to make contact wiring easier
  pinMode(inputVref,INPUT);
  pinMode(inputSIO,INPUT);
  //start serial comms
  Serial.begin(115200);
}


void loop() {
  if(dir){
    digitalWrite(dirPin,HIGH); // makes sure motor is turning the correct way.  Change to LOW to reverse extruder gear direction
  } 
  else {
    digitalWrite(dirPin,LOW); // reverse direction flow if dir is false
  }

  //read tension pin
  tensionLow = digitalRead(inputTensionLow); //Read and store the sensor data
  
  //While input is high (not shorted to ground) accellerate to full speed
  while (tensionLow == 1) {
    checkSerial();
    checkEnable();
    tensionLow = digitalRead(inputTensionLow);  //read tension pin and update variable for while loop
    digitalWrite(feedDebugPin,HIGH);  //set debug pin high while stepping for output to SIO
    vref1 = analogRead(inputVref);  //test vref for serial debug
    vref2 = (debugV*(vref1/1024.)*5.);  //convert vref from int to voltage
 
    for (j=1;j<=stepMode;j++) {
      //for loop for duplicating steps for microstepping
      digitalWrite(stepPin,HIGH);  //output first step
      delayMicroseconds(delayStep);  //delay
      digitalWrite(stepPin,LOW);  //output second step
      delayMicroseconds(delayStep);  //delay
    }
    stepRoll += 1;  //increment distance tracker
    delayStep -= (delayStep/p);  //decrement delayStep by (delayStep / p) produces accelleration curve instead of linear
    if (delayStep < minDelay) {delayStep = minDelay;}  //make sure delayStep isn't below minDelay and correct if needed
    if(debugM){
      newAck = ("Delay " + String(delayStep*debugD) + "-Feeding " + String(vref2) + " Vref on? " + String(debugV) + " Distance " + String(stepRoll));
      ack();
    };
  }  //end while loop High

  //whiel tension is low, slow stepper and then stop
  while (tensionLow == 0) {
    checkSerial();
    checkEnable();
    tensionLow = digitalRead(inputTensionLow);  //read tension pin and update variable for while loop
    digitalWrite(feedDebugPin,LOW);  //set debug pin low while stepping
    vref1 = analogRead(inputVref);
    vref2 = (debugV*(vref1/1024.)*5.);
    //if delay step is less than, most importantly not equal to, max delay, then slow down.
    if (delayStep < maxDelay) {
      for (j=1;j<=stepMode;j++) {
        digitalWrite(stepPin,HIGH);
        delayMicroseconds(delayStep);
        digitalWrite(stepPin,LOW);
        delayMicroseconds(delayStep);
      }  //for loop for duplicating steps for microstepping
      stepRoll += 1;  //increment distance tracker
    }  //end if loop for stopping extruder 
    delayStep += d;  //decrement delayStep by d
    if (delayStep > maxDelay) delayStep = maxDelay;  //make sure delayStep isn't above maxDelay and correct if needed
    if(debugM){
      if (delayStep == maxDelay){
        newAck = ("Delay " + String(delayStep*debugD) + "-Stopped " + String(vref2) + " Vref on? " + String(debugV) + " Distance " + String(stepRoll));
      } else {
        newAck = ("Delay " + String(delayStep*debugD) + "-Slowing " + String(vref2) + " Vref on? " + String(debugV) + " Distance " + String(stepRoll));
      };
      ack();
    };
  }  //end while loop Low

} //end of void loop

void ack() {
  if (oldAck != newAck) {
    Serial.println(newAck);
    oldAck = newAck;
    };  //if Acknowledgement has changed, report to serial and update oldAck
}

void checkEnable(){
  if (flock) {
    digitalWrite(enPin, LOW);  // Enable stepper driver board regardless of SIO input
  }
  else {
    SIOenable = digitalRead(inputSIO); //check the SIO pin to see if feeder should be enabled. use SIOenable = 1 if SIO is not connected
    if (SIOenable) {
      digitalWrite(enPin,LOW);  // Enables the stepper driver board
    }
    else {
      digitalWrite(enPin,HIGH);  // Disable the stepper driver board
    };
  };
}

void checkSerial(){
  if (Serial.available()){
    
    String buf = Serial.readStringUntil('\n');  //fetch the buffer until the first line break
    buf.trim();  //trim any leading or trailing whitespaces
    int sepPos = buf.indexOf(" ");
    String command ="";
    String value = "";

    if(sepPos !=-1){
      //if the index of a separator exists then calculate the command and the value after the separator
      command = buf.substring(0,sepPos);
      value = buf.substring(sepPos+1);
    }else{
      //if there is no separator then just use the buffer as the full command
      command = buf;
    }
    command.toLowerCase();  //convert command to Lower Case
    
    if(debugD || debugM){
      if(command != ""){
        debugD = 0;
        debugM = 0;
        Serial.println("Monitor halted, ready for command:");
      };
    };
    
    if(command.startsWith("?") || command == "help"){
      Serial.println(F("Help:"));
      Serial.println(F("Commands:"));
      Serial.println(F("m (monitor activity)"));
      Serial.println(F("Vout on/off (enable Vref output)"));
      Serial.println(F("Dout (enable Delay output)"));
      Serial.println(F("send any key to exit monitoring"));
      Serial.println(F("report (echo variables)"));
      Serial.println(F("Delay (impermanantly set delay)"));
      Serial.println(F("mind (set max speed)"));
      Serial.println(F("maxd (set min speed)"));
      Serial.println(F("P (controls accelleration d -= d/p)"));
      Serial.println(F("I (depreciated)"));
      Serial.println(F("D (controls decelleration d += d)"));
      Serial.println(F("flock on/off (locks enable pin on)"));
      Serial.println(F("count (reset steproll count)"));
      Serial.println(F("steps (set microstepping mode)"));
      Serial.println(F("! (emergency restore defaults with flock enabled)"));
      Serial.println(F("C (change direction)"));
    }
    else if(command == "vout"){
      if (value == "on" || value == "1") {
        debugV = 1;
        Serial.println("Vref debug on");
      }
      else if(value == "off" || value == "0"){
        debugV = 0;
        Serial.println("Vref debug off");
      }
      else {
        Serial.println("value not recognized, use on/off or 1/0");
      }
      return;
    } 
    else if(command == "dout"){ 
      debugD = 1;
      Serial.println("Delay debug on");
      return;
    } 
    else if(command == "m"){ 
      debugM = 1;
      Serial.println("monitor on");
      return;
    } 
    else if(command == "report"){ 

      Serial.println("delay----" + String(delayStep));
      Serial.println("mindelay-" + String(minDelay));
      Serial.println("maxdelay-" + String(maxDelay));
      Serial.println("p--------" + String(p));
      Serial.println("i--------" + String(i));
      Serial.println("d--------" + String(d));
      Serial.println("Flock----" + String(flock));
      Serial.println("Steps----" + String(stepRoll));
      Serial.println("MSteps---" + String(stepMode));
      return;
    } 
    else if(command == "delay"){ 
      delayStep = value.toInt();
      Serial.println("delay = " + String(delayStep));
      return;
    } 
    else if(command == "mind"){ 
      minDelay = value.toInt();
      Serial.println("mindelay = " + String(minDelay));
      return;
    } 
    else if(command == "maxd"){ 
      maxDelay = value.toInt();
      Serial.println("maxdelay = " + String(maxDelay));
      return;
    } 
    else if(command == "p"){ 
      p = value.toInt();
      Serial.println("P = " + String(p));
      return;
    } 
    else if(command == "i"){ 
      i = value.toInt();
      Serial.println("I = " + String(i));
      return;
    } 
    else if(command == "d"){ 
      d = value.toInt();
      Serial.println("D = " + String(d));
      return;
    } 
    else if(command == "flock"){ 
      if (value == "on" || value == "1") {
        flock = true;
        Serial.println("Feed-Lock ON");

      }
      else if(value == "off" || value == "0") {
        flock = false;
        Serial.println("Feed-Lock OFF");

      }
      else {
        Serial.println("value not recognized, use on/off or 1/0");
      }
      return;
    }
    else if(command == "count"){ 
      stepRoll = 0;
      Serial.println("Distance = " + String(stepRoll));
      return;
    }
    else if(command == "steps"){ 
      stepMode = value.toInt();
      Serial.println("Microstep mode = " + String(stepMode));
      return;
    }
    else if(command == "!"){ 
      delayStep = 1000;  //default speed when starting up
      minDelay = 200;  //minimum delay (fastest feedrate)
      maxDelay = 1000;  //maximum delay (slowest feedrate and idle trigger)
      p=10;  //used to speed up feedrate using equation delayStep -= (delaystep/p)
      i=2;  //depreciated
      d=10;  //used to slow down feedrate using equation delayStep += d
      stepMode = 8;  //microstepping mode used in for loops
      debugV = 0;  //for vref debug cleanup
      debugD = 0;  //for delay debug cleanup
      debugM = 0;  //for simple debug
      flock = true;  //for manual control of enable pin
      stepRoll = 0;  //for debug output of distance travelled.
      Serial.println("Emergency Defaults Restored!");
      return;
    }
    else if(command == "c"){ 
      if (value == "on" || value == "1") {
        dir = false;
        Serial.println("Direction Reversed");
      }
      else if(value == "off" || value == "0") {
        dir = true;
        Serial.println("Direction Forward");
      }
      else {
        Serial.println("value not recognized, use on/off or 1/0");
      }
      return;
    }
    else{
      Serial.print(F("ERR: bad cmnd["));Serial.print(command);Serial.println(F("]"));
    }
  }
}
