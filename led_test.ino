#include <FastLED.h>
#include <BluetoothSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define LED_COUNT 83
#define PIN 2
#define BRIGHT 50

BluetoothSerial ESP_BT; 
QueueHandle_t bluetoothQueue = xQueueCreate(10, sizeof(String));  // Очередь на 10 команд

int BOTTOM_INDEX = 0;        // светодиод начала отсчёта
int TOP_INDEX = int(LED_COUNT / 2);
int EVENODD = LED_COUNT % 2;
struct CRGB leds[LED_COUNT];
int ledsX[LED_COUNT][3];     //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, MARCH, ETC)

volatile int hsv;
volatile int thisbright = BRIGHT;
volatile int thisdelay = 20;          //-FX LOOPS DELAY VAR
int thisstep = 10;           //-FX LOOPS DELAY VAR
int thishue = 0;             //-FX LOOPS DELAY VAR
int thissat = 255;           //-FX LOOPS DELAY VAR

int thisindex = 0;
int thisRED = 0;
int thisGRN = 0;
int thisBLU = 0;

int idex = 0;                //-LED INDEX (0 to LED_COUNT-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)
int bouncedirection = 0;     //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount = 0.0;          //-INC VAR FOR SIN LOOPS
int lcount = 0;              //-ANOTHER COUNTING VAR
byte counter;
volatile int modeLed = 2;
unsigned long prev = 0;
byte ballColors[3][3] = {
  {0xff, 0, 0},
  {0xff, 0xff, 0xff},
  {0   , 0   , 0xff},
};

void setup() {
  ESP_BT.begin("ESP32_LED_Control"); 
  FastLED.addLeds<WS2812B, PIN, GRB>(leds, LED_COUNT).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(50);

     xTaskCreatePinnedToCore(
        processBluetooth,   // Функция задачи
        "BluetoothTask",    // Имя задачи
        10000,              // Размер стека
        NULL,               // Параметры
        1,                  // Приоритет
        NULL,               // Дескриптор задачи
        0                   // Ядро (0 или 1)
    );
//  Serial.begin(9600);

}
void processBluetooth(void *parameter) {
    while (1) {
        if (ESP_BT.available()) {
            String receivedData = ESP_BT.readStringUntil('\n');
            receivedData.trim();
            if (receivedData.startsWith("!") && receivedData.endsWith(";")) {
                xQueueSend(bluetoothQueue, &receivedData, portMAX_DELAY);  // Отправляем в очередь
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
}
void loop() {
    String receivedData;
    if (xQueueReceive(bluetoothQueue, &receivedData, 0)) { 
        parseBluetoothCommand(receivedData);
    }

    switch (modeLed){
    case 999: ; break;                         // пазуа
    case  2: rainbow_fade(); break;            // плавная смена цветов всей ленты
    case  3: rainbow_loop(); break;            // крутящаяся радуга
    case  4: random_burst(); break;            // случайная смена цветов
    case  5: color_bounce(); break;            // бегающий светодиод // цвет
    case  6: color_bounceFADE(); break;        // бегающий паровозик светодиодов // цвет
    case  7: ems_lightsONE(); break;           // вращаются красный и синий
    case  8: ems_lightsALL(); break;           // вращается половина красных и половина синих
    case  9: flicker(); break;                 // случайный стробоскоп // задержка
    case 10: pulse_one_color_all(); break;     // пульсация одним цветом // цвет
    case 11: fade_vertical(); break;           // плавная смена яркости по вертикали (для кольца)
    case 12: rule30(); break;                  // безумие красных светодиодов // цвет
    case 13: random_march(); break;            // безумие случайных цветов  
    case 14: rwb_march(); break;               // белый синий красный бегут по кругу (ПАТРИОТИЗМ!) 
    case 15: Russia_flag(); break;           // RWB пропеллер 
    case 16: radiation(); break;               // пульсирует значок радиации
    case 17: pop_horizontal(); break;          // красные вспышки спускаются вниз // цвет
    case 18: flame(); break;                   // эффект пламени 
    case 19: rainbow_vertical(); break;        // радуга в вертикаьной плоскости (кольцо)
    case 20: random_color_pop(); break;        // безумие случайных вспышек
    case 21: ems_lightsSTROBE(); break;        // полицейская мигалка
    case 22: rgb_propeller(); break;           // RGB пропеллер 
    case 23: kitt(); break;                    // случайные вспышки красного в вертикаьной плоскости // цвет
    case 24: matrix(); break;                  // зелёненькие бегают по кругу случайно // цвет
    case 25: new_rainbow_loop(); break;        // крутая плавная вращающаяся радуга 
    case 26: colorWipe(0x00, 0xff, 0x00, thisdelay);
      colorWipe(0x00, 0x00, 0x00, thisdelay); break;                                // плавное заполнение цветом // цвет
    case 27: CylonBounce(0xff, 0, 0, 4, 10, thisdelay); break;                      // бегающие светодиоды // цвет
    case 28: Fire(55, 120, thisdelay); break;                                       // линейный огонь // цвет
    case 29: rainbowCycle(thisdelay); break;                                        // очень плавная вращающаяся радуга 
    case 30: TwinkleRandom(20, thisdelay, 1); break;                                // случайные разноцветные включения (1 - танцуют все, 0 - случайный 1 диод)
    case 31: RunningLights(0xff, 0xff, 0x00, thisdelay); break;                     // бегущие огни
    case 32: Sparkle(0xff, 0xff, 0xff, thisdelay); break;                           // случайные вспышки белого цвета // скорсоть
    case 33: Strobe(0xff, 0xff, 0xff, 10, thisdelay, 1000); break;                  // стробоскоп // скорость цвет

    }
}

void parseBluetoothCommand(String command) {
    String cleanCmd = command.substring(1, command.length() - 1);
    int equalIndex = cleanCmd.indexOf('=');
    if (equalIndex == -1) return;

    String param = cleanCmd.substring(0, equalIndex);
    String valueStr = cleanCmd.substring(equalIndex + 1);

    if (param == "b") {
        FastLED.setBrightness(valueStr.toInt());
        thisbright = valueStr.toInt();
        FastLED.show();
    } 
    else if (param == "c") {
        int firstComma = valueStr.indexOf(',');
        int secondComma = valueStr.indexOf(',', firstComma + 1);
        if (firstComma != -1 && secondComma != -1) {
            int r = valueStr.substring(0, firstComma).toInt();
            int g = valueStr.substring(firstComma + 1, secondComma).toInt();
            int b = valueStr.substring(secondComma + 1).toInt();
            one_color_all(r, g, b);
            FastLED.show();
        }
    }
    else if (param == "m") {
        change_mode(valueStr.toInt());
    }
    else if (param == "s") {
        thisdelay = valueStr.toInt();
    }
}
void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i].setRGB( cred, cgrn, cblu);
  }
}
void change_mode(int newmode) {
  thissat = 255;
  switch (newmode) {
    case 0: one_color_all(0, 0, 0); LEDS.show(); break; //---ALL OFF
    case 1: one_color_all(255, 255, 255); LEDS.show(); break; //---ALL ON
    case 2: thisdelay = 20; break;                      //---STRIP RAINBOW FADE
    case 3: thisdelay = 20; thisstep = 10; break;       //---RAINBOW LOOP
    case 4: thisdelay = 20; break;                      //---RANDOM BURST
    case 5: thisdelay = 20; thishue = 0; break;         //---CYLON v1
    case 6: thisdelay = 40; thishue = 0; break;         //---CYLON v2
    case 7: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SINGLE
    case 8: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SOLID
    case 9: thishue = 160; thissat = 50; break;         //---STRIP FLICKER
    case 10: thisdelay = 15; thishue = 0; break;        //---PULSE COLOR BRIGHTNESS
    case 11: thisdelay = 60; thishue = 180; break;      //---VERTICAL SOMETHING
    case 12: thisdelay = 100; break;                    //---CELL AUTO - RULE 30 (RED)
    case 13: thisdelay = 40; break;                     //---MARCH RANDOM COLORS
    case 14: thisdelay = 80; break;                     //---MARCH RWB COLORS
    case 15: thisdelay = 25; break;                     //---RWB PROPELLER
    case 16: thisdelay = 60; thishue = 95; break;       //---RADIATION SYMBOL
    case 17: thisdelay = 100; thishue = 0; break;       //---POP LEFT/RIGHT
    case 19: thisdelay = 50; thisstep = 15; break;      //---VERITCAL RAINBOW
    case 20: thisdelay = 35; break;                     //---RANDOM COLOR POP
    case 21: thisdelay = 25; thishue = 0; break;        //---EMERGECNY STROBE
    case 22: thisdelay = 25; thishue = 0; break;        //---RGB PROPELLER
    case 23: thisdelay = 100; thishue = 0; break;       //---KITT
    case 24: thisdelay = 50; thishue = 95; break;       //---MATRIX RAIN
    case 26: thisdelay = 50; break;                     // colorWipe
    case 27: thisdelay = 50; break;                     // CylonBounce
    case 28: thisdelay = 15; break;                     // Fire
    case 29: thisdelay = 20; break;                     // rainbowCycle
    case 30: thisdelay = 10; break;                     // rainbowTwinkle
    case 31: thisdelay = 50; break;                     // RunningLights
    case 32: thisdelay = 0; break;                      // Sparkle
    case 33: thisdelay = 100; break;                    // Strobe

    case 101: one_color_all(255, 0, 0); LEDS.show(); break; //---ALL RED
    case 102: one_color_all(0, 255, 0); LEDS.show(); break; //---ALL GREEN
    case 103: one_color_all(0, 0, 255); LEDS.show(); break; //---ALL BLUE
    case 104: one_color_all(255, 255, 0); LEDS.show(); break; //---ALL COLOR X
    case 105: one_color_all(0, 255, 255); LEDS.show(); break; //---ALL COLOR Y
    case 106: one_color_all(255, 0, 255); LEDS.show(); break; //---ALL COLOR Z
  }
  bouncedirection = 0;
//  one_color_all(0, 0, 0);
  modeLed = newmode;
}

void rainbow_fade() {   
  if (millis() - prev >= thisdelay) {
    prev = millis();
    ihue++;
    if (ihue > 255) {
      ihue = 0;
    }
    for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
      leds[idex] = CHSV(ihue, 255, thisbright);
    }
    LEDS.show();
  }
}
void rainbow_loop() {
    if (millis() - prev >= thisdelay) {
    prev = millis();
  idex++;
  ihue = ihue + thisstep;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  if (ihue > 255) {
    ihue = 0;
  }
  leds[idex] = CHSV(ihue, thissat, thisbright);
  LEDS.show();
    }
// delay(thisdelay);
}
void random_burst() {
    if (millis() - prev >= thisdelay) {
    prev = millis();
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  leds[idex] = CHSV(ihue, thissat, thisbright);
  LEDS.show();
//  delay(thisdelay);
    }

}
void color_bounce() {
    if (millis() - prev >= thisdelay) {
    prev = millis();
  if (bouncedirection == 0) {
    idex = idex + 1;
    if (idex == LED_COUNT) {
      bouncedirection = 1;
      idex = idex - 1;
    }
  }
  if (bouncedirection == 1) {
    idex = idex - 1;
    if (idex == 0) {
      bouncedirection = 0;
    }
  }
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == idex) {
      leds[i] = CHSV(thishue, thissat, thisbright);
    }
    else {
      leds[i] = CHSV(0, 0, 0);
    }
  }
  LEDS.show();
    }
//  delay(thisdelay);
}
void color_bounceFADE() {
    if (millis() - prev >= thisdelay) {
    prev = millis();
  if (bouncedirection == 0) {
    idex = idex + 1;
    if (idex == LED_COUNT) {
      bouncedirection = 1;
      idex = idex - 1;
    }
  }
  if (bouncedirection == 1) {
    idex = idex - 1;
    if (idex == 0) {
      bouncedirection = 0;
    }
  }
  int iL1 = adjacent_cw(idex);
  int iL2 = adjacent_cw(iL1);
  int iL3 = adjacent_cw(iL2);
  int iR1 = adjacent_ccw(idex);
  int iR2 = adjacent_ccw(iR1);
  int iR3 = adjacent_ccw(iR2);
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == idex) {
      leds[i] = CHSV(thishue, thissat, 255);
    }
    else if (i == iL1) {
      leds[i] = CHSV(thishue, thissat, 150);
    }
    else if (i == iL2) {
      leds[i] = CHSV(thishue, thissat, 80);
    }
    else if (i == iL3) {
      leds[i] = CHSV(thishue, thissat, 20);
    }
    else if (i == iR1) {
      leds[i] = CHSV(thishue, thissat, 150);
    }
    else if (i == iR2) {
      leds[i] = CHSV(thishue, thissat, 80);
    }
    else if (i == iR3) {
      leds[i] = CHSV(thishue, thissat, 20);
    }
    else {
      leds[i] = CHSV(0, 0, 0);
    }
  }
  LEDS.show();
    }
//  delay(thisdelay);
}
void ems_lightsONE() {
    if (millis() - prev >= thisdelay) {
    prev = millis();
  idex++;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  int idexR = idex;
  int idexB = antipodal_index(idexR);
  int thathue = (thishue + 160) % 255;
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == idexR) {
      leds[i] = CHSV(thishue, thissat, thisbright);
    }
    else if (i == idexB) {
      leds[i] = CHSV(thathue, thissat, thisbright);
    }
    else {
      leds[i] = CHSV(0, 0, 0);
    }
  }
  LEDS.show();
    }
//  delay(thisdelay);
}
void ems_lightsALL() {
      if (millis() - prev >= thisdelay) {
    prev = millis();
  idex++;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  int idexR = idex;
  int idexB = antipodal_index(idexR);
  int thathue = (thishue + 160) % 255;
  leds[idexR] = CHSV(thishue, thissat, thisbright);
  leds[idexB] = CHSV(thathue, thissat, thisbright);
  LEDS.show();

      }
}
void flicker() {                          //-m9-FLICKER EFFECT
        if (millis() - prev >= thisdelay) {
    prev = millis();
  int random_bright = random(0, 255);
  int random_delay = random(10, 100);
  int random_bool = random(0, random_bright);
  if (random_bool < 10) {
    for (int i = 0 ; i < LED_COUNT; i++ ) {
      leds[i] = CHSV(thishue, thissat, random_bright);
    }
    LEDS.show();
    
  }
        }
}
int adjacent_cw(int i) {
  int r;
  if (i < LED_COUNT - 1) {
    r = i + 1;
  }
  else {
    r = 0;
  }
  return r;
}
int antipodal_index(int i) {
  int iN = i + TOP_INDEX;
  if (i >= TOP_INDEX) {
    iN = ( i + TOP_INDEX ) % LED_COUNT;
  }
  return iN;
}
int adjacent_ccw(int i) {
  int r;
  if (i > 0) {
    r = i - 1;
  }
  else {
    r = LED_COUNT - 1;
  }
  return r;
}
void pulse_one_color_all() {              //-m10-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR
      if (millis() - prev >= thisdelay) {
    prev = millis();
  if (bouncedirection == 0) {
    ibright++;
    if (ibright >= 255) {
      bouncedirection = 1;
    }
  }
  if (bouncedirection == 1) {
    ibright = ibright - 1;
    if (ibright <= 1) {
      bouncedirection = 0;
    }
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(thishue, thissat, ibright);
  }
  LEDS.show();
//  delay(thisdelay);
      }
}
void fade_vertical() {                    //-m12-FADE 'UP' THE LOOP
      if (millis() - prev >= thisdelay) {
    prev = millis();
  idex++;
  if (idex > TOP_INDEX) {
    idex = 0;
  }
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  ibright = ibright + 10;
  if (ibright > 255) {
    ibright = 0;
  }
  leds[idexA] = CHSV(thishue, thissat, ibright);
  leds[idexB] = CHSV(thishue, thissat, ibright);
  LEDS.show();
//  delay(thisdelay);
}
}
void random_red() {                       //QUICK 'N DIRTY RANDOMIZE TO GET CELL AUTOMATA STARTED
  int temprand;
  for (int i = 0; i < LED_COUNT; i++ ) {
    temprand = random(0, 100);
    if (temprand > 50) {
      leds[i].r = 255;
    }
    if (temprand <= 50) {
      leds[i].r = 0;
    }
    leds[i].b = 0; leds[i].g = 0;
  }
  LEDS.show();
}
void rule30() {                          //-m13-1D CELLULAR AUTOMATA - RULE 30 (RED FOR NOW)
      if (millis() - prev >= thisdelay) {
    prev = millis();
  if (bouncedirection == 0) {
    random_red();
    bouncedirection = 1;
  }
  copy_led_array();
  int iCW;
  int iCCW;
  int y = 100;
  for (int i = 0; i < LED_COUNT; i++ ) {
    iCW = adjacent_cw(i);
    iCCW = adjacent_ccw(i);
    if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] > y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {
      leds[i].r = 0;
    }
    if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] > y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {
      leds[i].r = 255;
    }
    if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {
      leds[i].r = 0;
    }
  }
  LEDS.show();
//  delay(thisdelay);
}
}
void random_march() {                   //-m14-RANDOM MARCH CCW
      if (millis() - prev >= thisdelay) {
    prev = millis();
  copy_led_array();
  int iCCW;
  leds[0] = CHSV(random(0, 255), 255, thisbright);
  for (int idex = 1; idex < LED_COUNT ; idex++ ) {
    iCCW = adjacent_ccw(idex);
    leds[idex].r = ledsX[iCCW][0];
    leds[idex].g = ledsX[iCCW][1];
    leds[idex].b = ledsX[iCCW][2];
  }
  LEDS.show();
//  delay(thisdelay);
}
}
void rwb_march() {                    //-m15-R,W,B MARCH CCW
      if (millis() - prev >= thisdelay) {
    prev = millis();
  copy_led_array();
  int iCCW;
  idex++;
  if (idex > 2) {
    idex = 0;
  }
  switch (idex) {
    case 0:
      leds[0].r = 255;
      leds[0].g = 0;
      leds[0].b = 0;
      break;
    case 1:
      leds[0].r = 255;
      leds[0].g = 255;
      leds[0].b = 255;
      break;
    case 2:
      leds[0].r = 0;
      leds[0].g = 0;
      leds[0].b = 255;
      break;
  }
  for (int i = 1; i < LED_COUNT; i++ ) {
    iCCW = adjacent_ccw(i);
    leds[i].r = ledsX[iCCW][0];
    leds[i].g = ledsX[iCCW][1];
    leds[i].b = ledsX[iCCW][2];
  }
  LEDS.show();
}
}
void radiation() {                   //-m16-SORT OF RADIATION SYMBOLISH-
      if (millis() - prev >= thisdelay) {
    prev = millis();
  int N3  = int(LED_COUNT / 3);
  int N6  = int(LED_COUNT / 6);
  int N12 = int(LED_COUNT / 12);
  for (int i = 0; i < N6; i++ ) {    //-HACKY, I KNOW...
    tcount = tcount + .02;
    if (tcount > 3.14) {
      tcount = 0.0;
    }
    ibright = int(sin(tcount) * 255);
    int j0 = (i + LED_COUNT - N12) % LED_COUNT;
    int j1 = (j0 + N3) % LED_COUNT;
    int j2 = (j1 + N3) % LED_COUNT;
    leds[j0] = CHSV(thishue, thissat, ibright);
    leds[j1] = CHSV(thishue, thissat, ibright);
    leds[j2] = CHSV(thishue, thissat, ibright);
  }
  LEDS.show();
//  delay(thisdelay);
}
}
void pop_horizontal() {        //-m20-POP FROM LEFT TO RIGHT UP THE RING
      if (millis() - prev >= thisdelay) {
    prev = millis();
  int ix;
  if (bouncedirection == 0) {
    bouncedirection = 1;
    ix = idex;
  }
  else if (bouncedirection == 1) {
    bouncedirection = 0;
    ix = horizontal_index(idex);
    idex++;
    if (idex > TOP_INDEX) {
      idex = 0;
    }
  }
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i == ix) {
      leds[i] = CHSV(thishue, thissat, thisbright);
    }
    else {
      leds[i].r = 0; leds[i].g = 0; leds[i].b = 0;
    }
  }
  LEDS.show();
//  delay(thisdelay);
}
}
void flame() {                                    //-m22-FLAMEISH EFFECT
  int idelay = random(0, 35);
  float hmin = 0.1; float hmax = 45.0;
  float hdif = hmax - hmin;
  int randtemp = random(0, 3);
  float hinc = (hdif / float(TOP_INDEX)) + randtemp;
  int ihue = hmin;
  for (int i = 0; i <= TOP_INDEX; i++ ) {
    ihue = ihue + hinc;
    leds[i] = CHSV(ihue, thissat, thisbright);
    int ih = horizontal_index(i);
    leds[ih] = CHSV(ihue, thissat, thisbright);
    leds[TOP_INDEX].r = 255; leds[TOP_INDEX].g = 255; leds[TOP_INDEX].b = 255;
    LEDS.show();
    delay(idelay);
  }
}
void rainbow_vertical() {                        //-m23-RAINBOW 'UP' THE LOOP
      if (millis() - prev >= thisdelay) {
    prev = millis();
  idex++;
  if (idex > TOP_INDEX) {
    idex = 0;
  }
  ihue = ihue + thisstep;
  if (ihue > 255) {
    ihue = 0;
  }
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  leds[idexA] = CHSV(ihue, thissat, thisbright);
  leds[idexB] = CHSV(ihue, thissat, thisbright);
  LEDS.show();
//  delay(thisdelay);
      }
}
void random_color_pop() {                         //-m25-RANDOM COLOR POP
      if (millis() - prev >= thisdelay) {
    prev = millis();
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  one_color_all(0, 0, 0);
  leds[idex] = CHSV(ihue, thissat, thisbright);
  LEDS.show();
}
}

void ems_lightsSTROBE() {                  //-m26-EMERGENCY LIGHTS (STROBE LEFT/RIGHT)
  int thishue = 0;
  int thathue = (thishue + 160) % 255;
  for (int x = 0 ; x < 5; x++ ) {
    for (int i = 0 ; i < TOP_INDEX; i++ ) {
      leds[i] = CHSV(thishue, thissat, thisbright);
    }
    LEDS.show(); delay(thisdelay);
    one_color_all(0, 0, 0);
    LEDS.show(); delay(thisdelay);
  }
  for (int x = 0 ; x < 5; x++ ) {
    for (int i = TOP_INDEX ; i < LED_COUNT; i++ ) {
      leds[i] = CHSV(thathue, thissat, thisbright);
    }
    LEDS.show(); delay(thisdelay);
    one_color_all(0, 0, 0);
    LEDS.show(); delay(thisdelay);
  }
}
void Russia_flag() {                           //-m15-RWB PROPELLER
      if (millis() - prev >= thisdelay) {
    prev = millis();
  idex++;
  int N3  = int(LED_COUNT / 3);
  int N6  = int(LED_COUNT / 6);
  int N12 = int(LED_COUNT / 12);
  for (int i = 0; i < N3; i++ ) {
    int j0 = (idex + i + LED_COUNT - N12) % LED_COUNT;
    int j1 = (j0 + N3) % LED_COUNT;
    int j2 = (j1 + N3) % LED_COUNT;
    leds[j0] = CHSV(0, 0, thisbright);
    leds[j1] = CHSV(160, 255, thisbright);
    leds[j2] = CHSV(0, 255, thisbright);
  }
  LEDS.show();
//  delay(thisdelay);
}
}
void rgb_propeller() {                           //-m27-RGB PROPELLER
      if (millis() - prev >= thisdelay) {
    prev = millis();
  idex++;
  int ghue = (thishue + 80) % 255;
  int bhue = (thishue + 160) % 255;
  int N3  = int(LED_COUNT / 3);
  int N6  = int(LED_COUNT / 6);
  int N12 = int(LED_COUNT / 12);
  for (int i = 0; i < N3; i++ ) {
    int j0 = (idex + i + LED_COUNT - N12) % LED_COUNT;
    int j1 = (j0 + N3) % LED_COUNT;
    int j2 = (j1 + N3) % LED_COUNT;
    leds[j0] = CHSV(thishue, thissat, thisbright);
    leds[j1] = CHSV(ghue, thissat, thisbright);
    leds[j2] = CHSV(bhue, thissat, thisbright);
  }
  LEDS.show();
//  delay(thisdelay);
}
}

void kitt() {                                     //-m28-KNIGHT INDUSTIES 2000
  int rand = random(0, TOP_INDEX);
  for (int i = 0; i < rand; i++ ) {
    leds[TOP_INDEX + i] = CHSV(thishue, thissat, thisbright);
    leds[TOP_INDEX - i] = CHSV(thishue, thissat, thisbright);
    LEDS.show();
    delay(thisdelay / rand);
  }
  for (int i = rand; i > 0; i-- ) {
    leds[TOP_INDEX + i] = CHSV(thishue, thissat, 0);
    leds[TOP_INDEX - i] = CHSV(thishue, thissat, 0);
    LEDS.show();
    delay(thisdelay / rand);
  }
}

void matrix() {                                   //-m29-ONE LINE MATRIX
      if (millis() - prev >= thisdelay) {
    prev = millis();
  int rand = random(0, 100);
  if (rand > 90) {
    leds[0] = CHSV(thishue, thissat, thisbright);
  }
  else {
    leds[0] = CHSV(thishue, thissat, 0);
  }
  copy_led_array();
  for (int i = 1; i < LED_COUNT; i++ ) {
    leds[i].r = ledsX[i - 1][0];
    leds[i].g = ledsX[i - 1][1];
    leds[i].b = ledsX[i - 1][2];
  }
  LEDS.show();
//  delay(thisdelay);
}
}
void new_rainbow_loop() {                      //-m88-RAINBOW FADE FROM FAST_SPI2
      if (millis() - prev >= thisdelay) {
    prev = millis();
  ihue -= 1;
  fill_rainbow( leds, LED_COUNT, ihue );
  LEDS.show();
//  delay(thisdelay);
      }
}
void colorWipe(byte red, byte green, byte blue, int SpeedDelay) {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    setPixel(i, red, green, blue);
    FastLED.show();
    delay(SpeedDelay);
  }
}
void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {

  for (int i = 0; i < LED_COUNT - EyeSize - 2; i++) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    FastLED.show();
    delay(SpeedDelay);
  }

  delay(ReturnDelay);

  for (int i = LED_COUNT - EyeSize - 2; i > 0; i--) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    FastLED.show();
    delay(SpeedDelay);
  }

  delay(ReturnDelay);
}
void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[LED_COUNT];
  int cooldown;

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < LED_COUNT; i++) {
    cooldown = random(0, ((Cooling * 10) / LED_COUNT) + 2);

    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = LED_COUNT - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if ( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for ( int j = 0; j < LED_COUNT; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  FastLED.show();
  delay(SpeedDelay);
}
void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature / 255.0) * 191);

  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252

  // figure out which third of the spectrum we're in:
  if ( t192 > 0x80) {                    // hottest
    setPixel(Pixel, 255, 255, heatramp);
  } else if ( t192 > 0x40 ) {            // middle
    setPixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0);
  }
}
void NewKITT(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
}

void CenterToOutside(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = ((LED_COUNT - EyeSize) / 2); i >= 0; i--) {
    setAll(0, 0, 0);

    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);

    setPixel(LED_COUNT - i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(LED_COUNT - i - j, red, green, blue);
    }
    setPixel(LED_COUNT - i - EyeSize - 1, red / 10, green / 10, blue / 10);

    FastLED.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void OutsideToCenter(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = 0; i <= ((LED_COUNT - EyeSize) / 2); i++) {
    setAll(0, 0, 0);

    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);

    setPixel(LED_COUNT - i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(LED_COUNT - i - j, red, green, blue);
    }
    setPixel(LED_COUNT - i - EyeSize - 1, red / 10, green / 10, blue / 10);

    FastLED.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void LeftToRight(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = 0; i < LED_COUNT - EyeSize - 2; i++) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    FastLED.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void RightToLeft(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for (int i = LED_COUNT - EyeSize - 2; i > 0; i--) {
    setAll(0, 0, 0);
    setPixel(i, red / 10, green / 10, blue / 10);
    for (int j = 1; j <= EyeSize; j++) {
      setPixel(i + j, red, green, blue);
    }
    setPixel(i + EyeSize + 1, red / 10, green / 10, blue / 10);
    FastLED.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}
void rainbowCycle(int SpeedDelay) {
//      if (millis() - prev >= SpeedDelay) {
//    prev = millis();
  byte *c;
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel

    for (i = 0; i < LED_COUNT; i++) {
      c = Wheel(((i * 256 / LED_COUNT) + j) & 255);
      setPixel(i, *c, *(c + 1), *(c + 2));
    }
    FastLED.show();
    delay(SpeedDelay);
//  }
      }
}
byte * Wheel(byte WheelPos) {
  static byte c[3];

  if (WheelPos < 85) {
    c[0] = WheelPos * 3;
    c[1] = 255 - WheelPos * 3;
    c[2] = 0;
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    c[0] = 255 - WheelPos * 3;
    c[1] = 0;
    c[2] = WheelPos * 3;
  } else {
    WheelPos -= 170;
    c[0] = 0;
    c[1] = WheelPos * 3;
    c[2] = 255 - WheelPos * 3;
  }

  return c;
}
void TwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0, 0, 0);

  for (int i = 0; i < Count; i++) {
          if (millis() - prev >= thisdelay) {
    prev = millis();
    setPixel(random(LED_COUNT), random(0, 255), random(0, 255), random(0, 255));
    FastLED.show();
          }
//    delay(SpeedDelay);
    if (OnlyOne) {
      setAll(0, 0, 0);
    }
  }

  delay(SpeedDelay);
}
void RunningLights(byte red, byte green, byte blue, int WaveDelay) {
  int Position = 0;
        
  for (int i = 0; i < LED_COUNT * 2; i++)
  {
//    if (millis() - prev >= thisdelay) {
//    prev = millis();
    Position++; // = 0; //Position + Rate;
    for (int i = 0; i < LED_COUNT; i++) {
      setPixel(i, ((sin(i + Position) * 127 + 128) / 255)*red,
               ((sin(i + Position) * 127 + 128) / 255)*green,
               ((sin(i + Position) * 127 + 128) / 255)*blue);
    }

    FastLED.show();
    delay(WaveDelay);
//  }
  }
}
void Sparkle(byte red, byte green, byte blue, int SpeedDelay) {
        if (millis() - prev >= thisdelay) {
    prev = millis();
  int Pixel = random(LED_COUNT);
  setPixel(Pixel, red, green, blue);
  FastLED.show();
//  delay(SpeedDelay);
        
  setPixel(Pixel, 0, 0, 0);
        }
}
void Strobe(byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause) {
  for (int j = 0; j < StrobeCount; j++) {
    setAll(red, green, blue);
    FastLED.show();
    delay(FlashDelay);
    setAll(0, 0, 0);
    FastLED.show();
    delay(FlashDelay);
  }

  delay(EndPause);
}
int horizontal_index(int i) {
  //-ONLY WORKS WITH INDEX < TOPINDEX
  if (i == BOTTOM_INDEX) {
    return BOTTOM_INDEX;
  }
  if (i == TOP_INDEX && EVENODD == 1) {
    return TOP_INDEX + 1;
  }
  if (i == TOP_INDEX && EVENODD == 0) {
    return TOP_INDEX;
  }
  return LED_COUNT - i;
}
void copy_led_array() {
  for (int i = 0; i < LED_COUNT; i++ ) {
    ledsX[i][0] = leds[i].r;
    ledsX[i][1] = leds[i].g;
    ledsX[i][2] = leds[i].b;
  }
}
void set_color_led(int adex, int cred, int cgrn, int cblu) {
  leds[adex].setRGB( cred, cgrn, cblu);
}
void setPixel(int Pixel, byte red, byte green, byte blue) {
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
}

void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < LED_COUNT; i++ ) {
    setPixel(i, red, green, blue);
  }
  LEDS.show();
}
