#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Servo.h>
#include <AccelStepper.h>

// Servo Driver
Adafruit_PWMServoDriver srituhobby = Adafruit_PWMServoDriver();

// Limit Switches
const int limitHomePin = 4;  // Homing switch
const int limitEndPin = 5;   // Emergency stop

// Slider Stepper Motor
const int stepPin = 3;
const int dirPin = 2;
AccelStepper stepper(AccelStepper::DRIVER, stepPin, dirPin);

// Constants
#define STEPS_PER_POSITION_1 224
#define STEPS_PER_POSITION_2 234
#define STEPS_PER_POSITION_3 244
#define STEPS_PER_POSITION_4 250

int currentStepPosition = 0; // Home = 0

// PWM Servo pulse values (adjust as per calibration)
#define servoMIN 150
#define servoMID 375
#define servoMAX 600


// Add this at the top if not already defined
int currentPulse[6] = {425, 425, 375, 150, 150, 220}; // initial/defaults
int stepDelay = 10;  // delay in ms between steps
int stepSize = 2;    // change for speed (1 = very slow/smooth)

void moveServoSlow(int servoNum, int targetPulse) {
  int current = currentPulse[servoNum];

  if (current < targetPulse) {
    for (int p = current; p <= targetPulse; p += stepSize) {
      srituhobby.setPWM(servoNum, 0, p);
      delay(stepDelay);
    }
  } else {
    for (int p = current; p >= targetPulse; p -= stepSize) {
      srituhobby.setPWM(servoNum, 0, p);
      delay(stepDelay);
    }
  }

  currentPulse[servoNum] = targetPulse;
}

void setup() {
  Serial.begin(9600);

  // Limit switch inputs with pullups
  pinMode(limitHomePin, INPUT_PULLUP);
  pinMode(limitEndPin, INPUT_PULLUP);

  // PWM servo driver setup
  srituhobby.begin();
  srituhobby.setPWMFreq(60);

  // Stepper settings
  stepper.setMaxSpeed(800);
  stepper.setAcceleration(400);

  // Initial arm position
  setDefaultPosition();

  // Homing process
  Home();
}

void loop() {
  sort();
}

void sort() {
  setDefaultPosition();
  xPosition();  // Wait for user input for X axis
  yPosition();  // Wait for user input for Y axis
  delay(5000);
  setDefaultPosition();
  camScanner(); // Automated move to Ff
  delay(5000);
  setDefaultPosition()
  camScanner();
  sorter();
  delay(5000);
  // xmove();           // Ask for mm and move
  // dropObject();
  // returnToDefault();
  // xreturn();         // Return to original x
  // delay(1000);
}

// ---------------- ARM AND GRABBER FUNCTIONS ----------------


void setDefaultPosition() {
  delay(500); // short pause before moving

  moveServoSlow(0, 425);
  moveServoSlow(1, 425);
  moveServoSlow(2, 375);
  moveServoSlow(3, 150);
  moveServoSlow(4, 150);
  moveServoSlow(5, 220);

  Serial.println("Arm set to default position");
  delay(500); // short pause after moving
}

// void setDefaultPosition() {
//   delay(1000);
//   srituhobby.setPWM(0, 0, 425);
//   srituhobby.setPWM(1, 0, 425);
//   srituhobby.setPWM(2, 0, 375);
//   srituhobby.setPWM(3, 0, 150);
//   srituhobby.setPWM(4, 0, 150);
//   srituhobby.setPWM(5, 0, 200);
//   Serial.println("Arm set to default position");
//   delay(1000);
// }






void returnToDefault() {
  setDefaultPosition();
}

// ---------------- X-AXIS (SLIDER) FUNCTIONS ----------------

void Home(){
  Serial.println("Homing to mechanical origin (Home = 0)...");
  stepper.setSpeed(-400); // Move RIGHT (because positive goes left)
  while (digitalRead(limitHomePin) == HIGH) {
    stepper.runSpeed();
  }

  stepper.stop();
  stepper.setCurrentPosition(0);  // Set current step = 0 (Home)
  currentStepPosition = 0;
  Serial.println("Homing complete. Now at Home (0 steps).");
}

// Original: User input
void xPosition(){
  Serial.println("Enter target position (A–Q):");
  while (Serial.available() == 0);
  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toUpperCase();
  char target = input.charAt(0);

  if (target < 'A' || target > 'Q') {
    Serial.println("Invalid input. Use A–Q only.");
    return;
  }

  int targetStepPosition = 0;
  int stepsToMove = 0;

  if (target >= 'A' && target <= 'E') {
    targetStepPosition = (target - 'A' + 1) * STEPS_PER_POSITION_1;
  } else if (target >= 'F' && target <= 'J') {
    targetStepPosition = (target - 'A' + 1) * STEPS_PER_POSITION_2;
  } else if (target >= 'K' && target <= 'O') {
    targetStepPosition = (target - 'A' + 1) * STEPS_PER_POSITION_3;
  } else if (target >= 'P' && target <= 'Q') {
    targetStepPosition = (target - 'A' + 1) * STEPS_PER_POSITION_4;
  } else {
    Serial.println("Invalid position.");
    return;
  }

  stepsToMove = targetStepPosition - currentStepPosition;

  Serial.print("Moving to position ");
  Serial.print(target);
  Serial.print(" (Target Steps: ");
  Serial.print(targetStepPosition);
  Serial.print(" | Moving ");
  Serial.print(stepsToMove);
  Serial.println(" steps)");

  stepper.move(stepsToMove);
  while (stepper.distanceToGo() != 0) {
    if (digitalRead(limitEndPin) == LOW) {
      Serial.println("End switch hit. Stopping.");
      stepper.stop();
      break;
    }
    stepper.run();
  }

  currentStepPosition = targetStepPosition;
  Serial.print("Reached position ");
  Serial.print(target);
  Serial.print(" (");
  Serial.print(currentStepPosition);
  Serial.println(" steps)");

}

// Added: Direct X-axis movement function
void moveXTo(char position) {
  char target = toupper(position);
  int targetStepPosition = 0;
  int stepsToMove = 0;

  if (target < 'A' || target > 'Q') {
    Serial.println("Invalid input. Use A–Q only.");
    return;
  }

  if (target >= 'A' && target <= 'E') {
    targetStepPosition = (target - 'A' + 1) * STEPS_PER_POSITION_1;
  } else if (target >= 'F' && target <= 'J') {
    targetStepPosition = (target - 'A' + 1) * STEPS_PER_POSITION_2;
  } else if (target >= 'K' && target <= 'O') {
    targetStepPosition = (target - 'A' + 1) * STEPS_PER_POSITION_3;
  } else if (target >= 'P' && target <= 'Q') {
    targetStepPosition = (target - 'A' + 1) * STEPS_PER_POSITION_4;
  } else {
    Serial.println("Invalid position.");
    return;
  }

  stepsToMove = targetStepPosition - currentStepPosition;

  Serial.print("Moving to position ");
  Serial.print(target);
  Serial.print(" (Target Steps: ");
  Serial.print(targetStepPosition);
  Serial.print(" | Moving ");
  Serial.print(stepsToMove);
  Serial.println(" steps)");

  stepper.move(stepsToMove);
  while (stepper.distanceToGo() != 0) {
    if (digitalRead(limitEndPin) == LOW) {
      Serial.println("End switch hit. Stopping.");
      stepper.stop();
      break;
    }
    stepper.run();
  }

  currentStepPosition = targetStepPosition;
  Serial.print("Reached position ");
  Serial.print(target);
  Serial.print(" (");
  Serial.print(currentStepPosition);
  Serial.println(" steps)");
}

// ---------------- Y-AXIS (ARM) FUNCTIONS ----------------

// Original: User input
void yPosition() {
  Serial.println("Enter target position (a-h):");
  while (Serial.available() == 0);
  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toLowerCase();
  char position = input.charAt(0);

  if (position < 'a' || position > 'h') {
    Serial.println("Invalid input. Use a-h only.");
    return;
  }

  switch (position) {
    case 'a': s1(); 
              break;
    case 'b': s2(); 
              break;
    case 'c': s3(); 
              break;
    case 'd': s4(); 
              break;
    case 'e': s5(); 
              break;
    case 'f': s6(); 
              break;
    case 'g': s7(); 
              break;
    case 'h': s8(); 
              break;
  };
}

// Added: Direct Y-axis movement function
void moveYTo(char position) {
  char pos = tolower(position);
  if (pos < 'a' || pos > 'h') {
    Serial.println("Invalid input. Use a-h only.");
    return;
  }
  switch (pos) {
    case 'a': s1(); break;
    case 'b': s2(); break;
    case 'c': s3(); break;
    case 'd': s4(); break;
    case 'e': s5(); break;
    case 'f': s6(); break;
    case 'g': s7(); break;
    case 'h': s8(); break;
  }
}

// ---------------- DUMMY OBJECT DETECTION FUNCTION ----------------

void s1() {
  //Open the Grabber
  srituhobby.setPWM(0, 0, 300); delay(60);
  srituhobby.setPWM(1, 0, 460); delay(60);
  srituhobby.setPWM(2, 0, 150); delay(60);
  srituhobby.setPWM(3, 0, 458); delay(60);
  srituhobby.setPWM(4, 0, 250); delay(60);
  srituhobby.setPWM(5, 0, 200); delay(60);
  Serial.println("Moved to Position A1");
  delay(500);
  //CLose the Grabber
  srituhobby.setPWM(0, 0, 150); delay(60);
  Serial.println("Grab the Object from Position a");
  delay(500);
}

void s2() {
  srituhobby.setPWM(0, 0, 300); delay(60);
  srituhobby.setPWM(1, 0, 420); delay(60);
  srituhobby.setPWM(2, 0, 150); delay(60);
  srituhobby.setPWM(3, 0, 340); delay(60);
  srituhobby.setPWM(4, 0, 220); delay(60);
  srituhobby.setPWM(5, 0, 200); delay(60);
  Serial.println("Moved to Position A2");
  delay(500);
  //CLose the Grabber
  srituhobby.setPWM(0, 0, 150); delay(60);
  Serial.println("Grab the Object from Position b");
  delay(500);
}

void s3() {
  srituhobby.setPWM(0, 0, 300); delay(30);
  srituhobby.setPWM(1, 0, 430); delay(30);
  srituhobby.setPWM(2, 0, 150); delay(30);
  srituhobby.setPWM(3, 0, 310); delay(30);
  srituhobby.setPWM(4, 0, 233); delay(30);
  srituhobby.setPWM(5, 0, 200); delay(30);
  Serial.println("Moved to Position A3");
  delay(500);
}

void s4() {
  srituhobby.setPWM(0, 0, 300); delay(30);
  srituhobby.setPWM(1, 0, 395); delay(30);
  srituhobby.setPWM(2, 0, 150); delay(30);
  srituhobby.setPWM(3, 0, 280); delay(30);
  srituhobby.setPWM(4, 0, 250); delay(30);
  srituhobby.setPWM(5, 0, 200); delay(30);
  Serial.println("Moved to Position A4");
  delay(500);
}

void s5() {
  srituhobby.setPWM(0, 0, 300); delay(30);
  srituhobby.setPWM(1, 0, 305); delay(30);
  srituhobby.setPWM(2, 0, 150); delay(30);
  srituhobby.setPWM(3, 0, 270); delay(30);
  srituhobby.setPWM(4, 0, 300); delay(30);
  srituhobby.setPWM(5, 0, 200); delay(30);
  Serial.println("Moved to Position A5");
  delay(500);
}

void s6() {
  srituhobby.setPWM(0, 0, 300); delay(30);
  srituhobby.setPWM(1, 0, 295); delay(30);
  srituhobby.setPWM(2, 0, 150); delay(30);
  srituhobby.setPWM(3, 0, 285); delay(30);
  srituhobby.setPWM(4, 0, 337); delay(30);
  srituhobby.setPWM(5, 0, 200); delay(30);
  Serial.println("Moved to Position A6");
  delay(500);
}

void s7() {
  srituhobby.setPWM(0, 0, 300); delay(30);
  srituhobby.setPWM(1, 0, 295); delay(30);
  srituhobby.setPWM(2, 0, 150); delay(30);
  srituhobby.setPWM(3, 0, 350); delay(30);
  srituhobby.setPWM(4, 0, 430); delay(30);
  srituhobby.setPWM(5, 0, 200); delay(30);
  Serial.println("Moved to Position A7");
  delay(500);
}

void s8() {
  srituhobby.setPWM(0, 0, 300); delay(30);
  srituhobby.setPWM(1, 0, 370); delay(30);
  srituhobby.setPWM(2, 0, 150); delay(30);
  srituhobby.setPWM(3, 0, 484); delay(30);
  srituhobby.setPWM(4, 0, 530); delay(30);
  srituhobby.setPWM(5, 0, 200); delay(30);
  Serial.println("Moved to Position A8");
  delay(500);
}

bool isObjectAvailable() {
  // Replace with camera/sensor logic later
  return true;  // For demo: always true
}

// ---------------- camScanner Automated Ff Function ----------------

void camScanner() {
  Serial.println("Moving to Ff position under camScanner...");
  moveXTo('F');    // Move slider to F
  moveYTo('f');    // Move arm to f
  Serial.println("Arrived at Ff position for scanning.");
  // Add any camera/sensor logic here if needed
  delay(1000);     // Simulate scanning time
}


void s3() {
  srituhobby.setPWM(0, 0, 300); delay(30);
  srituhobby.setPWM(1, 0, 430); delay(30);  // 333
  srituhobby.setPWM(2, 0, 150); delay(30);
  srituhobby.setPWM(3, 0, 310); delay(30);  // 173
  srituhobby.setPWM(4, 0, 233); delay(30);  // 78
  srituhobby.setPWM(5, 0, 200); delay(30);
  Serial.println("Moved to Position A3");
  delay(500);
  srituhobby.setPWM(0, 0, 150); delay(30);
  delay(500);
}