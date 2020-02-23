#include "Button.h"

Button btn1(6);
Button btn2(5);
Button btn3(4);
Button btn4(3);
Button btn5(2);

void setup() {
  Serial.begin(9600);

  btn1.initialise();
  btn2.initialise();
  btn3.initialise();
  btn4.initialise();
  btn5.initialise();
}

void loop() {
  btn1.update();
  btn2.update();
  btn3.update();
  btn4.update();
  btn5.update();
  
  if (btn1.pressed())
    Serial.println("b1");

  if (btn2.pressed())
    Serial.println("b2");

  if (btn3.pressed())
    Serial.println("b3");

  if (btn4.pressed())
    Serial.println("b4");

  if (btn5.pressed())
    Serial.println("b5");  
}
