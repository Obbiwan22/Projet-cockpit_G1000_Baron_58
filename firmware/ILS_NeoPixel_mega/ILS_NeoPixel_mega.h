#pragma once
#include "Arduino.h"
#include <FastLED.h>

#define ILS_MAX_LEDS    9
#define ILS_BRIGHTNESS  80
#define ILS_PIN_LOC      7    // GPIO6 → D6  LOC DIN
#define ILS_PIN_GS       6    // GPIO7 → D7  GS  DIN

// DM13A SPI hardware Mega 2560
// EN# master + slave reliés sur GPIO5/D5 (PWM actif bas)
#define DM13A_EN    5    // GPIO5  → D5  PWM
#define DM13A_LAT  49    // D49    libre
#define DM13A_SS   53    // GPIO53 → D53 SS
// SCK=D52 / MOSI=D51 / MISO=D50 gérés par bitbang

// Note : plus de timeout d'extinction. L'extinction est désormais
// entièrement pilotée par le gate ILS_ACTIVE (MSG_ILS_ACTIVE),
// immédiat et explicite — pas de dépendance à un délai sans donnée.

// Message de gâchette : état ILS_ACTIVE (0/1), source de vérité
// unique pour l'affichage/extinction. LOC et GS (messageID 0/1)
// sont TOUJOURS transmis par MobiFlight (aucune précondition) et
// TOUJOURS mémorisés par le firmware ; seul l'AFFICHAGE dépend de
// ce gate, réévalué immédiatement à chaque bascule.
#define MSG_ILS_ACTIVE  3

class ILS_NeoPixel_mega
{
public:
    ILS_NeoPixel_mega(uint8_t Pin1, uint8_t Pin2);
    void begin();
    void attach(uint16_t Pin3, char *init);
    void detach();
    void set(int16_t messageID, char *setPoint);
    void update();

private:
    bool      _initialised;
    uint8_t   _pin1;
    uint8_t   _pin2;
    uint8_t   _pin3;
    uint32_t  _lastUpdateLOC;
    uint32_t  _lastUpdateGS;
    bool      _ilsActive;    // gate d'affichage — n'affecte QUE l'affichage, jamais la réception
    int       _lastLocIndex; // dernier index LOC reçu, mémorisé même si non affiché
    int       _lastGsIndex;  // dernier index GS  reçu, mémorisé même si non affiché
    void      showIndex(CRGB* strip, int idx);
    void      dm13a_byte(uint8_t val);
    void      dm13a(uint32_t mask);
    void      setBL(uint8_t val);
};
