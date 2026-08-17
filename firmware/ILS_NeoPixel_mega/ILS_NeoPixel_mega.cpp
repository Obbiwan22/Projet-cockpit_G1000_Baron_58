// ============================================================
//  ILS_NeoPixel_mega.cpp
//
//  Barre LED ILS pour Microsoft Flight Simulator 2024
//  Custom Device MobiFlight v11 – Community Firmware
//
//  Description :
//    Pilote deux rubans WS2812B de 9 LEDs chacun via FastLED :
//      - Barre LOC (Localizer)   : déviation horizontale
//      - Barre GS  (Glideslope) : déviation verticale
//
//    Chaque barre affiche une seule LED allumée dont la couleur
//    indique la déviation :
//      LED 0/8 : rouge vif    (déviation maximale)
//      LED 1/7 : orange foncé
//      LED 2/6 : orange
//      LED 3/5 : jaune
//      LED 4   : vert vif     (sur l'axe / sur le plan)
//
//    Extinction pilotée par ILS_ACTIVE (messageID MSG_ILS_ACTIVE) :
//    LOC/GS sont toujours reçus et mémorisés (aucune précondition
//    côté MobiFlight sur ces deux-là), seul l'AFFICHAGE dépend du
//    gate ILS_ACTIVE, réaffiché immédiatement à chaque bascule pour
//    éviter toute dépendance à un nouveau message LOC/GS après coup.
//    ILS_TIMEOUT_MS reste un pur filet de sécurité (perte de liaison
//    série), il ne doit quasiment jamais se déclencher.
//
//  Matériel :
//    - Arduino Mega 2560 Pro Mini
//    - Ruban WS2812B LOC : pin D6
//    - Ruban WS2812B GS  : pin D7
//    - Alimentation 5V USB (luminosité limitée à 80/255)
//
//  SimVars MSFS 2024 :
//    - LOC : NAV CDI:1              (plage -127 à +127)
//      Transform : Floor(($ + 127) / 254 * 8 + 0.5)
//    - GS  : NAV GLIDESLOPE ERROR:1 (plage -1.0 à +1.0, inversée)
//      Transform : Floor((($ * -1) + 1) / 2 * 8 + 0.5)
//    - Précondition : NAV HAS GLIDE SLOPE:1 (bool)
//
//  Auteur  : francois.colas22
//  Projet  : Cockpit G1000 – Beechcraft Baron 58
//  Licence : MIT
//  Version : 1.0.2
// ============================================================
 
#include "ILS_NeoPixel_mega.h"
#include "allocateMem.h"
#include "commandmessenger.h"
 
// ── Tableaux LEDs et flag statiques globaux ──────────────────
static CRGB ledsLOC[ILS_MAX_LEDS];
static CRGB ledsGS[ILS_MAX_LEDS];
static bool _fastledReady = false;  // FastLED.addLeds ne s'exécute qu'une seule fois
 
// ── Palette de couleurs par index LED ────────────────────────
// Index 0 = déviation max gauche/haut (rouge)
// Index 4 = centré sur l'axe          (vert)
// Index 8 = déviation max droite/bas  (rouge)
static const CRGB PALETTE[ILS_MAX_LEDS] = {
    CRGB(255,   0,   0),  // 0 – rouge vif      (déviation maximale)
    CRGB(255,  60,   0),  // 1 – orange foncé
    CRGB(255, 120,   0),  // 2 – orange
    CRGB(220, 200,   0),  // 3 – jaune
    CRGB(  0, 255,   0),  // 4 – vert vif        (centré sur l'axe ILS)
    CRGB(220, 200,   0),  // 5 – jaune
    CRGB(255, 120,   0),  // 6 – orange
    CRGB(255,  60,   0),  // 7 – orange foncé
    CRGB(255,   0,   0),  // 8 – rouge vif      (déviation maximale)
};
 
// ── DM13A SPI bitbang ────────────────────────────────────────
void ILS_NeoPixel_mega::dm13a_byte(uint8_t val)
{
    for (int b = 7; b >= 0; b--) {
        digitalWrite(51, (val >> b) & 1);  // MOSI D51
        digitalWrite(52, HIGH);             // SCK  D52
        delayMicroseconds(1);
        digitalWrite(52, LOW);
        delayMicroseconds(1);
    }
}

void ILS_NeoPixel_mega::dm13a(uint32_t mask)
{
    uint16_t hi = (uint16_t)(mask >> 16);
    uint16_t lo = (uint16_t)(mask & 0xFFFF);
    digitalWrite(DM13A_EN, HIGH);
    delayMicroseconds(2);
    digitalWrite(DM13A_SS, LOW);
    dm13a_byte((hi >> 8) & 0xFF);
    dm13a_byte( hi       & 0xFF);
    dm13a_byte((lo >> 8) & 0xFF);
    dm13a_byte( lo       & 0xFF);
    digitalWrite(DM13A_SS, HIGH);
    digitalWrite(DM13A_LAT, HIGH);
    delayMicroseconds(5);
    digitalWrite(DM13A_LAT, LOW);
    digitalWrite(DM13A_EN, LOW);
}

void ILS_NeoPixel_mega::setBL(uint8_t val)
{
    // val=255 → OE LOW  → max luminosité
    // val=0   → OE HIGH → éteint
    analogWrite(DM13A_EN, 255 - val);
}

// ─────────────────────────────────────────────────────────────
// CONSTRUCTEUR
// Reçoit les deux pins DIN des rubans WS2812B.
// Note : FastLED nécessite des pins connus à la compilation
// (templates C++), donc ILS_PIN_LOC et ILS_PIN_GS sont des
// constantes définies dans ILS_NeoPixel_mega.h (#define).
// ─────────────────────────────────────────────────────────────
ILS_NeoPixel_mega::ILS_NeoPixel_mega(uint8_t Pin1, uint8_t Pin2)
{
    _pin1          = Pin1;   // DIN LOC (D6)
    _pin2          = Pin2;   // DIN GS  (D7)
    _initialised   = false;
    _lastUpdateLOC = 0;
    _lastUpdateGS  = 0;
    _ilsActive     = false;
    _lastLocIndex  = -1;   // -1 = pas encore de donnée, hors plage → blanc
    _lastGsIndex   = -1;
}
 
// ─────────────────────────────────────────────────────────────
// BEGIN – Initialise FastLED et éteint toutes les LEDs
// Appelé une seule fois lors du premier attach().
// ─────────────────────────────────────────────────────────────
void ILS_NeoPixel_mega::begin()
{
    if (!_fastledReady) {
        // FastLED.addLeds appelé UNE SEULE FOIS pour toutes les instances
        FastLED.addLeds<WS2812B, ILS_PIN_LOC, GRB>(ledsLOC, ILS_MAX_LEDS);
        FastLED.addLeds<WS2812B, ILS_PIN_GS,  GRB>(ledsGS,  ILS_MAX_LEDS);
        FastLED.setBrightness(ILS_BRIGHTNESS);

        // TEST VISUEL : toutes LEDs blanches 2s au démarrage
        fill_solid(ledsLOC, ILS_MAX_LEDS, CRGB::White);
        fill_solid(ledsGS,  ILS_MAX_LEDS, CRGB::White);
        FastLED.show();
        delay(2000);
        fill_solid(ledsLOC, ILS_MAX_LEDS, CRGB::Black);
        fill_solid(ledsGS,  ILS_MAX_LEDS, CRGB::Black);
        FastLED.show();

        // DM13A init
        pinMode(51,        OUTPUT); digitalWrite(51,        LOW);
        pinMode(52,        OUTPUT); digitalWrite(52,        LOW);
        pinMode(DM13A_SS,  OUTPUT); digitalWrite(DM13A_SS,  HIGH);
        pinMode(DM13A_LAT, OUTPUT); digitalWrite(DM13A_LAT, LOW);
        pinMode(DM13A_EN,  OUTPUT); digitalWrite(DM13A_EN,  LOW);
        dm13a(0xFFFFFFFF);

        _fastledReady = true;
    }
 
    // Initialiser les timestamps à maintenant
    _lastUpdateLOC = millis();
    _lastUpdateGS  = millis();

    _initialised = true;
}
 
// ─────────────────────────────────────────────────────────────
// ATTACH – Appelé par MobiFlight lors du chargement de la config
// Pin3 est le 3ème paramètre de config (non utilisé ici).
// init contient la chaîne de configuration EEPROM.
// ─────────────────────────────────────────────────────────────
void ILS_NeoPixel_mega::attach(uint16_t Pin3, char *init)
{
    _pin3 = Pin3;
    // Initialiser FastLED au premier attach seulement
    if (!_initialised) begin();
}
 
// ─────────────────────────────────────────────────────────────
// DETACH – Appelé quand le device est retiré de la config
// Éteint toutes les LEDs proprement.
// ─────────────────────────────────────────────────────────────
void ILS_NeoPixel_mega::detach()
{
    if (!_initialised) return;
 
    fill_solid(ledsLOC, ILS_MAX_LEDS, CRGB::Black);
    fill_solid(ledsGS,  ILS_MAX_LEDS, CRGB::Black);
    FastLED.show();
    dm13a(0x00000000);
    setBL(0);
 
    _initialised = false;
}
 
// ─────────────────────────────────────────────────────────────
// SHOWINDEX – Allume une seule LED sur un ruban
// Si l'index est hors plage [0-8], éteint toutes les LEDs
// du ruban.
// ─────────────────────────────────────────────────────────────
void ILS_NeoPixel_mega::showIndex(CRGB* strip, int idx)
{
    if (idx < 0 || idx > 8) {
        // Index hors plage → extinction complète du ruban
        fill_solid(strip, ILS_MAX_LEDS, CRGB::Black);
    } else {
        // Allumer uniquement la LED à l'index demandé
        // avec sa couleur de palette, éteindre les autres
        for (int i = 0; i < ILS_MAX_LEDS; i++) {
            strip[i] = (i == idx) ? PALETTE[i] : CRGB::Black;
        }
    }
    FastLED.show();
}
 
// ─────────────────────────────────────────────────────────────
// SET – Appelé par MobiFlight quand une valeur SimVar change
//
// messageID : identifiant du message configuré dans MobiFlight
//   0  → barre LOC (Localizer horizontal)      — toujours mémorisé
//   1  → barre GS  (Glideslope vertical)       — toujours mémorisé
//   3  → MSG_ILS_ACTIVE : gate d'affichage (0/1), réaffiche immédiatement
//        depuis le cache si passage à 1, extinction immédiate si 0
//  -1  → arrêt MobiFlight Connector
//  -2  → entrée en mode Power Saving
//
// setPoint : chaîne contenant la valeur transformée (index 0-8)
//
// Transforms MobiFlight configurées :
//   LOC : Floor(($ + 127) / 254 * 8 + 0.5)
//         NAV CDI:1 plage -127..+127 → index 0..8
//   GS  : Floor((($ * -1) + 1) / 2 * 8 + 0.5)
//         NAV GLIDESLOPE ERROR:1 plage -1..+1 inversée → index 0..8
// ─────────────────────────────────────────────────────────────
void ILS_NeoPixel_mega::set(int16_t messageID, char *setPoint)
{
    if (!_initialised) return;
 
    // Convertir la chaîne en entier
    int32_t data = atoi(setPoint);
 
    switch (messageID) {

    case MSG_ILS_ACTIVE:
        // Gate d'affichage. LOC/GS continuent d'être reçus et
        // mémorisés en permanence (voir case 0/1 ci-dessous) —
        // ce message ne fait que décider si on les affiche.
        _ilsActive = (data != 0);
        if (!_ilsActive) {
            // Extinction immédiate, sans attendre quoi que ce soit.
            fill_solid(ledsLOC, ILS_MAX_LEDS, CRGB::Black);
            fill_solid(ledsGS,  ILS_MAX_LEDS, CRGB::Black);
            FastLED.show();
        } else {
            // Réaffichage IMMÉDIAT à partir du dernier index connu,
            // sans attendre un nouveau message LOC/GS de MobiFlight
            // (qui peut tarder si la valeur n'a pas changé entre
            // temps — comportement "envoi sur changement" observé).
            showIndex(ledsLOC, _lastLocIndex);
            showIndex(ledsGS,  _lastGsIndex);
        }
        _lastUpdateLOC = millis();
        _lastUpdateGS  = millis();
        break;

    case -1:
    case -2:
        // Arrêt MobiFlight ou mode économie d'énergie
        // Éteindre toutes les LEDs des deux barres
        fill_solid(ledsLOC, ILS_MAX_LEDS, CRGB::Black);
        fill_solid(ledsGS,  ILS_MAX_LEDS, CRGB::Black);
        FastLED.show();
        break;
 
    case 0:
        // Barre LOC (Localizer horizontal)
        // Index 0 = max gauche, 4 = centré, 8 = max droite
        // Toujours mémorisé ; affiché seulement si _ilsActive.
        _lastLocIndex  = (int)data;
        _lastUpdateLOC = millis();
        if (_ilsActive) showIndex(ledsLOC, _lastLocIndex);
        break;
 
    case 1:
        // Barre GS (Glideslope vertical)
        // Index 0 = trop haut, 4 = sur le plan, 8 = trop bas
        // Toujours mémorisé ; affiché seulement si _ilsActive.
        _lastGsIndex  = (int)data;
        _lastUpdateGS = millis();
        if (_ilsActive) showIndex(ledsGS, _lastGsIndex);
        break;
 
    case 2:
        // DM13A luminosité rétroéclairage (0-255, 255=max)
        // SimVar    : LIGHT POTENTIOMETER:85
        // Transform : Floor($ / 100 * 255)
        setBL((uint8_t)constrain(data, 0, 255));
        break;

    default:
        // MessageID inconnu → ignorer
        break;
    }
}
 
// ─────────────────────────────────────────────────────────────
// UPDATE – Appelé dans loop() par MobiFlight
//
// L'extinction est désormais ENTIÈREMENT gérée par le gate
// ILS_ACTIVE dans set() (MSG_ILS_ACTIVE), de façon immédiate et
// explicite. Cette fonction ne fait plus d'extinction basée sur
// un timeout : ce mécanisme entrait en conflit avec le gate et
// provoquait des extinctions intempestives dès que MobiFlight ne
// renvoyait pas de nouvelle donnée LOC/GS (valeur inchangée),
// même quand ILS_ACTIVE restait à 1. _lastUpdateLOC/_lastUpdateGS
// sont conservés (mis à jour dans set()) pour un usage diagnostic
// futur éventuel, mais ne déclenchent plus rien ici.
// ─────────────────────────────────────────────────────────────
void ILS_NeoPixel_mega::update()
{
    if (!_initialised) return;
    // Rien à faire : extinction pilotée exclusivement par le gate.
}
