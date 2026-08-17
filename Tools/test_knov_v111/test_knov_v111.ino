/*
 ============================================================
  TEST COMPLET — Platine Knov V3.3 REV 3.41
  Arduino Mega 2560 Pro Mini
  Version : 1.0.11  |  Juin 2026

  MAPPING DEFINITIF v1.0.11
  ════════════════════════════════════════════════════════════

  WS2812B :
    PIN_LOC              = D7   GPIO7  PWM
    PIN_GS               = D6   GPIO6  PWM

  DM13A SPI HARDWARE :
    DM13A_EN (OE)        = D5   GPIO5  PWM  actif bas
    DM13A_LAT            = D49  libre        latch enable
    DM13A_SS             = D53  GPIO53       chip select (CN5)
    DM13A_SCK            = D52  GPIO52       SPI hardware
    DM13A_MOSI           = D51  GPIO51       SPI hardware
    DM13A_MISO           = D50  GPIO50       SPI hardware (non utilisé)
    (DCK/DAI via CN4 supprimés — SPI hardware D52/D51 suffisent)

  Son NE555 :
    NE555_SOUND          = D43  GPIO43  CN4 IDC18

  Encodeur FMS (EC11EBB24C03) :
    ENC_FMS_A1           = D16  GPIO16  CN3
    ENC_FMS_A2           = D17  GPIO17  CN3
    ENC_FMS_B1           = D20  GPIO20  CN3
    ENC_FMS_B2           = D21  GPIO21  CN3
    ENC_FMS_SW           = D8   GPIO8   CN3 col gauche

  Encodeur CRS/BARO (EC11EBB24C03) :
    ENC_CRS_A1           = D24  GPIO24  CN4
    ENC_CRS_A2           = D25  GPIO25  CN4
    ENC_CRS_B1           = D22  GPIO22  CN4
    ENC_CRS_B2           = D23  GPIO23  CN4
    ENC_CRS_SW           = D11  GPIO11  CN3 col gauche

  Joystick RKJXT1F42001 (4 directions + rotation + push) :
    JOY_UP               = D26  GPIO26  CN4 IDC11
    JOY_LEFT             = D27  GPIO27  CN4 IDC13
    JOY_DOWN             = D28  GPIO28  CN4 IDC15
    JOY_RIGHT            = D29  GPIO29  CN4 IDC17
    JOY_PUSH             = D15  GPIO15  CN3
    ROTATE_LEFT          = D18  GPIO18  CN3
    ROTATE_RIGHT         = D19  GPIO19  CN3

  Boutons G1000 (via CN4) :
    BTN_FPL              = D32  GPIO31  CN4 IDC2
    BTN_PROC             = D35  GPIO32  CN4 IDC4
    BTN_CLR              = D31  GPIO30  CN4 IDC19
    BTN_MENU             = D34  GPIO35  CN4 IDC10
    BTN_ENT              = D30  GPIO34  CN4 IDC8
    BTN_DIRECT           = D33  GPIO33  CN4 IDC6

  Master Alarm / Master Caution :
    BTN_MASTER_CAUTION   = D38  GPIO38  CN4 IDC16  (entrée bouton)
    BTN_MASTER_ALARM     = D36  GPIO36  CN4 IDC12  (entrée switch)
    LED_MASTER_CAUTION   = D39  GPIO39             (sortie LED)
    LED_MASTER_ALARM     = D37  GPIO37  CN4 IDC14  (sortie LED)

  COMMANDES WEB SERIE :
    LOC:IDX=<0-8>        → LED LOC index
    GS:IDX=<0-8>         → LED GS index
    BL:OE=<0-255>        → Luminosité DM13A (255=max, 0=éteint)
    BL:DM13A=0xXXXXXXXX → Masque 32 canaux
    LED:CAUTION=0/1      → LED Master Caution
    LED:ALARM=0/1        → LED Master Alarm
    SND:BIP=<n>          → n bips NE555
    n                    → Fin phase interactive
 ============================================================
*/

#include <FastLED.h>
#include <SPI.h>

// ════════════════════════════════════════════════════════════
// DEFINES — MAPPING v1.0.11
// ════════════════════════════════════════════════════════════

// ── WS2812B ─────────────────────────────────────────────────
#define PIN_LOC        7    // GPIO7  → D7  PWM — LOC DIN
#define PIN_GS         6    // GPIO6  → D6  PWM — GS DIN
#define NUM_LEDS       9
#define BRIGHT        40    // 40/255 ≈ 16%

// ── DM13A SPI HARDWARE ──────────────────────────────────────
#define DM13A_EN       5    // GPIO5  → D5  PWM   EN# actif bas (luminosité)
#define DM13A_LAT     49    // D49    libre        LAT latch enable
#define DM13A_SS      53    // GPIO53 → D53        SS  chip select (CN5)
#define DM13A_SCK     52    // GPIO52 → D52        SCK SPI hardware
#define DM13A_MOSI    51    // GPIO51 → D51        MOSI SPI hardware
#define DM13A_MISO    50    // GPIO50 → D50        MISO SPI hardware (non utilisé)

// ── Son NE555 ────────────────────────────────────────────────
#define NE555_SOUND   43    // GPIO43 → D43        CN4 IDC18 — alarme sonore

// ── Encodeur FMS (EC11EBB24C03) ──────────────────────────────
#define ENC_FMS_A1    16    // GPIO16 → D16  CN3
#define ENC_FMS_A2    17    // GPIO17 → D17  CN3
#define ENC_FMS_B1    20    // GPIO20 → D20  CN3
#define ENC_FMS_B2    21    // GPIO21 → D21  CN3
#define ENC_FMS_SW     8    // GPIO8  → D8   CN3 col gauche

// ── Encodeur CRS/BARO (EC11EBB24C03) ─────────────────────────
#define ENC_CRS_A1    24    // GPIO24 → D24  CN4
#define ENC_CRS_A2    25    // GPIO25 → D25  CN4
#define ENC_CRS_B1    22    // GPIO22 → D22  CN4
#define ENC_CRS_B2    23    // GPIO23 → D23  CN4
#define ENC_CRS_SW    11    // GPIO11 → D11  CN3 col gauche

// ── Joystick RKJXT1F42001 (4 directions + rotation + push) ───
#define JOY_UP        26    // GPIO26 → D26  CN4 IDC11 — direction HAUT
#define JOY_LEFT      27    // GPIO27 → D27  CN4 IDC13 — direction GAUCHE
#define JOY_DOWN      28    // GPIO28 → D28  CN4 IDC15 — direction BAS
#define JOY_RIGHT     29    // GPIO29 → D29  CN4 IDC17 — direction DROITE
#define JOY_PUSH      15    // GPIO15 → D15  CN3       — push centre
#define ROTATE_LEFT   18    // GPIO18 → D18  CN3       — rotation gauche
#define ROTATE_RIGHT  19    // GPIO19 → D19  CN3       — rotation droite

// ── Boutons G1000 (via CN4) ──────────────────────────────────
#define BTN_FPL       32    // GPIO31 → D32  CN4 IDC2
#define BTN_PROC      35    // GPIO32 → D35  CN4 IDC4
#define BTN_CLR       31    // GPIO30 → D31  CN4 IDC19
#define BTN_MENU      34    // GPIO35 → D34  CN4 IDC10
#define BTN_ENT       30    // GPIO34 → D30  CN4 IDC8
#define BTN_DIRECT    33    // GPIO33 → D33  CN4 IDC6

// ── Master Alarm / Master Caution ────────────────────────────
#define BTN_MASTER_CAUTION   38   // GPIO38 → D38  CN4 IDC16 (entrée bouton)
#define BTN_MASTER_ALARM     36   // GPIO36 → D36  CN4 IDC12 (entrée switch)
#define LED_MASTER_CAUTION   39   // GPIO39 → D39  sortie LED Caution
#define LED_MASTER_ALARM     37   // GPIO37 → D37  CN4 IDC14 sortie LED Alarm

// ════════════════════════════════════════════════════════════
// PALETTE COULEURS ILS
// ════════════════════════════════════════════════════════════
const CRGB PAL[9] = {
  CRGB(255,  0,  0),  // 0 rouge vif    déviation max
  CRGB(255, 60,  0),  // 1 orange foncé
  CRGB(255,120,  0),  // 2 orange
  CRGB(220,200,  0),  // 3 jaune
  CRGB(  0,255,  0),  // 4 vert         CENTRE / SUR LE PLAN
  CRGB(220,200,  0),  // 5 jaune
  CRGB(255,120,  0),  // 6 orange
  CRGB(255, 60,  0),  // 7 orange foncé
  CRGB(255,  0,  0),  // 8 rouge vif    déviation max
};

// ════════════════════════════════════════════════════════════
// VARIABLES GLOBALES
// ════════════════════════════════════════════════════════════
CRGB ledsLOC[9], ledsGS[9];
String sBuf = "";
bool running = true;

struct Res { const char* n; bool ok; bool w; char m[80]; };
Res results[24];
int rCnt = 0;

// ════════════════════════════════════════════════════════════
// LOG SERIE
// ════════════════════════════════════════════════════════════
void sep()  { Serial.println(F("--------------------------------------------------")); }
void hdr(const char* t) { sep(); Serial.print(F("  ")); Serial.println(t); sep(); }

void logOK(const char* n, const char* d="") {
  Serial.print(F("[OK]   ")); Serial.print(n);
  if(*d){ Serial.print(F(" -- ")); Serial.print(d); } Serial.println();
  if(rCnt<24){ results[rCnt]={n,true,false}; strncpy(results[rCnt].m,d,79); rCnt++; }
}
void logW(const char* n, const char* d) {
  Serial.print(F("[WARN] ")); Serial.print(n);
  Serial.print(F(" -- ")); Serial.println(d);
  if(rCnt<24){ results[rCnt]={n,true,true}; strncpy(results[rCnt].m,d,79); rCnt++; }
}
void logF(const char* n, const char* d) {
  Serial.print(F("[FAIL] ")); Serial.print(n);
  Serial.print(F(" -- ")); Serial.println(d);
  if(rCnt<24){ results[rCnt]={n,false,false}; strncpy(results[rCnt].m,d,79); rCnt++; }
}
void logE(const char* d) { Serial.print(F("[ERROR] ")); Serial.println(d); }

// ════════════════════════════════════════════════════════════
// WS2812B
// ════════════════════════════════════════════════════════════
void showLED(CRGB* s, int i) {
  i = constrain(i, 0, 8);
  for(int j=0;j<9;j++) s[j] = (j==i) ? PAL[i] : CRGB::Black;
  FastLED.show();
}
void clrAll() {
  fill_solid(ledsLOC, 9, CRGB::Black);
  fill_solid(ledsGS,  9, CRGB::Black);
  FastLED.show();
}

// ════════════════════════════════════════════════════════════
// DM13A — SPI HARDWARE + LAT D49
// Séquence validée :
//   SS LOW → transfer16 hi (U3 slave) → transfer16 lo (U2 master)
//   → SS HIGH → pulse LAT
// ════════════════════════════════════════════════════════════
void dm13a(uint32_t mask) {
  // Protocole DM13A daisy-chain :
  // Arduino MOSI → DAI[U2 master] → DAO[U2] → DAI[U3 slave]
  //
  // Registre à décalage 16 bits par puce.
  // Transfer 1 (hi) : entre dans U2, U3 vide
  // Transfer 2 (lo) : entre dans U2, pousse hi vers U3
  // → hi finit dans U3 slave, lo reste dans U2 master
  //
  // On envoie 3 x 16 bits pour garantir que le slave est chargé
  // même si le registre était décalé d'un bit ou d'un mot :
  //   Transfer 1 (purge) : pousse tout résidu hors de U3
  //   Transfer 2 (hi)    : entre dans U2, purge vers U3
  //   Transfer 3 (lo)    : entre dans U2, hi vers U3
  //   → hi dans U3 slave, lo dans U2 master ✓
  //
  // OE désactivé pendant le transfert pour éviter interférence Timer3/PWM

  uint16_t hi = (uint16_t)(mask >> 16);    // pour U3 slave
  uint16_t lo = (uint16_t)(mask & 0xFFFF); // pour U2 master

  digitalWrite(DM13A_EN, HIGH);   // OE HIGH = sorties isolées pendant transfert
  delayMicroseconds(2);

  digitalWrite(DM13A_SS, LOW);
  SPI.transfer16(lo);              // transfer 1 : purge — pousse résidu hors U3
  SPI.transfer16(hi);              // transfer 2 : hi entre dans U2, purge→U3
  SPI.transfer16(lo);              // transfer 3 : lo entre dans U2, hi→U3 slave
  digitalWrite(DM13A_SS, HIGH);

  digitalWrite(DM13A_LAT, HIGH);
  delayMicroseconds(5);
  digitalWrite(DM13A_LAT, LOW);

  digitalWrite(DM13A_EN, LOW);    // OE LOW = sorties actives (max luminosité)
}

// OE actif bas : setBL(255)=max luminosité, setBL(0)=éteint
void setBL(int v) {
  analogWrite(DM13A_EN, 255 - constrain(v, 0, 255));
}

void dm13aInit() {
  pinMode(DM13A_SS,  OUTPUT); digitalWrite(DM13A_SS,  HIGH);
  pinMode(DM13A_LAT, OUTPUT); digitalWrite(DM13A_LAT, LOW);
  pinMode(DM13A_EN,  OUTPUT); digitalWrite(DM13A_EN,  LOW); // OE LOW = sorties actives max
  pinMode(DM13A_MISO, INPUT);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV16);
}

// ════════════════════════════════════════════════════════════
// SON NE555 — D43/GPIO43
// ════════════════════════════════════════════════════════════
void bip(int n, int dur_ms) {
  for(int i=0;i<n;i++){
    digitalWrite(NE555_SOUND, HIGH);
    Serial.println(F("GPIO:SOUND=1"));
    delay(dur_ms);
    digitalWrite(NE555_SOUND, LOW);
    Serial.println(F("GPIO:SOUND=0"));
    if(i<n-1) delay(dur_ms/2);
  }
}

// ════════════════════════════════════════════════════════════
// PARSEUR COMMANDES WEB
// ════════════════════════════════════════════════════════════
bool parseCmd(String& l) {
  l.trim();
  if(!l.length()) return false;

  if(l.startsWith("LOC:IDX=")) {
    int i = constrain(l.substring(8).toInt(), 0, 8);
    showLED(ledsLOC, i);
    Serial.print(F("LOC:IDX=")); Serial.println(i);
    return true;
  }
  if(l.startsWith("GS:IDX=")) {
    int i = constrain(l.substring(7).toInt(), 0, 8);
    showLED(ledsGS, i);
    Serial.print(F("GS:IDX=")); Serial.println(i);
    return true;
  }
  if(l.startsWith("BL:OE=")) {
    int v = constrain(l.substring(6).toInt(), 0, 255);
    setBL(v);
    Serial.print(F("OE:PCT=")); Serial.println(map(v,0,255,0,100));
    return true;
  }
  if(l.startsWith("BL:DM13A=")) {
    String h = l.substring(9);
    if(h.startsWith("0x")||h.startsWith("0X")) h = h.substring(2);
    uint32_t m = strtoul(h.c_str(), nullptr, 16);
    dm13a(m);
    char buf[10]; sprintf(buf,"%08lX",m);
    Serial.print(F("DM13A:MASK=0x")); Serial.println(buf);
    return true;
  }
  if(l.startsWith("LED:CAUTION=")) {
    int v = l.substring(12).toInt();
    digitalWrite(LED_MASTER_CAUTION, v?HIGH:LOW);
    Serial.print(F("GPIO:LED_CAUTION=")); Serial.println(v);
    return true;
  }
  if(l.startsWith("LED:ALARM=")) {
    int v = l.substring(10).toInt();
    digitalWrite(LED_MASTER_ALARM, v?HIGH:LOW);
    Serial.print(F("GPIO:LED_ALARM=")); Serial.println(v);
    return true;
  }
  if(l.startsWith("SND:BIP=")) {
    int n = constrain(l.substring(8).toInt(), 1, 10);
    bip(n, 200);
    return true;
  }
  return false;
}

// ════════════════════════════════════════════════════════════
// TESTS AUTOMATIQUES
// ════════════════════════════════════════════════════════════

void testPower() {
  Serial.println(F("\n>> TEST 1 : VCC"));
  long v=0;
  ADMUX=_BV(REFS0)|_BV(MUX4)|_BV(MUX3)|_BV(MUX2)|_BV(MUX1);
  delay(2); ADCSRA|=_BV(ADSC); while(bit_is_set(ADCSRA,ADSC));
  v=ADCL; v|=ADCH<<8; v=1125300L/v;
  Serial.print(F("   VCC = ")); Serial.print(v); Serial.println(F(" mV"));
  char msg[30]; sprintf(msg,"VCC=%ldmV",v);
  if(v>=4700&&v<=5300)      logOK("VCC 5V",msg);
  else if(v>=4500)           logW("VCC 5V","Chute USB");
  else { logF("VCC 5V",msg); logE("Tension basse!"); }
}

void testLOC() {
  Serial.println(F("\n>> TEST 2 : WS2812B LOC D7/GPIO7"));
  fill_solid(ledsLOC,9,CRGB::Red);   FastLED.show(); delay(300);
  fill_solid(ledsLOC,9,CRGB::Green); FastLED.show(); delay(300);
  fill_solid(ledsLOC,9,CRGB::Blue);  FastLED.show(); delay(300);
  for(int i=0;i<9;i++){
    fill_solid(ledsLOC,9,CRGB::Black); ledsLOC[i]=PAL[i]; FastLED.show();
    Serial.print(F("LOC:IDX=")); Serial.println(i); delay(60);
  }
  fill_solid(ledsLOC,9,CRGB::Black); FastLED.show();
  logW("WS2812B LOC D7/GPIO7","Verif visuelle R->V->B->sweep ?");
}

void testGS() {
  Serial.println(F("\n>> TEST 3 : WS2812B GS D6/GPIO6"));
  fill_solid(ledsGS,9,CRGB::Red);   FastLED.show(); delay(300);
  fill_solid(ledsGS,9,CRGB::Green); FastLED.show(); delay(300);
  fill_solid(ledsGS,9,CRGB::Blue);  FastLED.show(); delay(300);
  for(int i=0;i<9;i++){
    fill_solid(ledsGS,9,CRGB::Black); ledsGS[i]=PAL[i]; FastLED.show();
    Serial.print(F("GS:IDX=")); Serial.println(i); delay(60);
  }
  fill_solid(ledsGS,9,CRGB::Black); FastLED.show();
  logW("WS2812B GS D6/GPIO6","Verif visuelle R->V->B->sweep ?");
}

void testILS() {
  Serial.println(F("\n>> TEST 4 : Simulation ILS LOC+GS"));
  int seq[]={4,3,2,1,0,1,2,3,4,5,6,7,8,7,6,5,4};
  for(int s=0;s<17;s++){
    int i=seq[s];
    fill_solid(ledsLOC,9,CRGB::Black); fill_solid(ledsGS,9,CRGB::Black);
    ledsLOC[i]=PAL[i]; ledsGS[8-i]=PAL[8-i]; FastLED.show();
    Serial.print(F("LOC:IDX=")); Serial.println(i);
    Serial.print(F("GS:IDX="));  Serial.println(8-i);
    delay(90);
  }
  clrAll();
  logOK("ILS Simulation","Sweep LOC+GS OK");
}

void testDM13A() {
  Serial.println(F("\n>> TEST 5 : DM13A SPI hardware + OE PWM"));
  Serial.println(F("   EN=D5/G5  LAT=D49  SS=D53/G53  SCK=D52/G52  MOSI=D51/G51"));

  dm13a(0xFFFFFFFF);
  Serial.println(F("DM13A:MASK=0xFFFFFFFF")); delay(400);

  for(int c=0;c<4;c++){
    uint32_t p=(c%2)?0x55555555:0xAAAAAAAA;
    dm13a(p); char b[12]; sprintf(b,"0x%08lX",p);
    Serial.print(F("DM13A:MASK=")); Serial.println(b); delay(250);
  }

  Serial.println(F("   Sweep 0->31"));
  for(int b=0;b<32;b++){
    uint32_t m=(1UL<<b); dm13a(m);
    char buf[12]; sprintf(buf,"0x%08lX",m);
    Serial.print(F("DM13A:MASK=")); Serial.println(buf); delay(35);
  }
  dm13a(0xFFFFFFFF);

  // Test explicite slave/master séparés
  Serial.println(F("   Test SLAVE seul (bits 16-31) : 0xFFFF0000"));
  dm13a(0xFFFF0000);
  Serial.println(F("DM13A:MASK=0xFFFF0000"));
  delay(600);
  Serial.println(F("   Test MASTER seul (bits 0-15) : 0x0000FFFF"));
  dm13a(0x0000FFFF);
  Serial.println(F("DM13A:MASK=0x0000FFFF"));
  delay(600);
  dm13a(0xFFFFFFFF);
  Serial.println(F("DM13A:MASK=0xFFFFFFFF"));
  delay(400);

  Serial.println(F("   Fade OE D5 : 0->255->0"));
  for(int v=0;v<=255;v+=3){
    setBL(v); Serial.print(F("OE:PCT=")); Serial.println(map(v,0,255,0,100)); delay(6);
  }
  for(int v=255;v>=0;v-=3){
    setBL(v); Serial.print(F("OE:PCT=")); Serial.println(map(v,0,255,0,100)); delay(6);
  }
  // Fin test : canaux éteints mais OE reste LOW (actif)
  // setBL(0) retiré — il mettait OE HIGH et désactivait le slave définitivement
  dm13a(0x00000000);
  // OE LOW est remis automatiquement par dm13a() → slave et master restent prêts
  Serial.println(F("OE:PCT=100")); Serial.println(F("DM13A:MASK=0x00000000"));
  logW("DM13A SPI HW + OE D5 + LAT D49","Verif visuelle sweep + fade ?");
}

void testSound() {
  Serial.println(F("\n>> TEST 6 : NE555 Son D43/GPIO43"));
  bip(3, 200);
  logW("NE555 D43/GPIO43","3 bips emis -- verif sonore ?");
}

void testLED_Caution() {
  Serial.println(F("\n>> TEST 7 : LED Master Caution D39/GPIO39"));
  for(int i=0;i<5;i++){
    digitalWrite(LED_MASTER_CAUTION,HIGH);
    Serial.println(F("GPIO:LED_CAUTION=1")); delay(200);
    digitalWrite(LED_MASTER_CAUTION,LOW);
    Serial.println(F("GPIO:LED_CAUTION=0")); delay(200);
  }
  logW("LED Master Caution D39/GPIO39","5 clignotements -- verif visuelle ?");
}

void testLED_Alarm() {
  Serial.println(F("\n>> TEST 8 : LED Master Alarm D37/GPIO37"));
  for(int i=0;i<5;i++){
    digitalWrite(LED_MASTER_ALARM,HIGH);
    Serial.println(F("GPIO:LED_ALARM=1")); delay(200);
    digitalWrite(LED_MASTER_ALARM,LOW);
    Serial.println(F("GPIO:LED_ALARM=0")); delay(200);
  }
  logW("LED Master Alarm D37/GPIO37","5 clignotements -- verif visuelle ?");
}

void testBTN_Caution() {
  Serial.println(F("\n>> TEST 9 : BTN Master Caution D38/GPIO38"));
  int s=digitalRead(BTN_MASTER_CAUTION);
  Serial.print(F("   BTN_MASTER_CAUTION D38 = "));
  Serial.println(s==HIGH?F("HIGH (repos)"):F("LOW (actif)"));
  Serial.print(F("GPIO:BTN_CAUTION=")); Serial.println(s);
  if(s==HIGH) logOK("BTN Master Caution D38/GPIO38","HIGH au repos");
  else        logW("BTN Master Caution D38/GPIO38","LOW -- verifier");
}

void testBTN_Alarm() {
  Serial.println(F("\n>> TEST 10 : BTN Master Alarm D36/GPIO36"));
  int s=digitalRead(BTN_MASTER_ALARM);
  Serial.print(F("   BTN_MASTER_ALARM D36 = "));
  Serial.println(s==HIGH?F("HIGH (repos)"):F("LOW (actif)"));
  Serial.print(F("GPIO:ALARM_SW=")); Serial.println(s);
  if(s==HIGH) logOK("BTN Master Alarm D36/GPIO36","HIGH au repos");
  else        logW("BTN Master Alarm D36/GPIO36","LOW -- verifier");
}

void testEncoders() {
  Serial.println(F("\n>> TEST 11 : Encodeurs FMS et CRS/BARO"));
  Serial.println(F("   FMS : A1=D16/G16  A2=D17/G17  B1=D20/G20  B2=D21/G21  SW=D8/G8"));
  Serial.println(F("   CRS : A1=D24/G24  A2=D25/G25  B1=D22/G22  B2=D23/G23  SW=D11/G11"));

  int fA1=digitalRead(ENC_FMS_A1), fA2=digitalRead(ENC_FMS_A2);
  int fB1=digitalRead(ENC_FMS_B1), fB2=digitalRead(ENC_FMS_B2);
  int fS =digitalRead(ENC_FMS_SW);
  int cA1=digitalRead(ENC_CRS_A1), cA2=digitalRead(ENC_CRS_A2);
  int cB1=digitalRead(ENC_CRS_B1), cB2=digitalRead(ENC_CRS_B2);
  int cS =digitalRead(ENC_CRS_SW);

  Serial.print(F("   FMS A1=")); Serial.print(fA1);
  Serial.print(F(" A2="));       Serial.print(fA2);
  Serial.print(F(" B1="));       Serial.print(fB1);
  Serial.print(F(" B2="));       Serial.print(fB2);
  Serial.print(F(" SW="));       Serial.println(fS);
  Serial.print(F("   CRS A1=")); Serial.print(cA1);
  Serial.print(F(" A2="));       Serial.print(cA2);
  Serial.print(F(" B1="));       Serial.print(cB1);
  Serial.print(F(" B2="));       Serial.print(cB2);
  Serial.print(F(" SW="));       Serial.println(cS);

  Serial.print(F("GPIO:FMS_A=")); Serial.println(fA1);
  Serial.print(F("GPIO:FMS_B=")); Serial.println(fB1);
  Serial.print(F("GPIO:CRS_A=")); Serial.println(cA1);
  Serial.print(F("GPIO:CRS_B=")); Serial.println(cB1);

  bool fOK=(fA1==HIGH&&fA2==HIGH&&fB1==HIGH&&fB2==HIGH&&fS==HIGH);
  bool cOK=(cA1==HIGH&&cA2==HIGH&&cB1==HIGH&&cB2==HIGH&&cS==HIGH);
  if(fOK) logOK("Encodeur FMS D16/17/20/21/8","Pull-up OK");
  else  { logE("FMS etat LOW"); logF("Encodeur FMS","Pull-up erreur"); }
  if(cOK) logOK("Encodeur CRS D24/25/22/23/11","Pull-up OK");
  else  { logE("CRS etat LOW"); logF("Encodeur CRS","Pull-up erreur"); }
}

void testButtons() {
  Serial.println(F("\n>> TEST 12 : Boutons G1000"));
  Serial.println(F("   FPL=D32  PROC=D35  CLR=D31  MENU=D34  ENT=D30  DIR=D33"));

  struct B { int p; const char* n; };
  B btns[]={
    {BTN_FPL,   "FPL    D32/G31"},
    {BTN_PROC,  "PROC   D35/G32"},
    {BTN_CLR,   "CLR    D31/G30"},
    {BTN_MENU,  "MENU   D34/G35"},
    {BTN_ENT,   "ENT    D30/G34"},
    {BTN_DIRECT,"DIRECT D33/G33"},
  };
  bool ok=true;
  for(auto& b:btns){
    int s=digitalRead(b.p);
    Serial.print(F("   ")); Serial.print(b.n);
    Serial.println(s==HIGH?F(" OK(H)"):F(" ERR(L)"));
    if(s!=HIGH) ok=false;
  }
  if(ok) logOK("Boutons G1000","Tous HIGH au repos");
  else   logF("Boutons","Au moins 1 LOW");
}

void testJoystick() {
  Serial.println(F("\n>> TEST 13 : Joystick RKJXT1F42001"));
  Serial.println(F("   UP=D26/G26  LEFT=D27/G27  DOWN=D28/G28  RIGHT=D29/G29"));
  Serial.println(F("   PUSH=D15/G15  ROT_L=D18/G18  ROT_R=D19/G19"));

  int jU =digitalRead(JOY_UP);
  int jL =digitalRead(JOY_LEFT);
  int jD =digitalRead(JOY_DOWN);
  int jR =digitalRead(JOY_RIGHT);
  int jP =digitalRead(JOY_PUSH);
  int rL =digitalRead(ROTATE_LEFT);
  int rR =digitalRead(ROTATE_RIGHT);

  Serial.print(F("   UP="));    Serial.print(jU);
  Serial.print(F(" LEFT="));    Serial.print(jL);
  Serial.print(F(" DOWN="));    Serial.print(jD);
  Serial.print(F(" RIGHT="));   Serial.print(jR);
  Serial.print(F(" PUSH="));    Serial.print(jP);
  Serial.print(F(" ROT_L="));   Serial.print(rL);
  Serial.print(F(" ROT_R="));   Serial.println(rR);
  Serial.print(F("GPIO:PUSH=")); Serial.println(jP);

  bool ok=(jU==HIGH&&jL==HIGH&&jD==HIGH&&jR==HIGH&&jP==HIGH&&rL==HIGH&&rR==HIGH);
  if(ok) logOK("Joystick D26/27/28/29/15/18/19","Tous HIGH au repos");
  else   logF("Joystick","Au moins 1 LOW");
}

// ════════════════════════════════════════════════════════════
// RAPPORT
// ════════════════════════════════════════════════════════════
void rapport() {
  hdr("RAPPORT FINAL");
  int ok=0,w=0,f=0;
  for(int i=0;i<rCnt;i++){
    if(!results[i].ok)    { f++; Serial.print(F("[FAIL] ")); }
    else if(results[i].w) { w++; Serial.print(F("[WARN] ")); }
    else                  { ok++;Serial.print(F("[OK]   ")); }
    Serial.print(results[i].n); Serial.print(F(": ")); Serial.println(results[i].m);
  }
  sep();
  Serial.print(F("  OK:")); Serial.print(ok);
  Serial.print(F("  WARN:")); Serial.print(w);
  Serial.print(F("  FAIL:")); Serial.println(f);
  sep();
  if(!f&&!w)  Serial.println(F("  TOUS PASSES"));
  else if(!f) Serial.println(F("  PASSE AVEC AVERTISSEMENTS"));
  else        Serial.println(F("  ECHECS DETECTES"));
  sep();
}

// ════════════════════════════════════════════════════════════
// PHASE 2 : INTERACTIF + COMMANDES WEB
// ════════════════════════════════════════════════════════════
void phaseInteractive() {
  hdr("PHASE 2 : INTERACTIF + COMMANDES WEB");
  Serial.println(F("  LOC:IDX=  GS:IDX=  BL:OE=  BL:DM13A="));
  Serial.println(F("  LED:CAUTION=  LED:ALARM=  SND:BIP=  |  n=fin"));
  sep();

  // DM13A : s'assurer que OE est actif au démarrage phase interactive
  // (peut avoir été éteint par testDM13A)
  // La réactivation est faite avant phaseInteractive() dans setup()

  // États précédents
  int lFA1=HIGH,lFA2=HIGH,lFB1=HIGH,lFB2=HIGH,lFS=HIGH;
  int lCA1=HIGH,lCA2=HIGH,lCB1=HIGH,lCB2=HIGH,lCS=HIGH;
  int lBtns[6]={HIGH,HIGH,HIGH,HIGH,HIGH,HIGH};
  int bCnt[6]={};
  int lJU=HIGH,lJL=HIGH,lJD=HIGH,lJR=HIGH,lJP=HIGH,lRL=HIGH,lRR=HIGH;
  int lBtnCau=HIGH, lAlarm=HIGH;
  int fC=0, cC=0, rotC=0;
  running=true;

  while(running){
    // ── Lecture série ─────────────────────────────────────────
    while(Serial.available()){
      char c=Serial.read();
      if(c=='\n'||c=='\r'){
        if(sBuf.length()){
          if(sBuf=="n"||sBuf=="N") running=false;
          else parseCmd(sBuf);
          sBuf="";
        }
      } else { sBuf+=c; if(sBuf.length()>64) sBuf=""; }
    }
    if(!running) break;

    // ── Encodeur FMS ──────────────────────────────────────────
    int fA1=digitalRead(ENC_FMS_A1), fA2=digitalRead(ENC_FMS_A2);
    int fB1=digitalRead(ENC_FMS_B1), fB2=digitalRead(ENC_FMS_B2);
    if(fA1!=lFA1||fA2!=lFA2||fB1!=lFB1||fB2!=lFB2){
      fC++;
      const char* dir=(fA1==LOW&&fB1==HIGH)?"CW":(fB1==LOW&&fA1==HIGH)?"CCW":"?";
      Serial.print(F("[INPUT] ENC FMS  A1=")); Serial.print(fA1);
      Serial.print(F(" A2=")); Serial.print(fA2);
      Serial.print(F(" B1=")); Serial.print(fB1);
      Serial.print(F(" B2=")); Serial.print(fB2);
      Serial.print(F(" dir=")); Serial.print(dir);
      Serial.print(F(" cnt=")); Serial.println(fC);
      Serial.print(F("GPIO:FMS_A=")); Serial.println(fA1);
      Serial.print(F("GPIO:FMS_B=")); Serial.println(fB1);
      lFA1=fA1; lFA2=fA2; lFB1=fB1; lFB2=fB2;
    }
    int fS=digitalRead(ENC_FMS_SW);
    if(fS!=lFS){
      Serial.println(fS==LOW?F("[INPUT] ENC FMS SW PRESSE"):F("[INPUT] ENC FMS SW relache"));
      lFS=fS;
    }

    // ── Encodeur CRS ──────────────────────────────────────────
    int cA1=digitalRead(ENC_CRS_A1), cA2=digitalRead(ENC_CRS_A2);
    int cB1=digitalRead(ENC_CRS_B1), cB2=digitalRead(ENC_CRS_B2);
    if(cA1!=lCA1||cA2!=lCA2||cB1!=lCB1||cB2!=lCB2){
      cC++;
      const char* dir=(cA1==LOW&&cB1==HIGH)?"CW":(cB1==LOW&&cA1==HIGH)?"CCW":"?";
      Serial.print(F("[INPUT] ENC CRS  A1=")); Serial.print(cA1);
      Serial.print(F(" A2=")); Serial.print(cA2);
      Serial.print(F(" B1=")); Serial.print(cB1);
      Serial.print(F(" B2=")); Serial.print(cB2);
      Serial.print(F(" dir=")); Serial.print(dir);
      Serial.print(F(" cnt=")); Serial.println(cC);
      Serial.print(F("GPIO:CRS_A=")); Serial.println(cA1);
      Serial.print(F("GPIO:CRS_B=")); Serial.println(cB1);
      lCA1=cA1; lCA2=cA2; lCB1=cB1; lCB2=cB2;
    }
    int cS=digitalRead(ENC_CRS_SW);
    if(cS!=lCS){
      Serial.println(cS==LOW?F("[INPUT] ENC CRS SW PRESSE"):F("[INPUT] ENC CRS SW relache"));
      lCS=cS;
    }

    // ── BTN Master Caution ────────────────────────────────────
    int btnCau=digitalRead(BTN_MASTER_CAUTION);
    if(btnCau!=lBtnCau){
      Serial.println(btnCau==LOW?F("[INPUT] BTN MASTER_CAUTION PRESSE"):F("[INPUT] BTN MASTER_CAUTION relache"));
      Serial.print(F("GPIO:BTN_CAUTION=")); Serial.println(btnCau);
      // Pilote LED Caution
      digitalWrite(LED_MASTER_CAUTION, btnCau==LOW?HIGH:LOW);
      Serial.print(F("GPIO:LED_CAUTION=")); Serial.println(btnCau==LOW?1:0);
      lBtnCau=btnCau;
    }

    // ── BTN Master Alarm ──────────────────────────────────────
    int alarm=digitalRead(BTN_MASTER_ALARM);
    if(alarm!=lAlarm){
      Serial.println(alarm==LOW?F("[INPUT] BTN MASTER_ALARM ACTIVE"):F("[INPUT] BTN MASTER_ALARM relache"));
      Serial.print(F("GPIO:ALARM_SW=")); Serial.println(alarm);
      // LED Alarm suit automatiquement + bip sonore
      digitalWrite(LED_MASTER_ALARM, alarm==LOW?HIGH:LOW);
      Serial.print(F("GPIO:LED_ALARM=")); Serial.println(alarm==LOW?1:0);
      if(alarm==LOW) bip(2,100);
      lAlarm=alarm;
    }

    // ── Boutons G1000 ─────────────────────────────────────────
    const char* bN[]={"FPL","PROC","CLR","MENU","ENT","DIRECT"};
    int bP[]={BTN_FPL,BTN_PROC,BTN_CLR,BTN_MENU,BTN_ENT,BTN_DIRECT};
    for(int i=0;i<6;i++){
      int s=digitalRead(bP[i]);
      if(s!=lBtns[i]){
        if(s==LOW){
          bCnt[i]++;
          Serial.print(F("[INPUT] BTN ")); Serial.print(bN[i]);
          Serial.print(F(" PRESSE #")); Serial.println(bCnt[i]);
          fill_solid(ledsLOC,9,CRGB::Black); ledsLOC[4]=CRGB::Green; FastLED.show();
        } else {
          Serial.print(F("[INPUT] BTN ")); Serial.print(bN[i]); Serial.println(F(" relache"));
          fill_solid(ledsLOC,9,CRGB::Black); FastLED.show();
        }
        lBtns[i]=s;
      }
    }

    // ── Joystick directions ───────────────────────────────────
    struct JDir { int pin; int* last; const char* name; int ledIdx; };
    JDir dirs[]={
      {JOY_UP,    &lJU, "UP",    0},
      {JOY_LEFT,  &lJL, "LEFT",  3},
      {JOY_DOWN,  &lJD, "DOWN",  8},
      {JOY_RIGHT, &lJR, "RIGHT", 5},
    };
    for(auto& d:dirs){
      int s=digitalRead(d.pin);
      if(s!=*(d.last)){
        Serial.print(F("[INPUT] JOY ")); Serial.print(d.name);
        Serial.println(s==LOW?F(" actif"):F(" relache"));
        fill_solid(ledsGS,9,CRGB::Black);
        if(s==LOW) ledsGS[d.ledIdx]=PAL[d.ledIdx];
        FastLED.show();
        *(d.last)=s;
      }
    }

    // ── Push joystick ─────────────────────────────────────────
    int jP=digitalRead(JOY_PUSH);
    if(jP!=lJP){
      Serial.println(jP==LOW?F("[INPUT] JOY PUSH actif"):F("[INPUT] JOY PUSH relache"));
      Serial.print(F("GPIO:PUSH=")); Serial.println(jP);
      fill_solid(ledsGS,9,CRGB::Black); if(jP==LOW) ledsGS[4]=CRGB::White; FastLED.show();
      lJP=jP;
    }

    // ── Rotation joystick ─────────────────────────────────────
    int rL=digitalRead(ROTATE_LEFT), rR=digitalRead(ROTATE_RIGHT);
    if(rL!=lRL||rR!=lRR){
      rotC++;
      const char* dir=(rL==LOW&&rR==HIGH)?"LEFT":(rR==LOW&&rL==HIGH)?"RIGHT":"?";
      Serial.print(F("[INPUT] ROTATE  L=")); Serial.print(rL);
      Serial.print(F(" R=")); Serial.print(rR);
      Serial.print(F(" dir=")); Serial.print(dir);
      Serial.print(F(" cnt=")); Serial.println(rotC);
      lRL=rL; lRR=rR;
    }

    delay(5);
  }

  clrAll();
  digitalWrite(LED_MASTER_CAUTION, LOW);
  digitalWrite(LED_MASTER_ALARM,   LOW);
  digitalWrite(NE555_SOUND,        LOW);
}

// ════════════════════════════════════════════════════════════
// SETUP
// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  while(!Serial){}

  sep();
  Serial.println(F("  Knov V3.3 REV 3.41 — Test v1.0.11"));
  Serial.println(F("  LOC=D7/G7  GS=D6/G6  EN=D5/G5  LAT=D49  SS=D53  SCK=D52  MOSI=D51"));
  Serial.println(F("  FMS=D16/17/20/21/SW:D8   CRS=D24/25/22/23/SW:D11"));
  Serial.println(F("  JOY=UP:D26 L:D27 D:D28 R:D29 PSH:D15 ROT:D18/19"));
  Serial.println(F("  BTN=D32/35/31/34/30/33  SND=D43"));
  Serial.println(F("  BTN_CAU=D38 BTN_ALM=D36 LED_CAU=D39 LED_ALM=D37"));
  sep();

  // Sorties
  pinMode(LED_MASTER_CAUTION, OUTPUT); digitalWrite(LED_MASTER_CAUTION, LOW);
  pinMode(LED_MASTER_ALARM,   OUTPUT); digitalWrite(LED_MASTER_ALARM,   LOW);
  pinMode(NE555_SOUND,        OUTPUT); digitalWrite(NE555_SOUND,        LOW);

  // Entrées INPUT_PULLUP
  int inp[]={
    ENC_FMS_A1,ENC_FMS_A2,ENC_FMS_B1,ENC_FMS_B2,ENC_FMS_SW,
    ENC_CRS_A1,ENC_CRS_A2,ENC_CRS_B1,ENC_CRS_B2,ENC_CRS_SW,
    JOY_UP,JOY_LEFT,JOY_DOWN,JOY_RIGHT,JOY_PUSH,ROTATE_LEFT,ROTATE_RIGHT,
    BTN_FPL,BTN_PROC,BTN_CLR,BTN_MENU,BTN_ENT,BTN_DIRECT,
    BTN_MASTER_CAUTION,BTN_MASTER_ALARM
  };
  for(int p:inp) pinMode(p, INPUT_PULLUP);

  // FastLED
  FastLED.addLeds<WS2812B,PIN_LOC,GRB>(ledsLOC,9);
  FastLED.addLeds<WS2812B,PIN_GS, GRB>(ledsGS, 9);
  FastLED.setBrightness(BRIGHT);
  clrAll();

  // DM13A
  dm13aInit();
  delay(200);

  // PHASE 1 : TESTS AUTOMATIQUES
  hdr("PHASE 1 : TESTS AUTOMATIQUES");
  testPower();       delay(80);
  testLOC();         delay(80);
  testGS();          delay(80);
  testILS();         delay(80);
  testDM13A();       delay(80);
  testSound();       delay(80);
  testLED_Caution(); delay(80);
  testLED_Alarm();   delay(80);
  testBTN_Caution(); delay(80);
  testBTN_Alarm();   delay(80);
  testEncoders();    delay(80);
  testButtons();     delay(80);
  testJoystick();    delay(80);

  // Boot check final
  Serial.println(F("\n>> BOOT CHECK : LEDs blanches + Caution + Alarm + 1 bip"));
  fill_solid(ledsLOC,9,CRGB(60,60,60));
  fill_solid(ledsGS, 9,CRGB(60,60,60));
  FastLED.show();
  digitalWrite(LED_MASTER_CAUTION,HIGH);
  digitalWrite(LED_MASTER_ALARM,  HIGH);
  Serial.println(F("LOC:IDX=4")); Serial.println(F("GS:IDX=4"));
  Serial.println(F("GPIO:LED_CAUTION=1")); Serial.println(F("GPIO:LED_ALARM=1"));
  bip(1, 300);
  delay(1500);
  clrAll();
  digitalWrite(LED_MASTER_CAUTION,LOW);
  digitalWrite(LED_MASTER_ALARM,  LOW);
  Serial.println(F("GPIO:LED_CAUTION=0")); Serial.println(F("GPIO:LED_ALARM=0"));

  // Réinitialisation forcée DM13A après les tests (fade OE peut désynchroniser le slave)
  // Étape 1 : s'assurer que OE est actif (LOW) avant d'envoyer des données
  digitalWrite(DM13A_EN, LOW);
  delayMicroseconds(100);
  // Étape 2 : envoyer 3 fois le masque pour garantir la synchronisation du slave
  dm13a(0x00000000); delay(5);   // reset
  dm13a(0xFFFFFFFF); delay(5);   // tous ON — slave doit répondre ici
  dm13a(0xFFFFFFFF);             // confirmation
  Serial.println(F("DM13A:MASK=0xFFFFFFFF"));
  Serial.println(F("OE:PCT=100"));

  rapport();
  delay(400);
  phaseInteractive();
  rapport();
  Serial.println(F("  Fin — commandes web actives."));
}

// ════════════════════════════════════════════════════════════
// LOOP — commandes web + surveillance alarme permanentes
// ════════════════════════════════════════════════════════════
void loop() {
  while(Serial.available()){
    char c=Serial.read();
    if(c=='\n'||c=='\r'){
      if(sBuf.length()){ parseCmd(sBuf); sBuf=""; }
    } else { sBuf+=c; if(sBuf.length()>64) sBuf=""; }
  }
  // Surveillance BTN_MASTER_ALARM en permanence
  static int lastAlarm=HIGH;
  int alarm=digitalRead(BTN_MASTER_ALARM);
  if(alarm!=lastAlarm){
    digitalWrite(LED_MASTER_ALARM, alarm==LOW?HIGH:LOW);
    Serial.print(F("GPIO:ALARM_SW=")); Serial.println(alarm);
    Serial.print(F("GPIO:LED_ALARM=")); Serial.println(alarm==LOW?1:0);
    if(alarm==LOW) bip(2,100);
    lastAlarm=alarm;
  }
  // Surveillance BTN_MASTER_CAUTION en permanence
  static int lastCau=HIGH;
  int cau=digitalRead(BTN_MASTER_CAUTION);
  if(cau!=lastCau){
    digitalWrite(LED_MASTER_CAUTION, cau==LOW?HIGH:LOW);
    Serial.print(F("GPIO:BTN_CAUTION=")); Serial.println(cau);
    Serial.print(F("GPIO:LED_CAUTION=")); Serial.println(cau==LOW?1:0);
    lastCau=cau;
  }
}
