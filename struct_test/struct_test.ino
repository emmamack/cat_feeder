struct Time {
  byte hour;
  byte minute;
  byte second;
};

void setup() {
  Serial.begin(9600);
  Time my_time = { 3, 6, 30 };
  Serial.println(my_time[1]);

}

void loop() {
  // put your main code here, to run repeatedly:

}
