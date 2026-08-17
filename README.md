# G1000 Baron 58 — Cockpit Panel (COMMANDE G1000)

Panneau de commande physique pour le G1000 du Beechcraft Baron 58 sous Microsoft Flight Simulator 2024/2020, piloté par un Arduino Mega 2560 Pro Mini via MobiFlight.

![Statut](https://img.shields.io/badge/status-fonctionnel%20%E2%80%94%20valid%C3%A9%20en%20vol-brightgreen)
![Plateforme](https://img.shields.io/badge/plateforme-Arduino%20Mega%202560-blue)
![MobiFlight](https://img.shields.io/badge/MobiFlight-Community%20Firmware-orange)

## En bref

Adepte de Flight Simulator, je voulais accéder facilement aux commandes du G1000 sans construire un home cockpit complet. Ce dépôt contient tout ce qu'il faut pour reproduire un panneau de commande physique complet : encodeurs CRS/BARO et FMS, joystick 4 axes, boutons G1000, Master Alarm/Caution avec voyants et alarme sonore, rétro-éclairage variable, et une barre LED qui reproduit l'indicateur de déviation ILS (Localizer/Glideslope).

Le morceau qui a demandé le plus de travail — la barre ILS — a nécessité plusieurs refontes du firmware avant d'obtenir un comportement fiable en vol. Le détail complet de cette investigation est dans le rapport technique (voir plus bas), pour ceux que ça intéresse.

## Démonstration vidéo

- 🎥 [Démonstration 1](https://www.youtube.com/watch?v=X_OnPQQrdoY)
- 🎥 [Démonstration 2](https://www.youtube.com/watch?v=7gS3sEIO30M)

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
## Firmware — basé sur MobiFlight

Le firmware `ILS_NeoPixel_mega` est issu du firmware de base **MobiFlight** (version 3.1.0), sur lequel ont été ajoutés/adaptés :
- la gestion de la barre LED ILS (LOC/GS, WS2812B)
- le pilotage des LEDs DM13A (backlighting)
- la gestion des boutons Master Alarm/Caution avec buzzer
- les encodeurs rotatifs CRS/BARO et FMS (double anneau)
- le joystick 4 axes

Le firmware reste compatible avec l'écosystème MobiFlight (upload via MobiFlight Firmware Uploader / PlatformIO, `device.json` conforme au schéma MobiFlight).

## Le morceau intéressant : la logique d'extinction ILS

La vraie difficulté de ce projet n'a jamais été d'afficher la bonne LED — c'est trivial. Le vrai problème a été de l'**éteindre correctement** quand le signal ILS devient invalide. Plusieurs itérations ont été nécessaires avant de comprendre que la cause racine tenait à deux comportements peu documentés de MobiFlight :

- MobiFlight n'envoie une donnée au device **que si la valeur change** — un index de déviation qui reste stable plusieurs secondes en approche stabilisée arrête tout simplement d'être transmis.
- Une config désactivée puis réactivée par précondition **ne rafraîchit pas automatiquement** le matériel tant qu'aucun nouveau changement n'est détecté.

La solution finale retenue : séparer strictement la réception/mémorisation des données (toujours actives, jamais soumises à précondition) de leur affichage (piloté par une gâchette booléenne dédiée, intrinsèquement fiable puisqu'un changement d'état binaire est toujours transmis).

Le détail complet — schémas de câblage, historique des 5 itérations de diagnostic, configuration MobiFlight pas à pas, table de dépannage — est dans le rapport technique.

## Boutons Master Caution / Master Warning — configuration MobiFlight spécifique par appareil

Les boutons **Master Alarm** (SW8, GPIO36 = switch / GPIO37 = LED) et **Master Caution** (SW7, GPIO38 = switch / GPIO39 = LED) sont pilotés matériellement de façon identique pour tous les avions (mêmes GPIO, même buzzer NE555 sur GPIO43). En revanche, **la variable simulateur qui déclenche l'allumage côté MobiFlight change d'un appareil à l'autre** : chaque avion MSFS expose ses propres SimVars / L:Vars pour l'alarme et la caution (nom, type, seuil de déclenchement), donc la config MobiFlight (Config-item / preset) doit être recréée ou adaptée à chaque changement d'appareil piloté par le panneau.

| Appareil | SimVar / L:Var utilisée | Type de config MobiFlight | Remarques |
|---|---|---|---|
| Beechcraft Baron 58 | *(à compléter)* | *(Preset / Custom Code)* | |
| DA62 | *(à compléter)* | | |
| DA40-NG | *(à compléter)* | | |
| DA40 TDI | *(à compléter)* | | |
| TBM 930 | *TBM 930* |MASTER_CAUTION_YELLOW_LED | |
| C208B | *(à compléter)* | | |

**Point d'attention** : contrairement aux SimVars natives disponibles sur certains appareils, plusieurs avions tiers n'exposent l'alarme/caution que via des **L:Vars propres à leur addon**, à retrouver dans la documentation du développeur ou via le débogueur de variables MobiFlight/FSUIPC (WASM) avant de pouvoir créer le Config-item.

> ⚠️ Il n'existe pas de config MobiFlight universelle pour ces deux boutons : à chaque changement d'appareil, vérifier/adapter la variable source dans MobiFlight, sinon les LEDs Master Alarm/Caution restent inertes malgré l'appui physique.

## Boîtier — fabrication en impression 3D

Le boîtier a été entièrement réalisé en impression 3D, sur une imprimante **Creality K2 Pro** (extrudeur direct drive).

**Filament** : PLA pour l'ensemble des pièces structurelles. Le PETG convient également (meilleure tenue mécanique et thermique), mais demande un réglage plus fin.

**Pièces vitrées (PETG transparent)** : certaines pièces nécessitant une transparence optique — pour laisser passer la lumière du rétroéclairage — ont été imprimées en PETG transparent :
- Plaque avant (lettrage)
- Touches fond
- Cales touches
- Glace LOC/GLIDE (fenêtre de la barre LED ILS)

⚠️ **Contrainte d'impression** : l'impression en PETG transparent ne peut se faire qu'à **vitesse très lente**. Une vitesse standard donne un rendu opaque/laiteux qui casse l'effet de transparence recherché pour la diffusion de la lumière.

## Documentation

📄 **[Rapport technique complet](docs/)** (français et anglais) — électronique, firmware, configuration MobiFlight, dépannage

## Statut

✅ Module ILS validé en vol complet (interception → approche stabilisée → toucher des roues), sans extinction intempestive.

🔄 Essais en cours sur d'autres appareils à avionique Garmin (DA62, DA40, famille Cessna 172) pour confirmer la portabilité de la configuration.

## Remerciements

Je ne suis ni électronicien, ni programmeur émérite — juste un passionné de MSFS. Je me suis fait aider dans ce projet pour les parties electroniques et logiciel par Claude (l'IA d'Anthropic), notamment pour déboguer méthodiquement les comportements les plus tordus rencontrés en cours de route. 

## Licence

*Creative Commons Attribution — Pas d'Utilisation Commerciale 4.0 International (CC BY-NC 4.0)*
