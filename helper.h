
const char* DAYS[7] = {
  "Sun", "Mon", "Tue",
  "Wed", "Thu", "Fri", "Sat"
};

const char* MONTHS[13] = {
  "xxx", "Jan", "Feb", "Mar", "Apr",
  "May", "Jun", "Jul", "Aug",
  "Sep", "Oct", "Nov", "Dec"
};

// Serial max tokens
#define MAX_TOKENS 8
const char* HELP_TEXT = R"rawliteral(
Available commands:

set datetime YYYY MM DD HH MM SS
  - Set the current datetime.
  - Example: set datetime 2000 12 31 12 59 36

set alarm on|off
  - Enable/disable alarm.
  - Example: set alarm on

set alarm HH MM
  - Set alarm time (24-hour format).
  - Example: set alarm 13 30

get datetime
  - Show current datetime.

get alarm
  - Show current alarm time.

help
  - Show this help message.
)rawliteral";


byte ALIEN_1[8] = {B00100, B00010, B00111, B01101, B11111, B10111, B10100, B00011};
byte ALIEN_2[8] = {B00100, B01000, B11100, B10110, B11111, B11101, B00101, B11000};
byte ALARM_ICON[8] = {B00100, B01010, B01010, B01010, B01010, B11111, B00000, B00100};
byte BATTERY_ICON[8] = {B01110, B11011, B10001, B10001, B10001, B11101, B11111, B11111};
