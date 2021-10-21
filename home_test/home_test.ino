#define HOME_PIN A0

void setup() {
  Serial.begin(9600);

}

void loop() {
  int home_data = analogRead(HOME_PIN);
  Serial.println(home_data);
  delay(250);
}
