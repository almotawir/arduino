#include <SoftwareSerial.h>
#include <OneWire.h>
#include <Time.h>
#define DS18B20 0x28
#define BROCHE_ONEWIRE 4
#define SEUIL_BAS 15.0
#define SEUIL_HAUT 25.0
#define TEMPS_ATTENTE 600 // Temps d'attente entre deux mesures de la température (en sec)
#define TEMPS_ATTENTE_SECURITE 60 // Temps d'attente pour la mesure de vérification (en sec)
#define TEMPS_ATTENTE_VEILLE 60 // Temps d'attente entre deux vérifications du réseau mobile (en sec)
#define NOMBRE_CAPTEURS_MAX 10 // Nombre maximum de capteurs en série autorisé (valeur >= 1)
#define SIM_PIN_CODE "0000" // Code Pin de la carte SIM
 
// Variables relatives aux capteurs
OneWire ds(BROCHE_ONEWIRE);
byte adresseCapteurs[NOMBRE_CAPTEURS_MAX][8]; // Adresse des capteurs
float temperatureCapteurs[NOMBRE_CAPTEURS_MAX];
boolean intervalleCapteurs[NOMBRE_CAPTEURS_MAX];
time_t tempsPrecedentCapteurs[NOMBRE_CAPTEURS_MAX];
int nombre_capteurs = 0;
int capteurMesure = 0;
 
// Variables relatives au module GPRS
SoftwareSerial GPRS (7, 8); // GPRS : relatif au shield GPRS ; Serial : relatif à l'ordinateur
String phone_number = "+212666176051"; // Numéro par défaut
String msg = ""; // Mémoire tampon de la communication avec le module GPRS
int CorpsSMS = false; // Est mis à 1 quand le prochain message du shield GPRS contiendra le contenu du SMS
String numeroSMS;
String numeroTemp = phone_number;
boolean veille = true;
time_t tempsPrecedentVeille;
 
void setup() {
delay (3000);
GPRS.begin(19200); // Fréquence du module
tempsPrecedentVeille = now();
// Setup Capteurs
int i=0;
Serial.begin(9600); // Initialisation du port série
ds.reset();
ds.reset_search();
while ((nombre_capteurs < NOMBRE_CAPTEURS_MAX) && (ds.search(adresseCapteurs[nombre_capteurs]))) // Recherche des différents capteurs
{
tempsPrecedentCapteurs[nombre_capteurs] = now(); // Initialisation des variables du capteur
temperatureCapteurs[nombre_capteurs] = (SEUIL_BAS + SEUIL_HAUT) / 2;
intervalleCapteurs[nombre_capteurs] = true;
nombre_capteurs++;
}
}
 
void loop() {
if (GPRS.available()) {
executionGPRS(); }
else {
time_t time = now();
if ((time-tempsPrecedentVeille) >= TEMPS_ATTENTE_VEILLE)<
{
reseauIndisponible(); // Vérification du réseau
tempsPrecedentVeille = time;
}
if (!veille) {
if ((time-tempsPrecedentCapteurs[capteurMesure]) >= TEMPS_ATTENTE)
{
mesureCapteur(capteurMesure); // Mesure sur le capteur
tempsPrecedentCapteurs[capteurMesure] = time;
}
capteurMesure++;
      if (capteurMesure == nombre_capteurs) {
capteurMesure = 0; }
}
}
}
 
void mesureCapteur (const int i) // Demande de la température du capteur i
{
getTemperature (&temperatureCapteurs[i], adresseCapteurs[i]);
if (intervalleCapteurs[i] & ((temperatureCapteurs[i] < SEUIL_BAS) | (temperatureCapteurs[i] > SEUIL_HAUT))) // On sort de l'intervalle acceptable
{
delay (TEMPS_ATTENTE_SECURITE*1000); // Attente
tempsPrecedentCapteurs[i] = now();
getTemperature (&temperatureCapteurs[i], adresseCapteurs[i]);
if ((temperatureCapteurs[i] < SEUIL_BAS) | (temperatureCapteurs[i] > SEUIL_HAUT)) // Seconde vérification
{
reseauIndisponible(); // Vérification du réseau
tempsPrecedentVeille = now();
if (!veille) {
intervalleCapteurs[i]=false;
envoyerUnMessage (temperatureCapteurs[i], true, i+1);
}
}
}
else if (!intervalleCapteurs[i] & (temperatureCapteurs[i] > SEUIL_BAS) & (temperatureCapteurs[i] < SEUIL_HAUT)) // On rentre à nouveau dans cet intervalle
{
delay (TEMPS_ATTENTE_SECURITE*1000); // Attente
tempsPrecedentCapteurs[i] = now();
getTemperature (&temperatureCapteurs[i], adresseCapteurs[i]);
if ((temperatureCapteurs[i] > SEUIL_BAS) & (temperatureCapteurs[i] < SEUIL_HAUT)) // Seconde vérification
{
reseauIndisponible(); // Vérification du réseau
tempsPrecedentVeille = now();
if (!veille) {
intervalleCapteurs[i]=true;
envoyerUnMessage (temperatureCapteurs[i], true, i+1);
}
}
}
}
 
void getTemperature (float *temp, const byte addr[]) {
byte data[2];
ds.reset();
ds.select(addr); // Sélectionne le capteur de température
ds.write(0x44,1); // Lance la mesure de température
delay(800);
ds.reset(); // Reset pour envoyer maintenant la demande de lecture
ds.select(addr);
ds.write(0xBE); // Demande de lecture du scratchpad
data[0] = ds.read(); // Lit les 2 premiers octets du scratchpad sur lesquels est contenue la température
data[1] = ds.read();
*temp = ((data[1] << 8) | data[0]) * 0.0625; // Conversion en degrés celsius
}
 
void executionGPRS () {
char SerialInByte = (unsigned char)GPRS.read(); // Réception du caractère envoyé par le module GPRS
if (SerialInByte == 13) { // Si le message se termine par un <CR> alors on traite le message
gestionCommunicationGPRS();
}
else if (SerialInByte != 10) { // On ignore le caractère interligne
msg += String(SerialInByte); // Stockage du caractère dans la mémoire tampon
}
}
 
void gestionCommunicationGPRS () { // Interprete le message du GPRS shield et agit en conséquence
if (msg.indexOf("+CPIN: SIM PIN") >= 0) { // Demande du code Pin de la carte SIM
GPRS.print("AT+CPIN="); // Envoi du code Pin
GPRS.println( SIM_PIN_CODE );
}
if (msg.indexOf("Call Ready") >= 0) { // Le réseau mobile est disponible
GPRS.println("AT+CMGF=1"); } // Utilisation du mode texte pour la gestion des messages SMS
// Opérations relatives à la réception d'un SMS
if (msg.indexOf("+CMTI") >= 0) { // Phase 1 : début de réception d'un SMS
int iPos = msg.indexOf(",");
numeroSMS = msg.substring(iPos+1);
GPRS.print("AT+CMGR="); // Demande de lecture des informations du message
GPRS.println(numeroSMS);
  }
if (msg.indexOf("+CMGR:") >= 0) { // Phase 2 : réception du numéro de l'expéditeur
int iPos = msg.indexOf("+33");
numeroTemp = msg.substring(iPos, iPos+12);
CorpsSMS = true;
msg = "";
return; } // Le prochain message contiendra le corps du SMS
if (CorpsSMS == 1) { // Phase 3 : corps du SMS
traiterSMS(msg); // Traitement du corps du message
GPRS.print("AT+CMGD="); // Suppression du SMS pour ne pas encombrer la carte SIM
GPRS.println(numeroSMS);
delay(1000);
}
msg = ""; // Efface le contenu de la mémoire tampon des messages du GPRS shield
CorpsSMS = false;
}
 
void traiterSMS(const String smsText) {
for (int i=0; i < nombre_capteurs; i++) {
if (smsText.indexOf("Temp "+String(i+1)) >= 0) {
// Demande d'envoi de la température du capteur i
phone_number = numeroTemp; // Remplacement du numéro
envoyerUnMessage (temperatureCapteurs[i], false, i+1);
delay(1000);
}
  }
}
 
void envoyerUnMessage(const float temp, const boolean messageAutomatique, const int numeroCapteur) {
delay(500);
GPRS.print("AT+CMGS=\""); // Parametrage du numéro
GPRS.print(phone_number);
GPRS.print("\"\r\n");
while(GPRS.read()!='>');
if (messageAutomatique) { // Corps du SMS : 3 possibilités
if (!intervalleCapteurs[numeroCapteur-1]) {
GPRS.print("Attention, le seuil critique de temperature dans le capteur ");
GPRS.print(numeroCapteur);
GPRS.print(" a ete atteint. La temperature est de ");
}
else {
GPRS.print("La temperature dans le capteur ");
GPRS.print(numeroCapteur);
GPRS.print(" est de nouveau acceptable. Elle est de ");
}  }
  else {
GPRS.print("La temperature dans le capteur ");
GPRS.print(numeroCapteur);
GPRS.print(" est de ");
}
GPRS.print(temp, 2);
GPRS.print(" degres celsius.");
delay(500);
GPRS.write(0x1A); // Finalisation du SMS et envoi
GPRS.write(0x0D);
GPRS.write(0x0A);
delay(20000);
}
 
void reseauIndisponible() // Gère le réseau mobile {
GPRS.print("AT+CREG?\r\n"); // Demande de l'enregistrement sur le réseau
while (!GPRS.available());
while (GPRS.read() != 44); // Jusqu'à la virgule ','
veille = GPRS.read() != 49; // Analyse du résultat : ‘1’ ou pas
}