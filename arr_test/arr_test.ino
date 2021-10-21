void setup() {
  Serial.begin(9600);
  char time_sections [3] = {'hour','minute','tod'};
  Serial.println(time_sections[0]);
  Serial.println(time_sections[1]);
}

void loop() {
  // put your main code here, to run repeatedly:

}
