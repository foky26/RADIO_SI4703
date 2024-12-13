// LCD  SDA 21   SCL 22
// Rotary  13,12, 14
/*
  | Si470X    | Function              |ESP LOLIN32 WEMOS (GPIO) |
  |-----------| ----------------------|-------------------------|
  | RST/RESET |   RESET               |   25 (GPIO25)           |  
  | SDA/SDIO  |   SDIO                |   21 (SDA / GPIO21)     |
  | CLK/CLOCK |   SCLK                |   22 (SCL / GPIO22)     |
*/

/*
  | TFT        | Function              |ESP LOLIN32 WEMOS (GPIO) |
  |-----------| ----------------------|-------------------------|
  | RST/RESET |   RESET               |   4                     |  
  | DC        |   DC                  |   2                     |
  | CLK/CLOCK |   CLK                 |   18                    |
  | CS        |   CS                  |   15                    |
  | M0SI      |   MOSI                |   23                    |
  | MISO      |   MISO                |   19                    |
*/

#include <LCD_ESP.h>
#include "Button2.h"; 
#include "Rotary.h";
#include "SPI.h"
#include "TFT_eSPI.h"
#include <XPT2046_Touchscreen.h>

#include <SI470X.h>

#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 27   // T_CLK
#define XPT2046_CS 5    // T_CS


#define RESET_PIN 25       // On Arduino Atmega328 based board, this pin is labeled as A0 (14 means digital pin instead analog)
#define SDA_PIN   21       //  

// I2C bus pin on ESP32
#define ESP32_I2C_SDA 21
#define ESP32_I2C_SCL 22


#define MAX_DELAY_RDS 40  // 40ms - polling method
#define MAX_DELAY_STATUS 2000




#define BLACK           0x0000
#define BLUE            0x3EBF
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#define ENCODING_END 255




#define ROTARY_PIN1 13
#define ROTARY_PIN2 12
#define BUTTON_PIN  14

#define CLICKS_PER_STEP 4   // this number depends on your rotary encoder
#define MIN_POS         880
#define MAX_POS         1080
#define START_POS       937
#define INCREMENT       1   // this number is the counter increment on each step

char Salida[4];

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;
int volumenrecord=10;


TFT_eSPI tft = TFT_eSPI();

Rotary r;
Button2 b;    
int frecuencia;

long rds_elapsed = millis();
long status_elapsed = millis();
int desplaza=0;

char *programInfo;
char *stationName;

int volume=10;

SI470X rx;


String salida, salidaSN;

String laststationName;
String lastprogramInfo;


// Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
void printTouchToSerial(int touchX, int touchY, int touchZ) {
  Serial.print("X = ");
  Serial.print(touchX);
  Serial.print(" | Y = ");
  Serial.print(touchY);
  Serial.print(" | Pressure = ");
  Serial.print(touchZ);
  Serial.println();
}


void showHelp()
{
  
  Serial.println("Escribe S o s para buscar estaciones Arriba o Abajo.");
  Serial.println("Escribe + o - para Subir o Bajar el volúmen.");
  Serial.println("Escribe ? para ver esta ayuda.");
  Serial.println("==================================================");
  delay(200);
}

void setup() {
  
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  delay(50);
  digitalWrite(RESET_PIN, LOW);
  delay(50);
  digitalWrite(RESET_PIN, HIGH);
  delay(50);
  
  
  
 
  Serial.begin(115200);
    while (!Serial) ;
  delay(50);


   Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL);
    
    rx.setup(RESET_PIN, ESP32_I2C_SDA);

    rx.setVolume(10);

    delay(500);


    // Select a station with RDS service in your place
    showHelp();
    rx.setFrequency(9370); // It is the frequency you want to select in MHz multiplied by 100.

    // Enables SDR
    rx.setRds(true);
    rx.setRdsMode(0); 
    rx.setSeekThreshold(30); // Sets RSSI Seek Threshold (0 to 127)

    rx.setVolume(volume);

  b.begin(BUTTON_PIN);
  b.setTapHandler(click);
  b.setLongClickHandler(longClick);

  r.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP, MIN_POS, MAX_POS, START_POS, INCREMENT);
  r.setChangedHandler(rotate);
  r.setLeftRotationHandler(showDirection);
  r.setRightRotationHandler(showDirection);

 
  

  lcd.init();
  lcdIni();
  lcd.backlight();
  
  //lcd.setCursor(0,0);  
  //lcdPrint2(F("Frecuencia"));
  lcd.setCursor(12,0); 
  lcdPrint2(F("93.7"));
  lcd.setCursor(17,0); 
  lcdPrint2(F("Mhz"));

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 3: touchscreen.setRotation(3);
  touchscreen.setRotation(3);

  tft.begin(); 
  tft.setRotation(3);
  tft.setCursor(0, 0);

  tft.fillScreen(BLACK);
  TFTPintaFreq();
  TFTPintabotones();
  
}

/////////////////////////////////////////////////////////////////

// on change
void rotate(Rotary& r) {
  // Serial.println(r.getPosition());
   frecuencia=r.getPosition();
 
  rx.setFrequency(frecuencia*10);
  lcd.setCursor(0,0);  
  lcdPrint2(F("           ")); 
  lcd.setCursor(0,1);
  lcdPrint2(F("                    ")); 
  lcd.setCursor(0,2);
  lcdPrint2(F("                    ")); 
  lcd.setCursor(0,3);
  lcdPrint2(F("                    ")); 
  float a=(float)frecuencia/10;
  Pinta_Frecuencia();
  
  
}



void TFTPintaFreq()
{
  tft.fillRect(15,15, 285, 75,BLACK);
   

  
    //tft.fillRect(x0,y0,L, A,BLUE);
     tft.drawRoundRect(10, 10, 295, 85, 5, WHITE);


   tft.fillRect(20,20, 99, 30,BLACK);
   tft.setCursor(20,20);
   tft.setTextColor(WHITE);
   tft.setTextSize(3);
   tft.println(" 93.7");

   tft.setCursor(120,20);
   tft.println("Mhz");

   tft.setTextColor(BLUE); 
   tft.setTextSize(2);
 
   

   tft.setTextDatum(MC_DATUM);
   //tft.setFreeFont(MYFONT32); 


  



}

void TFTPintabotones(){


   //Botão Prev
   tft.fillCircle(80, 135, 30, BLUE);
   tft.fillCircle(80, 135, 25, WHITE);
 //tft.fillTriangle(x2, y2, x1, y1, x0, y0, BLACK);
   tft.fillTriangle(82, 144, 82, 119, 67, 129, BLACK);
   tft.fillTriangle(97, 144, 97, 119, 82, 129, BLACK);
 //tft.fillRect(x0,y0,L, A,BLUE); 
   tft.fillRect(62,124,5,20,BLACK);

      //Botão Next
   tft.fillCircle(240, 135, 30, BLUE);
   tft.fillCircle(240, 135, 25, WHITE);
 //tft.fillTriangle( x2, y2,  x1, y1,  x0, y0, BLACK);
   tft.fillTriangle(240, 144, 240, 119, 255, 129, BLACK);
   tft.fillTriangle(225, 144, 225, 119, 240, 129, BLACK);
   tft.fillRect(255,124,5,20,BLACK);

   //Barra de Volume
  tft.drawRoundRect(50, 180, 220, 20, 0, WHITE);
  tft.fillRect(51, 181, 218, 18, BLACK);
  
  //Botão Volume -
   tft.drawRoundRect( 20, 180, 20, 20, 0, WHITE);
   tft.setCursor(25,182);
   tft.setTextColor(WHITE);
   tft.setTextSize(2);
   tft.println("-");

   //Status da barra de volume
  

   PintaVolumen();
   

   //Botão Volume +
  
   tft.drawRoundRect(280, 180, 20, 20, 0, WHITE);
   tft.setCursor(285,182);
   tft.setTextColor(WHITE);
   tft.setTextSize(2);
   tft.println("+");
   
  //Barra de Sinal
    tft.setCursor(60,215);
   tft.setTextColor(WHITE);
   tft.setTextSize(2);
   tft.println("S");
 


  //Stereo
  //tft.fillRect(x0,y0,L, A,BLUE);
  tft.drawRect(130,  120, 60, 20, WHITE);
  tft.fillRect(132,  122, 24, 16, RED);
   tft.setCursor(134,122);
   tft.setTextColor(WHITE);
   tft.setTextSize(2);
   tft.println("ST");
 
  tft.drawRect(130,  120, 30, 20, WHITE);
  tft.setCursor(168,122);
   tft.setTextColor(WHITE);
   tft.setTextSize(2);
   tft.println("M");

  

}

void PintaVolumen() {

  int v = map(volume, 0, 15, 0, 218);
   tft.fillRect(51, 181, v, 18, RED);
   tft.fillRect(51+v, 181, 218-v, 18, BLACK);

  tft.drawRect(130, 150, 60, 20, WHITE); 
  if (volume==0)
  tft.fillRect(131, 151, 58, 18, RED);
  else
  tft.fillRect(131, 151, 58, 18, BLACK);
  


  tft.setCursor(136,152);
   tft.setTextColor(WHITE);
   tft.setTextSize(2);
   tft.println("Mute");



} 
  



void Pinta_Frecuencia(){
  
  rx.clearRdsBuffer();
  programInfo=" ";
  stationName=" ";
  float a=(float)frecuencia/10;
  String b = String(a,1);
  if (a<100) {b=" "+b;}

  lcd.setCursor(12,0);
  lcdPrint2(F("     ")); 
  lcd.setCursor(12,0);
  lcdPrint2(b);

   

   tft.fillRect(20,20, 99, 30,BLACK);
   tft.setCursor(20,20);
   tft.setTextColor(WHITE);
   tft.setTextSize(3);
   tft.println(b);
}

// on left or right rotation
void showDirection(Rotary& r) {
  int i;
  String direction=r.directionToString(r.getDirection());
  TFTBorrarFrecuencia();
  if (direction=="RIGHT"){
  for (i=25;i<31;i++){
  tft.drawCircle(240, 135, i, GREEN);
  }
   delay(100);
  for (i=25;i<31;i++){
  tft.drawCircle(240, 135, i, BLUE);
  }
  }
  else
  {
  for (i=25;i<31;i++){
  tft.drawCircle(80, 135, i, GREEN);
  }
  delay(100);
  for (i=25;i<31;i++){
  tft.drawCircle(80, 135, i, BLUE);
  }
  }

}

// single click
void click(Button2& btn) {
  BuscaArriba();
}

// long click
void longClick(Button2& btn) {
  BuscaArriba();

}

void TFTBorrarFrecuencia()
{
  tft.fillRect(15,15, 285, 75,BLACK);
   tft.setTextColor(WHITE);
   tft.setTextSize(3);
   tft.setCursor(120,20);
   tft.println("Mhz");
   tft.fillRect(185, 15, 110, 25,BLACK);

}

void BuscaArriba(){
lcd.setCursor(0,0);  
  lcdPrint2(F("             ")); 
  TFTBorrarFrecuencia();
  lastprogramInfo="";
  
  int i;
  for (i=25;i<31;i++){
  tft.drawCircle(240, 135, i, GREEN);
  }
  rx.seek(SI470X_SEEK_WRAP, SI470X_SEEK_UP);
  frecuencia=rx.getFrequency()/10;
  r.resetPosition(frecuencia);
  Pinta_Frecuencia();
  programInfo=" ";
  stationName=" ";
  
  delay(100);
  for (i=25;i<31;i++){
  tft.drawCircle(240, 135, i, BLUE);
  }
  

}

void BuscaAbajo(){
lcd.setCursor(0,0);  
  lcdPrint2(F("             ")); 
  TFTBorrarFrecuencia();
  int i;
  for (i=25;i<31;i++){
  tft.drawCircle(80, 135, i, GREEN);
  }
  lastprogramInfo="";
  
  rx.seek(SI470X_SEEK_WRAP, SI470X_SEEK_DOWN);
  frecuencia=rx.getFrequency()/10;
  r.resetPosition(frecuencia);
  Pinta_Frecuencia();
  programInfo=" ";
  stationName=" ";
  delay(100);
  for (i=25;i<31;i++){
  tft.drawCircle(80, 135, i, BLUE);
  }

}

/*********************************************************
   RDS
 *********************************************************/

char *rdsTime;

uint8_t tipo_programa;

void showRdsData() {
  
  String salida1;
  String salida2;
  char psalida1[21];
  char psalida2[21];
  int i;
  String programa;
  
  if (programInfo) {programa=Quitar_espacios(Adaptar_RDS(programInfo));}
  
  

  if ((programInfo) && ((String)programInfo!=lastprogramInfo)) {
  //Serial.println(programInfo);
  lcd.setCursor(0,2);  
  lcdPrint2(F("                    ")); 
  lcd.setCursor(0,3);  
  lcdPrint2(F("                    ")); 
  salida=programa;
  salida1=salida.substring(0,20);
  salida1.toCharArray(psalida1, 21);
  lcd.setCursor(0,2); 
  lcdPrint2(F(psalida1)); 
   
   tft.setTextColor(BLUE);
   tft.setTextSize(2);
   tft.fillRect(15,45, 285, 45,BLACK);
   tft.drawRoundRect(20, 45, 270, 45,2, BLUE);
   tft.setCursor(30,50);
   TFTEscribir(tft,salida1);
   
   
  salida2=salida.substring(20,40);
  salida2.toCharArray(psalida2, 21);
  lcd.setCursor(0,3); 
  lcdPrint2(F(psalida2)); 
  tft.setCursor(30,70);
   TFTEscribir(tft,salida2);
  lastprogramInfo=(String)programInfo;
  
  
  }
  else
  if ((programInfo) && (programa.length()>40)) {
  desplaza=desplaza+1;
  lcd.setCursor(0,2);  
  lcdPrint2(F("                    ")); 
  lcd.setCursor(0,3);  
  lcdPrint2(F("                    ")); 
  
  
  salida1=salida.substring(0+desplaza,21+desplaza);
  salida1.toCharArray(psalida1, 21);
  lcd.setCursor(0,2); 
  lcdPrint2(F(psalida1)); 
  tft.fillRect(21, 46,  268, 43,BLACK);
   tft.setTextColor(BLUE);
   tft.setTextSize(2);
   tft.setCursor(30,50);
    TFTEscribir(tft,salida1);
  salida2=salida.substring(20+desplaza,41+desplaza);
  salida2.toCharArray(psalida2, 21);
  //Serial.println(salida1);
  //Serial.println(salida2);
  lcd.setCursor(0,3); 
  lcdPrint2(F(psalida2)); 
  tft.setCursor(30,70);
  TFTEscribir(tft,salida2);
  if (lastprogramInfo.length()<=(desplaza+40)) {desplaza=-1;}
  
  }

  if ((stationName) && ((String)stationName!=laststationName)) {
  
  

  lcd.setCursor(0,1);  
  lcdPrint2(F("                    ")); 
  salidaSN=Adaptar_RDS(stationName);
  salida1=salidaSN.substring(0,21);
  salida1.toCharArray(psalida1, 21);
  lcd.setCursor(0,1); 
  lcdPrint2(F(psalida1)); 
  laststationName=(String)stationName;
  tft.fillRect(185, 15, 110, 25,BLACK);
  tft.setCursor(190,20);
   tft.setTextColor(MAGENTA); 
   tft.setTextSize(2);
   tft.drawRoundRect(185, 15, 110, 25, 2, MAGENTA);
   
    TFTEscribir(tft,stationName);


  }
  


  if (rdsTime) {
    //Serial.print("\nUTC / Time....: ");
    //Serial.println(rdsTime);
  }
 

}

void Pinta_RSSI()
{
  
 int sig=rx.getRssi();
 int i;

  if(sig<30){
  tft.fillRect(80, 220, 25, 10, BLACK); 
   tft.fillRect(110, 218, 25, 12, BLACK); 
   tft.fillRect(140, 216, 25, 14, BLACK); 
   tft.fillRect(170, 214, 25, 16, BLACK); 
   tft.fillRect(200, 212, 25, 18, BLACK);  
   tft.fillRect(230, 209, 25, 21, BLACK);  
  }
  if(sig>30 & sig<=37){
  tft.fillRect(80, 220, 25, 10, GREEN); 
   tft.fillRect(110, 218, 25, 12, BLACK); 
   tft.fillRect(140, 216, 25, 14, BLACK); 
   tft.fillRect(170, 214, 25, 16, BLACK); 
   tft.fillRect(200, 212, 25, 18, BLACK);  
   tft.fillRect(230, 209, 25, 21, BLACK);  
  }
  if(sig>37 & sig<=44){
   tft.fillRect(80, 220, 25, 10, GREEN); 
   tft.fillRect(110, 218, 25, 12, GREEN); 
   tft.fillRect(140, 216, 25, 14, BLACK); 
   tft.fillRect(170, 214, 25, 16, BLACK); 
   tft.fillRect(200, 212, 25, 18, BLACK);  
   tft.fillRect(230, 209, 25, 21, BLACK);  
  }
  if(sig>44 & sig<=51){
    tft.fillRect(80, 220, 25, 10, GREEN); 
   tft.fillRect(110, 218, 25, 12, GREEN); 
   tft.fillRect(140, 216, 25, 14, GREEN); 
   tft.fillRect(170, 214, 25, 16, BLACK); 
   tft.fillRect(200, 212, 25, 18, BLACK);  
   tft.fillRect(230, 209, 25, 21, BLACK);  
  }
  if(sig>51 & sig<=59){
  tft.fillRect(80, 220, 25, 10, GREEN); 
   tft.fillRect(110, 218, 25, 12, GREEN); 
   tft.fillRect(140, 216, 25, 14, GREEN); 
   tft.fillRect(170, 214, 25, 16, GREEN); 
   tft.fillRect(200, 212, 25, 18, BLACK);  
   tft.fillRect(230, 209, 25, 21, BLACK);  
  }
  if(sig>59 & sig<=67){
   tft.fillRect(80, 220, 25, 10, GREEN); 
   tft.fillRect(110, 218, 25, 12, GREEN); 
   tft.fillRect(140, 216, 25, 14, GREEN); 
   tft.fillRect(170, 214, 25, 16, GREEN); 
   tft.fillRect(200, 212, 25, 18, GREEN);  
   tft.fillRect(230, 209, 25, 21, BLACK); 
  }
  if(sig>67){
    
   tft.fillRect(80, 220, 25, 10, GREEN); 
   tft.fillRect(110, 218, 25, 12, GREEN); 
   tft.fillRect(140, 216, 25, 14, GREEN); 
   tft.fillRect(170, 214, 25, 16, GREEN); 
   tft.fillRect(200, 212, 25, 18, GREEN);  
   tft.fillRect(230, 209, 25, 21, GREEN);  
  }


 int valor=sig/5-5;
 
 char caracter=255;;
 String texto=String(caracter);

 if ((valor>0) && (valor<=10))
 {
 for (i=0;i<(valor+1);i=i+1){
 lcd.setCursor(i,0);
 lcdPrint2(texto); 
 }

 for (i=(valor+1);i<11;i=i+1){
 lcd.setCursor(i,0);
 lcdPrint2(" "); 
 }
 }
}


void checkRDS() {
  if (rx.getRdsReady()) {
    programInfo = rx.getRdsProgramInformation();
    stationName = rx.getRdsStationName();
    rdsTime = rx.getRdsTime();
    tipo_programa=rx.getRdsProgramType();
    showRdsData();
  }
}





void loop() {

   if (touchscreen.tirqTouched() && touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    printTouchToSerial(x, y, z);
    pulsaPantalla(x,y,z);

    delay(100);
  }



  r.loop();
  b.loop();

      if ((millis() - rds_elapsed) > MAX_DELAY_RDS) {
    Pinta_RSSI();    
    checkRDS();
    rds_elapsed = millis();

  }
 
 if (Serial.available() > 0)
  {
    char key = Serial.read();
    switch (key)
    {
    case '+':
      rx.setVolumeUp();
     
      volume=volume+1;
      if (volume>15) {volume=15;}
      PintaVolumen();
      break;
    case '-':
      rx.setVolumeDown();
   
      volume=volume-1;
      if (volume<0) {volume=0;}
      PintaVolumen();
      break;

    case 'S':
      BuscaArriba();
      break;
    case 's':
      BuscaAbajo();
      break;
    
    case '?':
      showHelp();
      break;
    default:
      break;
    } 
 
 } 
}


char convertRDS(char letra){
  
  uint8_t txt=(uint8_t)letra;
  switch (txt) {
    case 0x80:
      return 'á';  //á
    case 0x82:
      return 'é';  //é
    case 0x84:
      return 'í';  //í
    case 0x86:
      return 'ó';  //ó
    case 0x88:
      return 'ú';  //ú
    case 0x8A:
      return 'Ñ';  //Ñ >> ñ
    case 0x9A:
      return 'ñ';  //ñ
    case 0x8B:
      return 'C';  //Ç >> ç
    case 0x9B:
      return 'c';  //ç 
    case 0xD9:
      return 'U';  //Ü >> ü
    case 0x99:
      return 'u';  //ü
    case 0xC0:
      return 'Á';  //Á >> A
    case 0xC2:
      return 'É';  //É >> E
    case 0xC4:
      return 'Í';  //Í >> I
    case 0xC6:
      return 'Ó';  //Ó >> O
    case 0xC8:
      return 'Ú';  //Ú >> U
    default:
      if (txt>127) {return ' ';}
      else
      return letra;
  }
  }

String Adaptar_RDS (char* entrada){
  String TextoRDS="";
  TextoRDS=(String) entrada;
  String TextoRDS2="                                                                                                                        ";
  int i=0;
  int j=0;
  String Texto3="";
  //Serial.println(TextoRDS);
  while(((uint8_t)TextoRDS.charAt(i)!=13) && (i <TextoRDS.length()))
{
  TextoRDS2.setCharAt(i,convertRDS(TextoRDS.charAt(i)));
  //Serial.print((int)TextoRDS.charAt(i)); Serial.print(" ");
  i=i+1;
}
 //Serial.println(" ");Serial.println(" ");

j=i-1;
while (((int)TextoRDS.charAt(j)==32) && (j>2)){
j=j-1;
}

Texto3=TextoRDS2.substring(0,j+1);

return Texto3;
}


char convertTFT(char letra){
  int c=letra;
  
 
  
  switch (c) {

   
    case 129:
      return 'A';  //Á >> A
    case 137:
      return (char)(144);  //É  
    case 141:
      return 'I';  //Í >> I
    case 147:
      return 'O';  //Ó >> O
    case 154:
      return 'U';  //Ú >> U
    case 161:
      return (char)(160);   //á   
    case 169:
      return (char)(130);   //é
    case 173:
      return (char)(161);   //í
    case 179:
      return (char)(162);  //ó
    case 186:
      return (char)(163);  //ú
    case 177:
      return (char)(164);  //ñ
    case 145:
      return (char)(165);  //Ñ
    case 156:
      return (char)(154);  //Ü
    case 188:
      return (char)(129);  //ü  
    case 191:
      return (char)(168);  //ü  
    default:
        return letra;
  }
  }




void TFTEscribir (TFT_eSPI pantalla, String entrada){
  String Texto=(String)entrada;
    
  int i=0;
  char c;
  
    while( i <Texto.length())
{
  c=(char)(Texto.charAt(i));
  
  if (((int)c!=195) && ((int)c!=194)) {pantalla.print(convertTFT(c));}

  i=i+1;
}

}


String Quitar_espacios(String Tentrada){
String Tsalida=Tentrada;
int i=Tentrada.length();
while(Tentrada.charAt(i-1)==' '){
if (i>2)
  {
    Tsalida=Tentrada.substring(0,i-1);
    i=i-1;
  }
}

return Tsalida;
}

void pulsaPantalla (int x, int y, int z)
{
 
 int v;

if (z>450){

delay(100);

if ((x>=75) && (x<=135) && (y>=100) && (y<=160)) {
  BuscaAbajo();
}
if ((x>=240) && (x<=300) && (y>=100) && (y<=160)) {
  BuscaArriba();
}

if ((x>=40) && (x<=65) && (y>=175) && (y<=200)) {
      rx.setVolumeDown();
      volume=volume-1;
      if (volume<0) {volume=0; volumenrecord=0;}
      PintaVolumen();
}

if ((x>=300) && (x<=335) && (y>=175) && (y<=200)) {
      rx.setVolumeUp();
      volume=volume+1;
      if (volume>15) {volume=15;}
      PintaVolumen();
}
if ((x>=155) && (x<=215) && (y>=150) && (y<=170)) {
      if (volume==0) {
      rx.setVolume(volumenrecord);
      volume=volumenrecord;
      PintaVolumen();
      }
      else {
      rx.setVolume(0);
      volumenrecord=volume;
      volume=0;
      PintaVolumen();
      }
}

}
}
