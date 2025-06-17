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


// Alarm time
int alarmHour = 12;
int alarmMinute = 0;
bool isRinging = false;

// Serial max tokens
#define MAX_TOKENS 8
const char* HELP_TEXT = R"rawliteral(
Available commands:

set date YYYY MM DD HH MM SS
  - Set the current date.
  - Example: set date 2000 12 31 12 59 36

set alarm HH MM
  - Set alarm time (24-hour format).
  - Example: set alarm 13 30

get date
  - Show current date.

get alarm
  - Show current alarm time.

help
  - Show this help message.
)rawliteral";


unsigned long lastTouchMs = millis();

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
  serial();

  datetime = Rtc.GetDateTime();
  
  if (datetime.IsValid()) {
    isBatteryLow = false;
  } else {
    datetime = compiledDateTime + millis() / 1000;
    Rtc.SetDateTime(datetime);
    isBatteryLow = true;
  }
  
  displayClock(datetime);

  if (datetime.Hour() == alarmHour && datetime.Minute() == alarmMinute && datetime.Second() == 0) {
    isRinging = true;
  }
  
  if (isRinging) {
    const char alarmStr[17];
    sprintf(alarmStr, "     %02d:%02d      ", alarmHour, alarmMinute);
    lcdPrint("     Alarm      ", 0, 0);
    lcdPrint(alarmStr, 0, 1);
    
    unsigned long lastBuzzMs = millis();
    while (true) {
      if (digitalRead(TOUCH) == HIGH) {
        lastTouchMs = millis();
        isRinging = false;
        digitalWrite(BUZZER, LOW);
        lcd.display();
        break;
      }

      if (ticksDiff(millis(), lastBuzzMs) >= 1000) {
        if (digitalRead(BUZZER)== HIGH) {
          digitalWrite(BUZZER, LOW);
          lcd.noDisplay();
        } else {
          digitalWrite(BUZZER, HIGH);
          lcd.display();
        }
      }
    }
  } else {
    if (digitalRead(TOUCH) == HIGH && ticksDiff(millis(), lastTouchMs) >= 300) {
      lastTouchMs = millis();
      lcdPrint("     Alarm      ", 0, 0);
      const char alarmStr[17];
      sprintf(alarmStr, "     %02d:%02d      ", alarmHour, alarmMinute);
      lcdPrint(alarmStr, 0, 1);
      delay(3000);
    }
  }
}

int splitString(String input, char delimiter, String tokens[]) {
  int tokenIndex = 0;
  int start = 0;

  for (int i = 0; i <= input.length(); i++) {
    if (input.charAt(i) == delimiter || i == input.length()) {
      if (tokenIndex < MAX_TOKENS) {
        tokens[tokenIndex++] = input.substring(start, i);
        start = i + 1;
      } else {
        break;  // prevent overflow
      }
    }
  }
  return tokenIndex;  // number of tokens parsed
}

void serial() {
  if (Serial.available()) {
    boolean showHelp = false;
    String input = Serial.readStringUntil('\n');
    String args[MAX_TOKENS];
    splitString(input, ' ', args);
  
    if (args[0] == "set") {
      // set date
      if (args[1] == "date") {
        if (args[2] == "" || args[3] == "" || args[4] == "" || args[5] == "" || args[6] == "" || args[7] == "") {
          // missing date values
          Serial.println("Missing values.");
          showHelp = true;
        } else {
          // set date
          int YYYY = String(args[2]).toInt();
          int MM = String(args[3]).toInt();
          int DD = String(args[4]).toInt();
          int HH = String(args[5]).toInt();
          int mm = String(args[6]).toInt();
          int SS = String(args[7]).toInt();
          RtcDateTime newDt = RtcDateTime(YYYY, MM, DD, HH, mm, SS);
  
          if (newDt.IsValid()) {
            Rtc.SetDateTime(newDt);
            Serial.println("Datetime set.");
          } else {
            const char dtStr[20];
            sprintf(dtStr, "%04d %02d %02d %02d:%02d:%02d", YYYY, MM, DD, HH, mm, SS);
            Serial.print(dtStr);
            Serial.println(" : Invalid date values.");
          }
        }
      } else if (args[1] == "alarm") {
        if (args[2] == "" || args[3] == "") {
          Serial.println("Missing values.");
        } else {
          alarmHour = String(args[2]).toInt();
          alarmMinute = String(args[3]).toInt();
          Serial.println("Alarm set.");
        }
      } else {
        Serial.println("Missing options: date, alarm.");
      }
    } else if (args[0] == "get") {
      if (args[1] == "date") {
        RtcDateTime dt = Rtc.GetDateTime();
        const char dtStr[20];
        sprintf(
          dtStr, "%04d %02d %02d %02d:%02d:%02d", 
          dt.Year(), dt.Month(), dt.Day(), 
          dt.Hour(), dt.Minute(), dt.Second()
        );
        Serial.println(dtStr);
      } else if (args[1] == "alarm") {
        const char alarmStr[6];
        sprintf(alarmStr, "%02d:%02d", alarmHour, alarmMinute);
        Serial.println(alarmStr);
      } else {
        Serial.print(args[1]);
        Serial.println(" : unknown option.");
      }
    } else {  
      Serial.print(args[0]);
      Serial.println(": command not found.");
      showHelp = true;
    }
    
    if (showHelp) {
      Serial.print(HELP_TEXT);
    }
  }
}

void displayClock(RtcDateTime &dt) {
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

void lcdPrint(const char* str, int x, int y) {
  lcd.setCursor(x, y);
  lcd.print(str);
}

unsigned long ticksDiff(unsigned long m1, unsigned long m2) {
  return m1 - m2;
}
