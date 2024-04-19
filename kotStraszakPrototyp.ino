
// CONNECTIONS:
// DS1302 CLK/SCLK --> 5
// DS1302 DAT/IO --> 4
// DS1302 RST/CE --> 2
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND
//
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//A0,A1,A2;   IO, SCLK, CE, 5V   %zegar
//8,9;                           %servo 
//11;                            %IR reciver (output, GND, VCC)
//6;                             %czujnik temp.

//2;                             %button uzbrajanie/rozbrajanie
//3;                             %button2 pomiar 
//10;                            %buzzer
 
//13;           %dioda od sprawdzenia flagi
//12;           %dioda od flagi
//7;            %dioda od otrzymania sygnału
//4;            %dioda od przekroczenia ilosci alarmow
//arduino nano na A4- SDA, A5- SCL
//5; musi być wyzwolenie//teraz daje diode

/////na 8; daje diode od uzbrojenia

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#include <Wire.h> 
#include <Servo.h> 
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <IRremote.h>

#define DHTPIN 6
#define DHTTYPE    DHT11

DHT_Unified dht(DHTPIN, DHTTYPE);

byte cat[8] = {
  0b11011,
  0b11011,
  0b11111,
  0b10101,
  0b11111,
  0b01110,
  0b11110,
  0b11111
};

byte tail[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b00010,
  0b00100,
  0b01000,
  0b00101,
  0b00011
};

byte stopien[8] = {
  0b00111,
  0b00101,
  0b00111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

byte klucz[8] = {
  0b00100,
  0b00110,
  0b00100,
  0b00110,
  0b00100,
  0b01110,
  0b01110,
  0b00000
};

byte kropla[8] = {
  0b00100,
  0b00100,
  0b01110,
  0b01110,
  0b10111,
  0b10111,
  0b01110,
  0b00000
};


int RECV_PIN = 11;
long unsigned int sygnal;
IRrecv irrecv(RECV_PIN);
IRsend sender;

uint32_t data = 0xFF609F;                             ///////////////sygnal  ///co to
  uint8_t len = 32; 
decode_results results;

LiquidCrystal_I2C lcd(0x3F, 16, 2);//ty masz inny adres

ThreeWire myWire(A0, A1, A2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire); 

const int buzzer = 10; //5000 fajny efekt
const int ledPin7 =  7; 
const int ledPin12 =  12;      
const int ledPin13 =  13;
const int ledPin4 =  4;

const int diodaNadajnik =  5;

const int buttonPin = 2;   
const int buttonPin2 = 3;
int buttonState = 0;
int buttonState2 = 0;
         
bool flaga=true;
bool flaga2=false;

int licznik=0;
int steps=0;

Servo myservo1;
Servo myservo2;

unsigned long aktualnyCzas = 0;
unsigned long zapamietanyCzasLED1 = 0;
unsigned long zapamietanyCzasReciver = 0;


void setup() {

                                                  
  Serial.begin(9600);
  Serial.println("TEST");
   
 //myservo1.attach(8); 
 
 myservo2.attach(9);


  dht.begin();
  
   Rtc.Begin();
   getDateTime(Rtc.GetDateTime());    
   RtcDateTime czas = RtcDateTime(__DATE__, __TIME__);                          //ustawienie bieżącej daty i godziny przy kompilacji 
   Rtc.SetDateTime(czas);
   getDateTime(Rtc.GetDateTime());
   
  irrecv.enableIRIn(); // Start the receiver
 
 lcd.begin();
 lcd.noBacklight();
 lcd.createChar(0, cat);
 lcd.createChar(1, tail);
 lcd.createChar(2, stopien);
 lcd.createChar(3, klucz);
 lcd.createChar(4, kropla);

  Serial.print("Data systemowa: ");
  Serial.println(__DATE__);
  Serial.print("godzina systemowa: ");
  Serial.println(__TIME__);

pinMode(5, OUTPUT);//diodaIR

  
  pinMode(ledPin4, OUTPUT); //zabezpieczenie przed przeciążeniowe
  pinMode(ledPin7, OUTPUT); // nadanie sygnału 
  pinMode(ledPin12, OUTPUT);// flaga
  pinMode(ledPin13, OUTPUT);// sprawdzenie flagi
pinMode(8,OUTPUT); // uzbrojenie
   
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  
attachInterrupt(digitalPinToInterrupt(2), onStep, FALLING);
//attachInterrupt(digitalPinToInterrupt(3), onStep2, FALLING);
flaga=1;
}

void loop() { //############################################################################################################################
  
//Serial.println(flaga);
aktualnyCzas = millis();
   if(flaga==1){digitalWrite(ledPin12, HIGH);}
  else{digitalWrite(ledPin12, LOW);}
   

if (irrecv.decode(&results)) {            
   checkSignal();
  delay(100);
} else{digitalWrite(ledPin7, LOW);}

  

   


getDateTime(Rtc.GetDateTime());

//buttonState = digitalRead(buttonPin);
//if(buttonState == LOW){ odczytajDane();}
//Serial.println(steps);
 if(steps==2){odczytajDane();}

 if(licznik>3){  //ZABEZPIECZENIE PRZED ZBLOKOWANIEM ALARMU
 digitalWrite(ledPin4,HIGH);
 throwError();
 }  

                      
 
 
//Serial.println(licznik);
  
  
}///########################################################################################################################################################
#define countof(a)  (sizeof(a) / sizeof(a[0]))

                                                  void getDateTime(const RtcDateTime& dt)    {

char datestring[20];
int sekundy=dt.Second();

byte second = dt.Second ();
byte minute = dt.Minute();
byte hour  = dt.Hour();
//byte day 
//byte month
//byte year
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(), 
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
  //  Serial.println(datestring);
 //delay(250);


 if(hour==23 && minute==59 && second== 59){ licznik=0; }  //%%zerowanie licznika zdarzeń po pełnej dobie
 //Serial.println(datestring[18]);
  if(datestring[18]=='5' && flaga2==0 || datestring[18]=='0' && flaga2==0){ nadajSygnal(); flaga2=1;}
   if(datestring[18]=='8'||datestring[18]=='3'){ 
    flaga2=0; 
    
    digitalWrite(ledPin13, HIGH);
    delay(1000);
    digitalWrite(ledPin13, LOW);
    if(flaga==0){zdarzenie();}    }   //tutaj dodaj że kiedy sprawdzasz  
}

                                                  void zdarzenie(){

      lcd.clear();
       lcd.backlight();
      lcd.print("Brak sygnalu");
      delay(3000);                                              
      
Serial.println("Nie otrzymano pakietu danych na czas-ALARM");
for(int i=0;i<2;i++){
//tone(10,5000);
delay(100);
noTone(10);
delay(100);
}

licznik++;
delay(1000);
lcd.clear();
       lcd.noBacklight();
} 

                                                 void odczytajDane(){
   lcd.clear();
  lcd.backlight();
  lcd.print("Zarejestrowano");
    lcd.write(1);
     lcd.write(0);
     
  lcd.setCursor(0,1);
  lcd.print(" zdarzen: ");
 
  lcd.print(licznik);
    lcd.print(" ");
  
  //delay(1000);

  
Serial.println(steps);
 
 do{Serial.println(steps);}
 while(steps==2); 
 lcd.clear();
  checkPogoda();
 
 
 }

                                                void throwError(){
  Serial.println("przeciazenie systemu");
   digitalWrite(13, HIGH);
   lcd.clear();
   lcd.backlight();

   while(1==1){
  lcd.print("Przekroczono");
  lcd.setCursor(0,1);
  lcd.print(" ilosc zdarzen");

       delay(3000);
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("Nacisnij reset");
   lcd.setCursor(0,1);
  lcd.print("przycisk NANO");
  delay(3000);
  lcd.clear();
   }
   
   
}
                                              void nadajSygnal(){
  flaga=0;
  //sender.sendNEC(data, len);      //////////////////sygnal
  digitalWrite(5,HIGH);
  delay(250 );
digitalWrite(5,LOW);
}


                                              void checkSignal(){

   Serial.println(results.value, HEX);
    sygnal=results.value;
    irrecv.resume(); // Receive the next value
  if(sygnal==16753245){ 
        flaga=1; 
        digitalWrite(ledPin7,HIGH);
        //delay(100);
        //digitalWrite(ledPin7,LOW);
        
        }
  else{digitalWrite(ledPin7,LOW);}
  } 


                                            void checkPogoda(){
  lcd.clear();
lcd.setCursor(0,0);
  
sensors_event_t event;
 dht.temperature().getEvent(&event);
 if (isnan(event.temperature)) {
  lcd.setCursor(0,0);
   lcd.print(F("Awaria czujnika"));
   lcd.setCursor(1,1);
   lcd.print(F("temperatury!"));
  delay(5000);
lcd.clear();


  }
  else {
 
     lcd.write(3);
     lcd.print(F("  "));
  
   lcd.print(event.temperature);
    lcd.write(2);
    lcd.print(F("C"));
//delay(250);


  }

  lcd.setCursor(0,1);
  
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
      lcd.print(F(""));
  

  }

  else {
 
    lcd.write(4);
     lcd.print(F("  "));
    lcd.print(event.relative_humidity);
    lcd.print(F("%"));
   //delay(250);
  }



 do{Serial.println(steps);}
 while(steps==3); 
 
lcd.clear();
  lcd.noBacklight();
 // steps=1;
    
    
    
}

void onStep(){


  if(steps>3){steps=1;}
static unsigned long lastTime;
  unsigned long timeNow = millis();
  if (timeNow - lastTime < 200)
    return;

  steps++;
  lastTime = timeNow;
  
}

// void onStep2(){


//   if(steps>3){steps=1;}
// static unsigned long lastTime;
//   unsigned long timeNow = millis();
//   if (timeNow - lastTime < 200)
//     return;

//   steps++;
//   lastTime = timeNow;
  
// }


/*do zrobienia:

1.sprawdzić czemu dioda od uzbrojenia nie świeci
2.sprawdzić czemu dioda od odebrania sygnału nie świeci
3.kupić obiornik IR
4.kupić diode IR/pilota 
5.dociąć włącznik w obwód
6.dodać kod od uzbrojenia

*/