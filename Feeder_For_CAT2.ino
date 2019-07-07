/*
данная версия прошита 21.06.2019 в контролллер


TODO 

+ не работает BlueTooth перепутал Rx и Tx :) при установке перемычек
- есть косяки и лишние места по плате тож надо переделать  (например DC/DC модуль кверху пузом)
- реализовать прошивку контроллера ч/з блютуз - ????? 
-- выправить руки при напайке SMD компонетов (опционально) 
- ввести при запуске вывод хелпа в СОМ порт ( а то сам долго искал как с контроллером работать) 
-- допилить логирование событий
- заменить НС-06 на ЕSР8266 для интеграции в "вумныЙ Дом"
--- проц не стартует без подключенных RTC 

угол подправлен на 110 с возвратом в 0
*/



#include <Servo.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <DS3231.h>   





uint8_t count_To_EAT = 0, noEAT = 0, firstStart = 1;
uint8_t hours = 0, mins = 0, secs = 0, months = 0, days = 0, day_of_wek = 0;
uint8_t l_hours = 0, l_mins = 0, l_secs = 0; // переменные для "локального" времени 
uint16_t years = 0, t1= 0, t2 = 0;
uint8_t count = 0;
uint16_t val = 0; 
uint8_t Angle = 110;
uint8_t DeviderToEat = 1; // значение кратному времени кормежки


Servo servo;

char b[9];              //буфер для получения данных из COM порта

void Flush()
{  
  uint8_t i = 0;
    while(i!=9)
    {
      b[i] = 0;
      i++;
    }
}



DS3231  rtc(SDA, SCL);                 // Инициализация DS3231// SDA - A4 SCL - A5


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
  Serial.println("Enter the date and day of week: (dDDMMYYYY)");
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
 
  Serial.println("Enter a Hour: ");
    hours = Time_Set();
    Serial.println("Enter a Min: ");
    mins = Time_Set();
    Serial.println("Enter a Sec: ");
    secs = Time_Set();
    Serial.print("Time is: ");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(mins);
    Serial.print(":");
    Serial.println(secs);
             
    rtc.setTime(hours, mins, secs);              //  Установить время 16:29:00 (формат 24 часа)
  
    DateSet();
    rtc.setDOW(day_of_wek);                  //  Установить день-недели
    rtc.setDate(days, months, years);            //  Установить дату 31 августа 2018 года
 

}

void TimePrint()
{
  Serial.print(rtc.getDOWStr());       // Отправляем день-неделя
  Serial.print(" ");
  
  Serial.print(rtc.getDateStr());      // Отправляем дату
  Serial.print(" -- ");

  Serial.println(rtc.getTimeStr());    // Отправляем время

}

void setup() 
{

 Serial.begin(9600); 
 rtc.begin(); 
 
 /*
 ACSR |= 1<<ACBG|1<<ACIS1|1<<ACIE;
 ADCSRB |= 1<<ACME|1<<ADTS0;
 ADCSRA  &~(1<<ADEN);
 ADMUX |=1<<MUX0;
*/ 

  rtc.setOutput(0);
  pinMode(2, INPUT);

  EICRA |= 1<<ISC01|1<<ISC00;
  EIMSK |= 1<<INT0;
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


void BoomBang()
{
  servo.attach(9);       //устанавливаем пин как вывод управления сервой
  digitalWrite(7, HIGH);
  servo.write(180);              // tell servo to go to position in variable 'pos' 
  delay(150);                       // waits 15ms for the servo to reach the position 
  servo.write(120);              // tell servo to go to position in variable 'pos' 
  delay(150);     
  servo.write(180);              // tell servo to go to position in variable 'pos' 
  delay(150);
  delay(150);                       // waits 15ms for the servo to reach the position 
  servo.write(120);              // tell servo to go to position in variable 'pos' 
  delay(150);     
  servo.write(180);              // tell servo to go to position in variable 'pos' 
  delay(150);
digitalWrite(7, LOW);  
 servo.detach();       //устанавливаем пин как вывод управления сервой
}


void loop() {



  if (Serial.available()) // проверяем, поступают ли какие-то команды
  {

    val = Serial.read(); // переменная val равна полученной команде

    if (val == '1') { TimePrint(); } 
    if (val == '2') { TimePrint(); TimeForEAT(); Serial.println("Time To EAT");} 
    if (val == '3') { DS3231Control(); }
//    if (val == '4') { LogRead();}
    if (val == '+') {Angle = Angle + 2;}
    if (val == '-') {Angle =- 2;}
    
  }
  if(firstStart) { TimePrint(); count_To_EAT++; TimeForEAT(); Serial.println("Time To EAT"); firstStart = 0; }
//  if((((l_hours+1)%DeviderToEat)==0)&&(l_mins==0)&&(l_secs==0)) 
  if((((l_mins+1)%DeviderToEat)==0)&&(l_secs==0)) 
  {
    TimePrint();
    if(noEAT==0) { TimeForEAT(); Serial.println("Time To EAT");}
    else {Serial.println("No EAT");}
    count_To_EAT++;
 //   if(count_To_EAT==2) {BoomBang();}
  //  if(count_To_EAT==3) {noEAT=1;}
   // if(count_To_EAT==4) {BoomBang();}
   // if(count_To_EAT==5) {noEAT=0; count_To_EAT = 0;}



  }




}


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

