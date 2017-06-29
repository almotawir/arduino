int capteur = 0;
 int note = 330;
 
void setup(){
  Serial.begin(9600);
  pinMode(4,OUTPUT); //LED VERTE
  pinMode(8,OUTPUT); //LED ROUGE
}

void loop(){
  int valcapt = analogRead(capteur);
  float voltage =  valcapt*5.0/1024.0;
  float temperature = (voltage - 0.5)*100;
  Serial.print( "temperature : ");
  Serial.print(temperature);
   Serial.println(" C ");
  delay(2000);
  
  if( temperature > 28)
  {
    digitalWrite(4,LOW); //  VERTE ETEINTE
    digitalWrite(8,HIGH); //  ROUGE ALLUMEE
    tone(3, note);
  }
  
  else{
    digitalWrite(4,HIGH); // VERTE ALLUMEE
    digitalWrite(8,LOW); // ROUGE ETEINTE
    noTone(3);
  }
}