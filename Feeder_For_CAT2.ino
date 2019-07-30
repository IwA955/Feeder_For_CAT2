/*
TODO 

+ не работает BlueTooth перепутал Rx и Tx :) при установке перемычек
- есть косяки и лишние места по плате тож надо переделать  (например DC/DC модуль кверху пузом)
- реализовать прошивку контроллера ч/з блютуз - ????? 
-- выправить руки при напайке SMD компонетов (опционально) 
+ ввести при запуске вывод хелпа в СОМ порт ( а то сам долго искал как с контроллером работать) 
-- допилить логирование событий
- заменить НС-06 на ЕSР8266 для интеграции в "вумныЙ Дом"
--- проц не стартует без подключенных RTC 
*/



#include <Servo.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <DS3231.h>   

#include <EEPROM.h>


#define DEBUG


uint8_t count_To_EAT = 0, noEAT = 0, firstStart = 1;
uint8_t hours = 0, mins = 0, secs = 0, months = 0, days = 0, day_of_wek = 0;
uint8_t l_hours = 0, l_mins = 0, l_secs = 0; // переменные для "локального" времени 
uint16_t years = 0, t1= 0, t2 = 0;
uint8_t count = 0, FstTime = 06, ScdTime = 0x09, ThdTime = 11;
uint16_t val = 0; 
uint16_t deb_val = 0; 

// настройки 
volatile uint8_t Angle = 150; // угол
//volatile uint8_t EatSkipNumber = 4; // номер кормления для пропуска
//volatile uint8_t DeviderToEat = 1; // значение кратному времени кормежки
volatile uint16_t BOUDRATE = 9600;

volatile uint8_t FstTimeH = 07, FstTimeM = 30,  ScdTimeH = 12, ScdTimeM = 0, ThdTimeH = 19, ThdTimeM = 0;


int address = 1;
byte value = 0;



Servo servo;

char b[9];              //буфер для получения данных из COM порта


void Start0() // программисты считают с 0 и это самый-самый первый старт!
{
  // записываем 0 в первые 12 ячейки памяти EEPROM
  for (int i = 0; i < 12; i++) {EEPROM.write(i, 0);} // трем предыдущие значения
  EEPROM.write(0, 254); // это типо флаг того что нулевой старт прошел и нужные настройки записаны в ЕЕПРОМ
  EEPROM.write(1, Angle);//Angle = 110
  EEPROM.write(2, FstTimeH);
  EEPROM.write(3, FstTimeM); 
  EEPROM.write(4, ScdTimeH); 
  EEPROM.write(5, ScdTimeM); 
  EEPROM.write(6, ThdTimeH); 
  EEPROM.write(7, ThdTimeM); 
}

void StoreSettings()
{
  // записываем 0 в первые 4 ячейки памяти EEPROM
  for (int i = 1; i < 12; i++) {EEPROM.write(i, 0);} // трем предыдущие значения
  // и пишем новые
  EEPROM.write(1, Angle);
  EEPROM.write(2, FstTimeH);
  EEPROM.write(3, FstTimeM); 
  EEPROM.write(4, ScdTimeH); 
  EEPROM.write(5, ScdTimeM); 
  EEPROM.write(6, ThdTimeH); 
  EEPROM.write(7, ThdTimeM); 
  
  
  Serial.println(F("Settings stored in EEPROM..."));
}

void RestoreSettings()
{
  byte b1[7];
/*  
  Serial.println("Current setting is: ");
  Serial.print("Angle of turn: ");
  Serial.println(Angle);
  Serial.print("EatSkipNumber: ");
  Serial.println(EatSkipNumber);
  Serial.print("DeviderToEat: ");
  Serial.println(DeviderToEat);
  */
  while(address!=7)
  {
    // считываем значение по текущему адресу EEPROM
    value = EEPROM.read(address);
    b1[address] = value;
 /*
    Serial.print(address);
    Serial.print("\t");
    Serial.print(value, DEC);
    Serial.println();
   */ 
    // устанавливаем следующую ячейку памяти
    address = address + 1;
    
    
  //  delay(500);
  }
  address = 1;
  Angle = b1[1];
  FstTimeH = b1[2];
  FstTimeM = b1[3]; 
  ScdTimeH = b1[4]; 
  ScdTimeM = b1[5]; 
  ThdTimeH = b1[6]; 
  ThdTimeM = b1[7]; 
  
  
 /*
  Serial.println("New setting is: ");
  Serial.print("Angle of turn: ");
  Serial.println(Angle);
  Serial.print("EatSkipNumber: ");
  Serial.println(EatSkipNumber);
  Serial.print("DeviderToEat: ");
  Serial.println(DeviderToEat);
  */
  Serial.println(F("Settings restored..."));
  
}

void PrintAllCurrentSettings()
{
    Serial.println(F("All current setting is:  "));
    Serial.print(F("Angle of turn: "));
    Serial.println(Angle);
    
    Serial.print(F("First Time for EAT: "));
    Serial.print(FstTimeH);
    Serial.print(":");
    Serial.println(FstTimeM);
    
    Serial.print(F("Second Time for EAT: "));
    Serial.print(ScdTimeH);
    Serial.print(":");
    Serial.println(ScdTimeM);
    
    Serial.print(F("Third Time for EAT: "));    
    Serial.print(ThdTimeH);
    Serial.print(":");
    Serial.println(ThdTimeM);
}

void PrintHELP()
{
    //char PROGMEM  help[] = {"Privet!!! Dobro pozalovat' !! I'am Cat Feeder v 2.0 \n For control me send next: \n 1 - HELP\n  2 - Time Print \n  3 - Kormit' kota (prinuditel'no)\n 4 - SetUP for RTC\n + or - for correct the angle of turn\n Have a nice Day...\n "};
    
    //{" 1 - HELP\n  2 - Time Print \n  3 - Kormit' kota (prinuditel'no)\n 4 - SetUP for RTC \n"}; 
    Serial.println(F("1 - HELP"));
    Serial.println(F("2 - Time Print"));
    Serial.println(F("3 - Kormit' kota"));
    Serial.println(F("4 - Setup for RTC"));
    Serial.println(F("5 - Setup for Time for Eat"));
    Serial.println(F("6 - Print All Current settings"));
    Serial.println(F("!!! DEBUG MODE !!!"));
    Serial.println(F("7 - AngleCorrect\n 8 - freeRam\n 9 - EEPROM clear"));
  
  
    delay(5000);
}

void Flush()
{  
  uint8_t i = 0;
    while(i!=9)
    {
      b[i] = 0;
      i++;
    }
}

void AngleCorrect()
{
  Flush();// чистим массив
  count = 0;

  Serial.println(F("Current setting is:  "));
  Serial.print(F("Angle of turn: "));
  Serial.println(Angle);
 /*
  Serial.print(F("EatSkipNumber: "));
  Serial.println(EatSkipNumber);
  Serial.print(F("DeviderToEat: "));
  Serial.println(DeviderToEat);
  */
  Serial.println(F("Enter a new settings: (example: 110 or 030)"));
  
  
  while(count!=3)
  {
    if (Serial.available()) // проверяем, поступают ли какие-то команды
    {
   
        b[count] = Serial.read();
        //костыль!
        if(b[count]>48) {b[count] = b[count]-48;}
        if(b[count]==48) {b[count] = 0;}

        count++;
    }
  }
  count = 0;
  
  
  t1 = ((b[0]*100) + (b[1]*10) + b[2]);
  b[0] = t1;
/*
  count = 3;
  while(count!=7)
  {
    
    t1 = uint8_t(b[count]*10);
    t1 = (t1<<4);
    t2 = uint8_t(b[count+1]);
    t2 = (t2<<4);
    t1 = (t1+t2)>>4;
    b[count]= t1;
  //  Serial.println(t1);
    count =count +2;
//    delay(1700);
  } */
  count = 0;
  Angle = b[0];
  
 /* EatSkipNumber = b[3];
  DeviderToEat = b[5];*/
  Serial.println(F("New setting is: "));
  Serial.print(F("Angle of turn: "));
  Serial.println(Angle);
  
  /*
  Serial.print(F("EatSkipNumber: "));
  Serial.println(EatSkipNumber);
  Serial.print(F("DeviderToEat: "));
  Serial.println(DeviderToEat);
  */
  Serial.println(F(" Have a nice Day..."));
  StoreSettings();
}



DS3231  rtc(SDA, SCL);                 // Инициализация DS3231// SDA - A4 SCL - A5

Time t;
/*********************************************************************************/

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
/*********************************************************************************/



uint16_t Time_Set()
{

  Flush();
  t1 = 0;
  t2 = 0;
  count = 0;
  while(count!=2)
  {
    if (Serial.available()) // проверяем, поступают ли какие-то команды
    {
   
        b[count] = Serial.read();
        //костыль!
        if(b[count]>48) {b[count] = b[count]-48;}
        if(b[count]==48) {b[count] = 0;}
//        Serial.readBytes(b,2);
//        Serial.println("value is entered... ");
        count++;
    }
  }

  t1 = uint8_t(b[0]*10);
  t1 = (t1<<4);
  t2 = uint8_t(b[1]);
  t2 = (t2<<4);
  t2 = (t1+t2)>>4;
  Serial.println(t2);

  return t2;
}

void DateSet()
{

  Flush();
  t1 = 0;
  t2 = 0;
  count = 0;
  Serial.println(F("Enter the date and day of week: (dDDMMYYYY)"));
  while(count!=9)
  {
    if (Serial.available()) // проверяем, поступают ли какие-то команды
    {
   
        b[count] = Serial.read();
        //костыль!
        if(b[count]>48) {b[count] = b[count]-48;}
        if(b[count]==48) {b[count] = 0;}

        count++;
    }
  }

  day_of_wek = b[0];
  
  t1 = uint8_t(b[1]*10);
  t1 = (t1<<4);
  t2 = uint8_t(b[2]);
  t2 = (t2<<4);
  days = (t1+t2)>>4;
  
//  Serial.println(t1);
  
  t1 = uint8_t(b[3]*10);
  t1 = (t1<<4);
  t2 = uint8_t(b[4]);
  t2 = (t2<<4);
  months = (t1+t2)>>4;
  
  //years = ((b[5]*1000)+(b[6]*100)+(b[7]*10)+b[8]);
  years = ((b[5]*1000)+(0)+(b[7]*10)+b[8]);
 // Serial.println(b[6]);
 // Serial.println(years);
/*  
  Serial.print("Date is: ");
  
  Serial.print(day_of_wek);
  Serial.print(" ");
  Serial.print(days);
  Serial.print(".");
  Serial.print(months);
  Serial.print(".");
  Serial.println(years);
*/
  
}

void DS3231Control()
{
 
  Serial.println(F("Enter a Hour: "));
    hours = Time_Set();
    Serial.println(F("Enter a Min: "));
    mins = Time_Set();
    Serial.println(F("Enter a Sec: "));
    secs = Time_Set();
    Serial.print(F("Time is: "));
    Serial.print(hours);
    Serial.print(F(":"));
    Serial.print(mins);
    Serial.print(F(":"));
    Serial.println(secs);
             
    rtc.setTime(hours, mins, secs);             
  
    DateSet();
    rtc.setDOW(day_of_wek);                 
    rtc.setDate(days, months, years);            
 

}


void EATControl()
{
 
  Serial.println(F("Enter a First Hour: "));
    FstTimeH = Time_Set();
    Serial.println(F("Enter a First Min: "));
    FstTimeM = Time_Set();
  
    Serial.println(F("Enter a Second Hour: "));
    ScdTimeH = Time_Set();
    Serial.println(F("Enter a Second Min: "));
    ScdTimeM = Time_Set();

    Serial.println(F("Enter a Third Hour: "));
    ThdTimeH = Time_Set();
    Serial.println(F("Enter a Third Min: "));
    ThdTimeM = Time_Set();             
 
   
    Serial.println(F("New Settings is:"));
    Serial.print(FstTimeH);
    Serial.print(":");
    Serial.println(FstTimeM);
    
    Serial.print(ScdTimeH);
    Serial.print(":");
    Serial.println(ScdTimeM);
    
    Serial.print(ThdTimeH);
    Serial.print(":");
    Serial.println(ThdTimeM);
    StoreSettings();
}


void TimePrint()
{
  Serial.print(rtc.getDOWStr());       // Отправляем день-неделя
  Serial.print(F(" "));
  
  Serial.print(rtc.getDateStr());      // Отправляем дату
  Serial.print(F(" -- "));

  Serial.println(rtc.getTimeStr());    // Отправляем время

}

void BattCheck(void)
{
  if(ACSR&(1<<ACO))
  {
    PORTD |= 1<<5;
  }
  else
  {
    PORTD &= ~(1<<5);
  }
}

void setup() 
{

 Serial.begin(BOUDRATE); 
 rtc.begin(); 
 
 delay(8000); 
 if((EEPROM.read(0))==254) {RestoreSettings();}
 else{ Serial.println(F("Start 0...")); Start0();}
 
 DDRD |=1<<5 ;

 ACSR |= (1<<ACBG)|(1<<ACIE)|(1<<ACIS1)|(1<<ACIS0);
 
 /*
 ACSR |= 1<<ACBG|1<<ACIS1|1<<ACIE;
 ADCSRB |= 1<<ACME|1<<ADTS0;
 ADCSRA  &~(1<<ADEN);
 ADMUX |=1<<MUX0;
*/ 

 // rtc.setOutput(0);
 // pinMode(2, INPUT);

//  EICRA |= 1<<ISC01|1<<ISC00;
//  EIMSK |= 1<<INT0;
//  sei();
  
 // delay(5000); // пауза для зарядки конденсатора по питанию
//  ADC_Init();
  pinMode(7, OUTPUT);    //питание сервопривода
  pinMode(6, OUTPUT);    //управление сервиком
  pinMode(5, OUTPUT);    //светодиод на уровень АКБ
  digitalWrite(5, HIGH); // высокий пока норм заряд АКБ
  
  servo.attach(9);       //устанавливаем пин как вывод управления сервой


  // ставим серву в 0
  digitalWrite(7, HIGH);
 
//  delay(150);//время на открытие транзистора
  servo.write(0);    //поворачиваем сервопривод чтобы кормушка закрылась   
  delay(1500);          //задержка чтобы сервопривод успел повернуться на нужный угол
  digitalWrite(7, LOW);
  servo.detach();       
 
}


void TimeForEAT()
{     
  
   servo.attach(9);       //устанавливаем пин как вывод управления сервой
    digitalWrite(7, HIGH);
    
    delay(150); //время на открытие транзистора
   
    servo.write(Angle); //открываем на угол заданный потенциометром
    delay(570);               // ждем чтобы кормушка успела открыться
    servo.write(0);         // закрывам кормушку   
    delay(750);               // ждем чтобы кормушка успела открыться
    digitalWrite(7, LOW);
   servo.detach();       //устанавливаем пин как вывод управления сервой
}


void EEPROM_Clear()
{
 
  for (uint8_t i = 0; i< 12; i++)  {Serial.println(EEPROM.read(i)); delay(500);}
  for (uint8_t i = 0; i < 12; i++) {EEPROM.write(i, 0);} // трем предыдущие значения
  for (uint8_t i = 0; i< 12; i++)  {Serial.println(EEPROM.read(i)); delay(500);}
  
}



void loop() {


  BattCheck();
  _delay_ms(50);
  
  if (Serial.available()) // проверяем, поступают ли какие-то команды
  {

    val = Serial.read(); // переменная val равна полученной команде

    if (val == '1') {PrintHELP();} 
    if (val == '2') { TimePrint(); } 
    if (val == '3') { TimePrint(); TimeForEAT(); Serial.println(F("Time To EAT"));} 
    if (val == '4') { DS3231Control(); }
    if (val == '5') { EATControl();}
    if (val == '6') {PrintAllCurrentSettings();}
    // режим отладки реализован следующим:
    if (val == '7') {AngleCorrect();} 
    if (val == '8') { Serial.println(freeRam()); } 
    if (val == '9') {EEPROM_Clear();} 
    
    
  }
  if(firstStart) 
  {
      Serial.println(F("Privet! \n Dovai kormit' kota? \n For HELP send me 1...\n")); 
     // Serial.println(freeRam());  
      TimePrint(); 
      count_To_EAT++; 
      TimeForEAT(); 
      Serial.println(F("Time To EAT")); 
      firstStart = 0; 
      noEAT = 1;
  }
 
  
  t = rtc.getTime();
  if((t.hour==FstTimeH)&&(t.min==FstTimeM)&&(t.sec==0)) {noEAT=0;}
  else 
  {
     if((t.hour==ScdTimeH)&&(t.min==ScdTimeM)&&(t.sec==0)) {noEAT=0;}
     else
     {
         if((t.hour==ThdTimeH)&&(t.min==ThdTimeM)&&(t.sec==0)) {noEAT=0;}
     }  
  }
  

  
   if(noEAT==0) { TimePrint(); TimeForEAT(); Serial.println(F("Time To EAT")); noEAT = 1; }
   


}

/*
ISR (INT0_vect)
{
    l_secs++;
    if(l_secs==59) 
    {
        l_secs = 0; 
        l_mins++;
        if(l_mins==59)
        {
          l_mins =0;
          l_hours ++;
          if(l_hours==23) {l_hours=0;}
        }
    }  
}
*/

