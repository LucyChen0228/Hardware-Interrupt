// -------------------- User Settable Variables --------------------

int8_t hrpin = 0; //Whatever analog pin the sensor is hooked up to

int8_t emgpin = 1; //emg sensor pin

int8_t gsrpin = 2; //gsr sensor pin

int8_t BUTTON = 7; // button connector

int8_t ledPin = 13; //  the number of the LED pin ( The Arduino mage 2560's own led is in the pin 13) 

 

int8_t scale_data = 0; /*Uses dynamic scaling of data when set to 1, not if set to 0 \

                       sampling speed over 1500Hz not recommended when scaling data 

                       on 8-bit AVR (e.g. Arduino)*/

int8_t mode = 1; /*Speed mode. \

              0 means the "sample_rate" speed will be used \

              1 means custom. Custom sampling rate is set through Serial after connect.\ */

int16_t sample_rate = 100; /*should be 4Hz or more, over 2KHz on AVR not recommended.

                           When using adaptive scaling: over 1.5KHz not recommended on AVR.

                           Higher speeds attainable on 32-bit chipsets.

                           Please see documentation for suggested limits and theoretical limits*/

                          

// -------------------- End User Settable Variables --------------------

//Don't change values from here on unless you know what you're doing (from some guide book)

long scalingFactor;

long timerValue;

long t1;

long t_end;

 

// -------------------- Data Struct Definition--------------------

struct dataBuffers 

{

  //initialise two buffers of 50 each sensor

  int16_t hrdata0[50] = {0};

  int16_t hrdata1[50] = {0};

  int16_t gsrdata0[50] = {0};

  int16_t gsrdata1[50] = {0};

  int16_t emgdata0[50] = {0};

  int16_t emgdata1[50] = {0};

  int8_t buttondata0[50] = {0};

  int8_t buttondata1[50] = {0};

  int16_t bufferPointer = 0; //buffer index to write values to

  int16_t bufferMarker = 0; //0 for buffer 0, 1 for buffer 1

  int16_t buffer0State = 0; //0 if clean, 1 if dirty

  int16_t buffer1State = 0;

};

 

struct workingDataContainer // to store the data

{

  int16_t curVal = 0;

  int16_t curVal_emg = 0;

  int16_t curVal_gsr = 0;

  int16_t curVal_button = 0;

  int16_t rangeLow = 0;

  int16_t rangeLowNext = 1024;

  int16_t rangeHigh = 1023;

  int16_t rangeHighNext = 1;

  int16_t rangeCounter = 0;

};

 

// -------------------- Data Struct Init --------------------

struct dataBuffers dataBuf;

struct workingDataContainer workingData;

 

// -------------------- Functions --------------------

void readSensors(struct dataBuffers &dataBuf)

{ //read the sensors, put in correct buffer

  workingData.curVal = analogRead(hrpin);

  workingData.curVal_emg = analogRead(emgpin);

  workingData.curVal_gsr = analogRead(gsrpin);

  workingData.curVal_button = digitalRead(BUTTON);

 

 

if ( workingData.curVal_button == HIGH) {     //being pressed not sure HIGH will work here, might be boolean

    // turn LED on:

    digitalWrite(ledPin, HIGH);

  } else {

    // turn LED off:

    digitalWrite(ledPin, LOW);  //not being pressed
  }
  


 if(scale_data) 
  { //scale the data if requested
    //Actually we don't need this function, since we don't need to scale the data here.
    // I just mention this function here incase.
   

    establish_range(workingData);
    workingData.curVal = mapl(workingData.curVal, workingData.rangeLow, workingData.rangeHigh);  
    if(workingData.curVal < 0) workingData.curVal = 0;
  }
  
  //put sensor value in correct buffer
  if (dataBuf.bufferMarker == 0) 
  {
    dataBuf.hrdata0[dataBuf.bufferPointer] = workingData.curVal;
    dataBuf.gsrdata0[dataBuf.bufferPointer] = workingData.curVal_gsr;
    dataBuf.emgdata0[dataBuf.bufferPointer] = workingData.curVal_emg;
    dataBuf.buttondata0[dataBuf.bufferPointer] = workingData.curVal_button;
    
  } else 
  {
    dataBuf.hrdata1[dataBuf.bufferPointer] = workingData.curVal;
    dataBuf.gsrdata1[dataBuf.bufferPointer] = workingData.curVal_gsr;
    dataBuf.emgdata1[dataBuf.bufferPointer] = workingData.curVal_emg;
    dataBuf.buttondata1[dataBuf.bufferPointer] = workingData.curVal_button;
  }
  dataBuf.bufferPointer++;
}


// something with scale_data. We don't need that, so please skip the next two functions
void establish_range(struct workingDataContainer &workingData) 
{
  if(workingData.rangeCounter <= scalingFactor)
  {
    //update upcoming ranges
    if(workingData.rangeLowNext > workingData.curVal) workingData.rangeLowNext = workingData.curVal;
    if(workingData.rangeHighNext < workingData.curVal) workingData.rangeHighNext = workingData.curVal;
    workingData.rangeCounter++;
  } else {
    //set range, minimum range should be bigger than 50
    //otherwise set to default of (0, 1024)
    if((workingData.rangeHighNext - workingData.rangeLowNext) > 50)
    {
      //update range
      workingData.rangeLow = workingData.rangeLowNext;
      workingData.rangeHigh = workingData.rangeHighNext;
      workingData.rangeLowNext = 1024;
      workingData.rangeHighNext = 1;      
    } else {
      //reset range to default
      workingData.rangeLow = 0;
      workingData.rangeHigh = 1024;
    }
    workingData.rangeCounter = 0;
  }
}

// skip
long mapl(long x, long in_min, long in_max)
{
  return (x - in_min) * 1023 / (in_max - in_min) + 1;
}

void getOCR(long fs)
{ // Calculate timer compare flag value
  // every clock cycle is 16,000,000 increments in one second
  // for 8-bit counter registers, it can hold up to 255; Then 16,000,000/255 = 62500 interupts
  // for 16-bit, it can hold up to 65535; Then 16,000,000/65535 = 244 interupts
  // a prescaler of 64 would be a good option here.
  
  timerValue = ((F_CPU / (64 * fs)) - 1);
  scalingFactor = 2 * fs;
  if(timerValue >= 65535 || timerValue < 1)
  {
    Serial.println("Error, timer value incorrect.");
    Serial.print("Sample rate should be faster than 4Hz. You requested: "); 
    Serial.println(fs);
    Serial.println("Speeds above 2KHz are untested and not recommended");
    Serial.println("=================");
    Serial.println("connection closed. Restart Serial connection to reset logger");
    delay(100);
    exit(0);
  }  
}



void setTimerInterrupts(int8_t mode)
{ // set timer compare value
  switch(mode)
  {
    case 0:
      getOCR(sample_rate);
      break;
    case 1:      
      Serial.println("Logger ready!");
      while(Serial.available() == 0){} // make sure there are number of bytes (characters) that available for reading from the serial port
      sample_rate = Serial.parseInt(); 
      Serial.flush();//Waits for the transmission of outgoing serial data to complete
      getOCR(sample_rate);
      break;
  };
  //set-up the interrupts on the 2560
  cli();
  TCCR1A = 0; //set entire TCCR2A register to 0
  TCCR1B = 0; // same for TCCR2B
  TCNT1  = 0; //initialize counter value to 0
  TCCR1B |= (1 << WGM12); //turn on CTC mode
  TCCR1B |= (1 << CS01) | (1 << CS00); //set prescaler
  TIMSK1 |= (1 << OCIE1A); // set timer compare match interrupt
  OCR1A = timerValue; //timer interrupt value
  sei();
}

// I change the format of the output data, we might don't need this part.
ISR(TIMER1_COMPA_vect)
{ //define interrupt service routine
  //t1 = micros();
  readSensors(dataBuf);
  //t_end = micros() - t1;
  //Serial.println(t_end);
}

// -------------------- Setup --------------------
void setup() 
{
  //start serial
  Serial.begin(250000);  
  setTimerInterrupts(mode);
    // initialize the LED pin as an output:

  pinMode(ledPin, OUTPUT);

  // initialize the pushbutton pin as an input:

  pinMode(BUTTON, INPUT);

}

// -------------------- Main Loop --------------------
void loop()
{
  if ((dataBuf.bufferPointer >=  49) && (dataBuf.bufferMarker == 0)) 
  { //time to switch buffer0 to buffer1
    if(dataBuf.buffer1State == 1)  //check if buffer1 is dirty before switching
    {
      Serial.println("buffer0 overflow"); //report error if dirty
      delay(20); //give the processor some time to finish serial print before halting
      exit(0); //halt processor
    } else 
    { //if switching is possible
      dataBuf.buffer0State = 1; //mark buffer0 dirty
    }
    dataBuf.bufferMarker = 1; //set buffer flag to buffer1
    dataBuf.bufferPointer = 0; //reset datapoint bufferPointer
    for (int i = 0; i < 49; i++) { //transmit contents of buffer0
      Serial.print(dataBuf.hrdata0[i]);
      Serial.print(",");
      Serial.print(dataBuf.gsrdata0[i]);
      Serial.print(",");
      Serial.print(dataBuf.emgdata0[i]);
      Serial.print(",");
      Serial.print(dataBuf.buttondata0[i]);
      Serial.println(",");
    }
    dataBuf.buffer0State = 0; //release buffer0 after data tranmission, mark as clean
    //here follows same as above, except with reversed buffer order
  } else if ((dataBuf.bufferPointer >= 49) && (dataBuf.bufferMarker == 1)) 
  {
    if(dataBuf.buffer0State == 1)
    {
      Serial.println("buffer1 overflow");
      delay(20);
      exit(0);
    } else 
    {
      dataBuf.buffer1State = 1;
    }
    dataBuf.bufferMarker = 0;
    dataBuf.bufferPointer = 0;
    for (int i = 0; i < 49; i++) 
    {
      Serial.print(dataBuf.hrdata1[i]);
      Serial.print(",");
      Serial.print(dataBuf.gsrdata1[i]);
      Serial.print(",");
      Serial.print(dataBuf.emgdata1[i]);
      Serial.print(",");
      Serial.print(dataBuf.buttondata1[i]);
      Serial.println(",");
    }
    dataBuf.buffer1State = 0;
  }
}
