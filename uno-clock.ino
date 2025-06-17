#include <LiquidCrystal_I2C.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

const char* DAYS[7] = {
  "Sun", "Mon", "Tue",
  "Wed", "Thu", "Fri", "Sat"
};

const char* MONTHS[13] = {
  "xxx", "Jan", "Feb", "Mar", "Apr",
  "May", "Jun", "Jul", "Aug",
  "Sep", "Oct", "Nov", "Dec"
};

// connect to SCL and SDA
LiquidCrystal_I2C lcd(0x27, 16, 2);


// UI views
#define CLOCK_VIEW 1 
#define ALARM_VIEW 2
#define MENU_VIEW 3
int currentView = CLOCK_VIEW;

ThreeWire myWire(8, 9, 10); // IO/DAT, CLK, CE/RST
RtcDS1302<ThreeWire> Rtc(myWire);
RtcDateTime datetime;
RtcDateTime compiledDateTime;
boolean isBatteryLow = false;
const int compileTimeOffset = 8;
char mDate[17];
char mTime[9];

const int TOUCH = 12;
const int BUZZER = 13;

bool isRinging = false;
unsigned long alarmTime = 0; // alarm in seconds, should be only within 24 hours


void setup() {
  Serial.begin(9600);
  
  lcd.init();
  lcd.backlight();

  Rtc.Begin();
  compiledDateTime = RtcDateTime(__DATE__, __TIME__) + compileTimeOffset;

  // if RTC module is low/missing battery
  // or undetected
  if (!Rtc.IsDateTimeValid()) {
    Rtc.SetDateTime(compiledDateTime);
    isBatteryLow = true;
  }

  // Disable RTC write-protection
  if (Rtc.GetIsWriteProtected()) {
    Rtc.SetIsWriteProtected(false);
  }

  // Run RTC
  if (!Rtc.GetIsRunning()) {
    Rtc.SetIsRunning(true);
  }

  // Update RTC if time if not set
  if (Rtc.GetDateTime() <= compiledDateTime) {
    Rtc.SetDateTime(compiledDateTime);
  } else {
    Serial.println("RTC time not set.");
  }

  pinMode(BUZZER, OUTPUT);
  pinMode(TOUCH, INPUT);
}

void loop() {
  datetime = Rtc.GetDateTime();
  
  if (datetime.IsValid()) {
    isBatteryLow = false;
  } else {
    datetime = compiledDateTime + millis() / 1000;
    Rtc.SetDateTime(datetime);
    isBatteryLow = true;
  }


  switch (currentView) {
    
    case CLOCK_VIEW:
      Serial.println("CLOCK");
      unsigned long touchStart = millis();
      while (digitalRead(TOUCH) == HIGH) {
        currentView = ALARM_VIEW;
        if (ticksDiff(millis(), touchStart) >= 1000) {
          currentView = MENU_VIEW;
        }
      }
      clockDisplayHandler(datetime);
      break;
      
    case ALARM_VIEW:
      Serial.println("ALARM");
      break;

    case MENU_VIEW:
      Serial.println("MENU");
      break;

    default:
      Serial.println("DEFAULT");
      break;
  }
}

void clockDisplayHandler(RtcDateTime &dt) {
  snprintf_P(
    mDate, 17, PSTR("%s, %02u %s %04u"),
    DAYS[dt.DayOfWeek()], dt.Day(), MONTHS[dt.Month()], dt.Year()
  );
  snprintf_P(
    mTime, 9, PSTR("%02u:%02u:%02u"),
    dt.Hour(), dt.Minute(), dt.Second()
  );

  lcdPrint(mDate, 0, 0);
  lcdPrint(mTime, 4, 1);
}

void lcdPrint(char str[], int x, int y) {
  lcd.setCursor(x, y);
  lcd.print(str);
}

unsigned long ticksDiff(unsigned long m1, unsigned long m2) {
  return m1 - m2;
}
