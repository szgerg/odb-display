#include "beep.h"
void setup()
{
}

void loop()
{
  delay(1000);
  beep_start();
  delay(1000);
  beep_stop();
}
