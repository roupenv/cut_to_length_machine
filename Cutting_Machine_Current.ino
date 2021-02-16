/******Libraries***********/
#include <SoftReset.h>      //Reset Arduino Library
#include <MenuBackend.h>    //MenuBackend library - copyright by Alexander Brevig
#include <LiquidCrystal.h>  //this library is included in the Arduino IDE
#include <AccelStepper.h>
#include <Wire.h>
#include <elapsedMillis.h>
/*************************/


/*******Define Variables*********/
AccelStepper stepper(1, 31, 33); // Define a stepper motor 1 for arduino // direction Digital 31 PUL- (CW), pulses Digital 33 DIR- (CLK)

const byte interruptPin = 18;

static void menuUsed(MenuUseEvent used);
static void menuChanged(MenuChangeEvent changed);

const byte stepperRelay = 8;  //Stepper relay hooked up to pin 38
const byte stepperMicroStep = 8; //Stepper set to 8 microsteps per step
const byte solenoidRelay = 9; //Pnuematic solenoid relay hooked up to pin 39

//Variable Material Cutting Parameter
float materialLength = 1.25;
int materialQuantity = 10;
float offset = 60;


//LED
elapsedMillis ledTimeElapsed;
bool ledPinState = 0;
const byte testLED = 13;

//Directional Navigation Pins
const int buttonPinLeft = 51;      // pin for the Left button
const int buttonPinRight = 53;    // pin for the Right button
const int buttonPinEsc = 50;     // pin for the Esc button
const int buttonPinEnter = 52;   // pin for the Enter button


int lastButtonPushed = 0;
int lastButtonEnterState = LOW;   // the previous reading from the Enter input pin
int lastButtonEscState = LOW;   // the previous reading from the Esc input pin
int lastButtonLeftState = LOW;   // the previous reading from the Left input pin
int lastButtonRightState = LOW;   // the previous reading from the Right input pin


long lastEnterDebounceTime = 0;  // the last time the output pin was toggled
long lastEscDebounceTime = 0;  // the last time the output pin was toggled
long lastLeftDebounceTime = 0;  // the last time the output pin was toggled
long lastRightDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 200;    // the debounce time

// LiquidCrystal display with:
// rs=pin 7 ,rw = ground, enable=pin 12, d4=11, d5=10, d6=9, d7=8
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

//Menu variables
MenuBackend menu = MenuBackend(menuUsed, menuChanged);
//initialize menuitems
MenuItem menu1Item1 = MenuItem("1. Cut Settings");
MenuItem menuItem1SubItem1 = MenuItem("1.1 Enter Length");
MenuItem menuItem1SubItem2 = MenuItem("1.2 Enter Qty");
MenuItem menu1Item2 = MenuItem("2. Initialize");       //MenuItem menu1Item2 = MenuItem("2. Start Cutting");
MenuItem menuItem2SubItem1 = MenuItem("2.1 Start");       //MenuItem menuItem2SubItem1 = MenuItem("2.1 Start");
MenuItem menu1Item3 = MenuItem("3. Start Cutting");          //MenuItem menu1Item3 = MenuItem("3. Initialize");
MenuItem menuItem3SubItem1 = MenuItem("3.1 Start");       //MenuItem menuItem3SubItem1 = MenuItem("3.1 Start");
/*****************************************************/


void setup()
{
  Wire.begin();               //start communication with I2C
  Serial.begin(115200);      // start communication with serial at 115200 baud

  //Interrupt Setup
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), coverOff, LOW);  //Interupt

  //Configure Navigation push buttons as input
  pinMode(buttonPinLeft, INPUT);
  pinMode(buttonPinRight, INPUT);
  pinMode(buttonPinEnter, INPUT);
  pinMode(buttonPinEsc, INPUT);

  pinMode(testLED, OUTPUT);//LED as output

  //Configure control units as output
  pinMode(solenoidRelay, OUTPUT);
  pinMode(stepperRelay, OUTPUT);

  digitalWrite(solenoidRelay, HIGH); //Ensure Solenoid is Off
  digitalWrite(stepperRelay, HIGH); //Ensure Stepper is Off

  
  //Configure LCD Menu
  lcd.begin(16, 2); //LCD setup having 16 Columns, 2 Rows
  menu.getRoot().add(menu1Item1);
  menu1Item1.addRight(menu1Item2).addRight(menu1Item3);
  menu1Item1.add(menuItem1SubItem1).addRight(menuItem1SubItem2);
  menu1Item2.add(menuItem2SubItem1);
  menu1Item3.add(menuItem3SubItem1);
  menu.toRoot();
  lcd.setCursor(0, 0);
  lcd.print("Cutting Machine");

}


/*****Start Read Button Main Loop*********/
void loop()
{
  readButtons();  //I splitted button reading and navigation in two procedures because
  navigateMenus();  //in some situations I want to use the button for other purpose (eg. to change some settings)
  
  //Soft Reset if all 4 navigational buttons are pushed
  if ((digitalRead(buttonPinEnter) == HIGH) && (digitalRead(buttonPinLeft) == HIGH) && (digitalRead(buttonPinRight) == HIGH) && (digitalRead(buttonPinEsc) == HIGH) )
  {
   soft_restart();
  }
}
/******End Loop*********/





/******Define All Functions*********/

// Interupt Emergency
void coverOff() {

}

//Convert Inches/second to Stepper Motor Steps/Second, take into consideration roller size & gear ratio
float rollerSpeed(int inchsec )
{
  float stepsSec; //Calculate number of steps
  stepsSec = rollerDistance(inchsec) / 60;
  return stepsSec;
}


//Convert Inches to Stepper Motor Steps
int rollerDistance (float inch )
{
  //Roller diameter=7.5inches, Gear Ratio=2:1, 200 steps=1 rotation Stepper => 0.5 rotations Roller
  int steps; //Calculate number of step
  

  //Circum=20.8cm=8.19inch   Drive Wheel= 400 steps/revolution  or 400steps/8.19inch= 48.84 steps/inch   Driver set to 8 Microsteps/1 step so must multiply
  steps = (-inch * 49.0 * stepperMicroStep);
  steps=steps-offset;
  Serial.println(steps);
  return steps;
}


void fractionOutput (float currentMaterialQuantity )
{

String fraction;
int wholeNumber = (int)currentMaterialQuantity;
float decimal=currentMaterialQuantity-wholeNumber;

Serial.println(decimal);

if (decimal == 0.125)
{
  fraction="1/8";
}
else if (decimal == 0.25)
{
  fraction="1/4";
}
else if (decimal == 0.375)
{
  fraction="3/8";
}
else if (decimal == 0.50)
{
  fraction="1/2";
}
else if (decimal == 0.625)
{
  fraction="5/8";
}
else if (decimal == 0.75)
{
  fraction="3/4";
}
else if (decimal == 0.875)
{
  fraction="7/8";
}
else if (decimal == 0.00)
{
  fraction=".00";
}
  
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(wholeNumber);
  lcd.setCursor(3, 1);
  lcd.print("-");
  lcd.setCursor(4, 1);
  lcd.print(fraction);
  lcd.setCursor(7, 1);
  lcd.print("in.");
  //lcd.setCursor(0, 3);
  //lcd.print(decimal);
  
  
}



//Function to Blink LED
void ledBlink(int ledPin, int blinkInterval)
{

  if (ledTimeElapsed > blinkInterval )
  {
    ledPinState = !ledPinState;
    digitalWrite(ledPin, ledPinState);
    ledTimeElapsed = 0;
  }
}

// 0 To turn On and 1 to turn off
void relayAction(int type , boolean state) 
{
  if (type == "solenoid")
  {
    digitalWrite(solenoidRelay, state);
  }
  else if (type == "stepper")
  {
    digitalWrite(stepperRelay, state);
  }
}


void menuChanged(MenuChangeEvent changed) {

  MenuItem newMenuItem = changed.to; //get the destination menu

  lcd.setCursor(0, 1); //set the start position for lcd printing to the second row

  if (newMenuItem.getName() == menu.getRoot()) {
    lcd.print("Main Menu       ");
  } else if (newMenuItem.getName() == "1. Cut Settings") {
    lcd.print("1. Cut Settings ");
  } else if (newMenuItem.getName() == "1.1 Enter Length") {
    lcd.print("1.1 Enter Length");
  } else if (newMenuItem.getName() == "1.2 Enter Qty") {
    lcd.print("1.2 Enter Qty   ");
  } else if (newMenuItem.getName() == "2. Initialize") {
    lcd.print("2. Initialize   ");
  } else if (newMenuItem.getName() == "2.1 Start") {
    lcd.print("2.1 Start       ");
  } else if (newMenuItem.getName() == "3. Start Cutting") {
    lcd.print("3. Start Cutting");
  } else if (newMenuItem.getName() == "3.1 Start") {
    lcd.print("3.1 Start       ");
  }
}


void menuUsed(MenuUseEvent used) {
  //menu.toRoot();  //back to Main
  // lcd.setCursor(0, 0);
  //lcd.print("You used        ");


/****************Cutting Subroutine************************/
  if ((used.item.getName()) == "3.1 Start")
  {
    float precentDone;
    //LCD Screen Print Updates
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Material Qty=");
    lcd.setCursor(0, 1);
    lcd.print("Completion=");
    relayAction("solenoid", 1); //Enrure solenoid is off and blade is up ready to cut

    
      for (int i = 1; i <= materialQuantity; i++)
      {
        if (digitalRead(buttonPinEsc) == HIGH)  //Exit if Escape button is held
        {
          break;
        }
        
        precentDone = ( ( (float) i ) / ( (float) materialQuantity)) * 100;   //cast i & materialQuantity as float to carry out calculation
        
        //Initialize Stepper Motor Settings
        stepper.moveTo(rollerDistance(materialLength));  //Enter number of Inches  rollerDistance(materialLength)
        relayAction("stepper", 0); //Turn On Stepper
        stepper.setAcceleration(800);
        stepper.setMaxSpeed(900); //Enter Inch per second at roller  MAX 15  rollerSpeed(2) 1800 
        //delay(10); //Give enough time to Make Sure Stepper is On


        //Must use "while" to make stepper.run() funciton work. Keep it short inside loop
        while (stepper.distanceToGo() != 0)
        {
          stepper.run();
        }//end while loop


        //Print Status to LCD
        lcd.setCursor(13, 0);
        lcd.print("   ");
        lcd.setCursor(13, 0);
        lcd.print(i);
        lcd.setCursor(11, 1);
        lcd.print("      ");
        lcd.setCursor(11, 1);
        lcd.print(precentDone);
        lcd.setCursor(15, 1);
        lcd.print("%");

        //New Absolute position is reset to 0
        stepper.setCurrentPosition(0); //Reset Stepper Motor to Zero
        
        delay(350);// Wait for Plastic to Recover 
        //Solenoid Action
        relayAction("solenoid", 0); //Turn on solenoid valve
        delay(250); //Wait to cut material
        relayAction("solenoid", 1); //Turn off solenoid valve
        delay(250); //Wait for piston to return home

      } //end for loop

    //Finish Cutting Sequence
    digitalWrite(testLED, LOW);
    relayAction("stepper", 1); //Turn off stepper   
    lcd.clear();
    lcd.print("Cutting Machine ");
    lcd.setCursor(0, 1);
    lcd.print("Cutting Complete");
    delay(3000); //Show message
    menu.toRoot();  //back to Main

  }



 /****************Initialize Subroutine************************/

  if ((used.item.getName()) == "2.1 Start")
  {
    //LCD Screen Print Updates
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Start Initialize");
    lcd.setCursor(0, 1);
    lcd.print("Hold Red to Exit");
    stepper.setAcceleration(3000);
    stepper.setMaxSpeed(1800); //Enter Inch per second at roller  MAX 15  rollerSpeed(2)
    //relayAction("solenoid", 0); //Turn on solenoid valve
    delay(1000);


    do   //Execute code at least once, until Esc key is hit
    {
      if (digitalRead(buttonPinRight) == HIGH)
      {
        stepper.move(rollerDistance(0.125)); //Relative Movement
        relayAction("stepper", 0); //Turn On Stepper
        while (stepper.distanceToGo() != 0)
        {
          stepper.run();
        }
      }
      else if (digitalRead(buttonPinLeft) == HIGH)
      {
        stepper.moveTo(rollerDistance(0));  //Absolute Movement
        relayAction("stepper", 0); //Turn On Stepper
        while (stepper.distanceToGo() != 0)
        {
          stepper.run();
        }
        stepper.setCurrentPosition(0); //Reset Stepper Motor to Zero
      }
      else if (digitalRead(buttonPinEnter) == HIGH)
      {
        relayAction("solenoid", 1); //Turn off solenoid valve, Bring Blade Up
        delay(500);
        stepper.move(rollerDistance(1)); //Relative Movement Feed Material 
        relayAction("stepper", 0); //Turn On Stepper
        while (stepper.distanceToGo() != 0)
        {
          stepper.run();
        }

        relayAction("solenoid", 0); //Turn on solenoid valve, Cut Material
        delay(500); //Wait to cut material
        relayAction("solenoid", 1); //Turn off solenoid valve, Bring Blade Up
        delay(1000); //Wait for piston to return home

        stepper.setCurrentPosition(0); //Reset Stepper Motor to Zero
        break; //Exit Loop
      }

      delay(80);  //Wait


    } while (digitalRead(buttonPinEsc) == LOW);  //Execute Loop at least once until Esc button is presed

    relayAction("stepper", 1); //Turn off stepper
    stepper.setCurrentPosition(0); //Reset Stepper Motor to Zero
    lcd.clear();
    lcd.print("Cutting Machine ");
    lcd.setCursor(0, 1);
    lcd.print("Initialize Compl");
    delay(3000); //Wait for Message to show
    menu.toRoot();  //back to Main
  }



 /****************Set Length Subroutine************************/

  if ((used.item.getName()) == "1.1 Enter Length")
  {
    lcd.setCursor(0, 0);
    lcd.print("Enter Length    ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(materialLength);
    do
    {
      digitalWrite(testLED, HIGH);
      delay(100);
      digitalWrite(testLED, LOW);
      delay(100);

      readButtons();

      if (digitalRead(buttonPinRight) == HIGH)
      {
        materialLength = materialLength + 0.125 ;
        fractionOutput(materialLength);
        /*lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print(materialLength); */
      }
      else if (digitalRead(buttonPinLeft) == HIGH)
      {
        materialLength = materialLength - 0.125; 
        fractionOutput(materialLength);
        /*lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print(materialLength);*/
      }

    } while (digitalRead(buttonPinEsc) == LOW);

    lcd.clear();
    lcd.print("Length = ");
    lcd.print(materialLength,3);
    menu.toRoot();
  }


 /****************Set Quantity Subroutine************************/

  if ((used.item.getName()) == "1.2 Enter Qty")
  {
    lcd.setCursor(0, 0);
    lcd.print("Enter Quantity  ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(materialQuantity);
    do
    {
      digitalWrite(testLED, HIGH);
      delay(100);
      digitalWrite(testLED, LOW);
      delay(100);

      readButtons();

      if (digitalRead(buttonPinRight) == HIGH)
      {
        ++ materialQuantity ;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print(materialQuantity);
      }
      else if (digitalRead(buttonPinLeft) == HIGH)
      {
        -- materialQuantity;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print(materialQuantity);
      }

    } while (digitalRead(buttonPinEsc) == LOW);

    lcd.clear();
    lcd.print("Qty Entered= ");
    lcd.print(materialQuantity);
    menu.toRoot();

  }
  
}


 /****************Read Button Function************************/
void  readButtons()
{
  int reading;
  int buttonEnterState = LOW;           // the current reading from the Enter input pin
  int buttonEscState = LOW;           // the current reading from the input pin
  int buttonLeftState = LOW;           // the current reading from the input pin
  int buttonRightState = LOW;           // the current reading from the input pin

  //Enter button
  // read the state of the switch into a local variable:
  reading = digitalRead(buttonPinEnter);

  // check to see if you just pressed the enter button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonEnterState) {
    // reset the debouncing timer
    lastEnterDebounceTime = millis();
  }

  if ((millis() - lastEnterDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    buttonEnterState = reading;
    lastEnterDebounceTime = millis();
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonEnterState = reading;


  //Esc button
  // read the state of the switch into a local variable:
  reading = digitalRead(buttonPinEsc);

  // check to see if you just pressed the Down button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonEscState) {
    // reset the debouncing timer
    lastEscDebounceTime = millis();
  }

  if ((millis() - lastEscDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    buttonEscState = reading;
    lastEscDebounceTime = millis();
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonEscState = reading;


  //Down button
  // read the state of the switch into a local variable:
  reading = digitalRead(buttonPinRight);

  // check to see if you just pressed the Down button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonRightState) {
    // reset the debouncing timer
    lastRightDebounceTime = millis();
  }

  if ((millis() - lastRightDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    buttonRightState = reading;
    lastRightDebounceTime = millis();
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonRightState = reading;


  //Up button
  // read the state of the switch into a local variable:
  reading = digitalRead(buttonPinLeft);

  // check to see if you just pressed the Down button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonLeftState) {
    // reset the debouncing timer
    lastLeftDebounceTime = millis();
  }

  if ((millis() - lastLeftDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    buttonLeftState = reading;
    lastLeftDebounceTime = millis();;
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonLeftState = reading;

  //records which button has been pressed
  if (buttonEnterState == HIGH) {
    lastButtonPushed = buttonPinEnter;

  } else if (buttonEscState == HIGH) {
    lastButtonPushed = buttonPinEsc;

  } else if (buttonRightState == HIGH) {
    lastButtonPushed = buttonPinRight;

  } else if (buttonLeftState == HIGH) {
    lastButtonPushed = buttonPinLeft;

  } else {
    lastButtonPushed = 0;
  }
}




/****************Navigate Menus Function************************/
void navigateMenus()
{
  MenuItem currentMenu = menu.getCurrent();

  switch (lastButtonPushed) {
    case buttonPinEnter:
      if (!(currentMenu.moveDown())) { //if the current menu has a child and has been pressed enter then menu navigate to item below
        menu.use();
      } else { //otherwise, if menu has no child and has been pressed enter, the current menu is used
        menu.moveDown();
      }
      break;
    case buttonPinEsc:
      menu.toRoot();  //back to main
      break;
    case buttonPinRight:
      menu.moveRight();
      break;
    case buttonPinLeft:
      menu.moveLeft();
      break;
  }

  lastButtonPushed = 0; //reset the lastButtonPushed variable
}


/**************NOTES*********************/
/*

  The motor always starts off at 0 at power on.  move() and moveTo() are a relative and absolute version of the command to set a target position.  The unit is "motor steps".

  stepper.move(5000) will set the target for 5000 steps in one direction (a relative move), so if it is currently at position 2000, it will move to 7000.
  stepper.moveTo(5000) will set the target to position 5000 (absolute position), so if it is currently at position 2000 it will move 3000 extra steps to get to position 5000.

  If you want a constant speed, then you might use .runSpeedToPosition(<position>).   This takes an absolute position as an argument, and runs at the last set speed.  If you want to move to a relative position you can do something like

  stepper.setSpeed(500);
  stepper.runSpeedToPosition(stepper.currentPosition()+2000);

  and of course you can reset the position at any point to any value with stepper.setCurrentPosition(2000).

*/
