# G1000 Baron 58 — Cockpit Panel (COMMANDE G1000)

Panneau de commande physique pour le G1000 du Beechcraft Baron 58 sous Microsoft Flight Simulator 2024, piloté par un Arduino Mega 2560 Pro Mini via MobiFlight.

![Statut](https://img.shields.io/badge/status-fonctionnel%20%E2%80%94%20valid%C3%A9%20en%20vol-brightgreen)
![Plateforme](https://img.shields.io/badge/plateforme-Arduino%20Mega%202560-blue)
![MobiFlight](https://img.shields.io/badge/MobiFlight-Community%20Firmware-orange)

## En bref

Adepte de Flight Simulator, je voulais accéder facilement aux commandes du G1000 sans construire un home cockpit complet. Ce dépôt contient tout ce qu'il faut pour reproduire un panneau de commande physique complet : encodeurs CRS/BARO et FMS, joystick 4 axes, boutons G1000, Master Alarm/Caution avec voyants et alarme sonore, rétro-éclairage variable, et une barre LED qui reproduit l'indicateur de déviation ILS (Localizer/Glideslope).

Le morceau qui a demandé le plus de travail — la barre ILS — a nécessité plusieurs refontes du firmware avant d'obtenir un comportement fiable en vol. Le détail complet de cette investigation est dans le rapport technique (voir plus bas), pour ceux que ça intéresse.

## Ce que contient la carte

| Fonction | Détail |
|---|---|
| Microcontrôleur | Arduino Mega 2560 Pro Mini (CH340G) |
| Encodeurs | CRS/BARO et FMS, EC11EBB24C03 (bagues inner/outer indépendantes) |
| Joystick | 4 axes (Range/Pan), RKJXT1F42001 |
| Boutons | Direct, Menu, Proc, FPL, CLR, Ent |
| Master Alarm / Caution | Boutons illuminés YTS-D001-285 + buzzer NE555 |
| Rétro-éclairage | 2× DM13A chaînés (20 canaux LED) |
| Barre ILS | 2× rubans WS2812B (LOC + GS), 9 LEDs chacun |
| Alimentation | 7.5V DC externe (recommandé) ou USB |

32 broches GPIO du Mega sont mobilisées — sur les 54 disponibles, il en reste tout juste assez pour respirer.

## Structure du dépôt

```
firmware/       Firmware Arduino (.ino/.h/.cpp) — module ILS + panneau complet
mobiflight/      device.json, board.json — Custom Device MobiFlight
schematics/      Schémas EasyEDA (carte CPU + carte COMMANDE G1000)
docs/            Rapport technique complet (FR + EN), BOM, dépannage
tools/           Programme de test matériel autonome + console de contrôle web
```

## Le morceau intéressant : la logique d'extinction ILS

La vraie difficulté de ce projet n'a jamais été d'afficher la bonne LED — c'est trivial. Le vrai problème a été de l'**éteindre correctement** quand le signal ILS devient invalide. Plusieurs itérations ont été nécessaires avant de comprendre que la cause racine tenait à deux comportements peu documentés de MobiFlight :

- MobiFlight n'envoie une donnée au device **que si la valeur change** — un index de déviation qui reste stable plusieurs secondes en approche stabilisée arrête tout simplement d'être transmis.
- Une config désactivée puis réactivée par précondition **ne rafraîchit pas automatiquement** le matériel tant qu'aucun nouveau changement n'est détecté.

La solution finale retenue : séparer strictement la réception/mémorisation des données (toujours actives, jamais soumises à précondition) de leur affichage (piloté par une gâchette booléenne dédiée, intrinsèquement fiable puisqu'un changement d'état binaire est toujours transmis).

Le détail complet — schémas de câblage, historique des 5 itérations de diagnostic, configuration MobiFlight pas à pas, table de dépannage — est dans le rapport technique.

## Documentation

📄 **[Rapport technique complet](docs/)** (français et anglais) — électronique, firmware, configuration MobiFlight, dépannage

## Statut

✅ Module ILS validé en vol complet (interception → approche stabilisée → toucher des roues), sans extinction intempestive.

🔄 Essais en cours sur d'autres appareils à avionique Garmin (DA62, DA40, famille Cessna 172) pour confirmer la portabilité de la configuration.

## Remerciements

Je ne suis ni électronicien ni programmeur émérite — juste un passionné. Je me suis fait aider dans ce projet par Claude (l'IA d'Anthropic), notamment pour déboguer méthodiquement les comportements les plus tordus rencontrés en cours de route.

## Licence

*(à définir — MIT recommandé pour un projet hobbyiste partagé librement)*
