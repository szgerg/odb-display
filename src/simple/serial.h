void serial_init(int baud_rate = 115200)
{
  Serial.begin(baud_rate);
  Serial.println("[WDT] Serial initialized.");
}