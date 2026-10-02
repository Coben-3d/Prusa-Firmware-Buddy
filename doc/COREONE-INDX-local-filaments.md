# CORE One + INDX : détails du portage

Pour installer et utiliser le prototype, voir [le guide commun](LOCAL-FILAMENTS.md).
Le fonctionnement sur la machine de l’auteur a été confirmé le 2 octobre 2026 ;
les limites de cette confirmation sont dans [la validation](LOCAL-FILAMENTS-VALIDATION.md).

## Base et indices

Base officielle 6.9.1 : `f1a123aba502bf8b043fcd9c779ee20b9253a62e`.
Firmware du projet : `6.9.1-color+1`, cible `COREONE_INDX`, carte `XBUDDY`.
Slicer : 2.9.6, modèle `COREONE_INDX8T`, huit lignes et huit buses, sans MMU.
Les indices physiques, virtuels et G-code restent des types distincts.
Les conversions officielles sont utilisées ; le contrat v2 accepte la
correspondance physique/virtuelle identique de cette base et rejette une autre.

## Stockage et opérations

La nouvelle clé de journal `INDX Loaded Filament Color v1` contient huit valeurs
64 bits : RGB sur les bits 0–23, indicateur connu au bit 24, matière encodée
sur les bits 32–39. Zéro signifie inconnu ; le noir reste une couleur connue.
Les clés et versions de calibration du journal sont conservées.

Le choix de couleur précède la matière dans le menu de chargement. La couleur
temporaire est atomique et réinitialisée au début de chaque opération.
Seule la fin réussie du chargement confirme la déclaration de la tête visée.
Les chargements interrompus, déchargements et changements de matière
invalident cette tête. Une purge seule conserve sa déclaration.

## API locale

`GET /api/v1/filaments`, sous l’authentification PrusaLink existante.
Autres méthodes authentifiées : 405. Le firmware officiel ne fournit pas cette route.

- `schema_version`: 2 ; `printer_model`: `COREONE_INDX`.
- `indexing`: `physical_tools` ; `tool_count`: 8.
- `slots`: huit objets identifiés de 0 à 7, chacun contenant `slot`, `virtual_tool`,
  `enabled`, `loaded`, `material`, `color`, `source`.
- `source`: `user_declared` ; matière absente et couleur inconnue : `null`.
- RGB : `#RRGGBB`. Noir : `#000000`.
- `loaded` représente la matière mémorisée, pas une mesure indépendante de présence.

Le renderer possède son instantané et conserve ses curseurs entre les blocs
JSON. Slicer réordonne les slots, refuse doublons et valeurs incohérentes,
limite le corps à 4 096 octets et refuse les redirections.
Les huit profils sont résolus avant mutation. Annuler conserve lignes et couleurs.
La sélection physique, la configuration et le projet sont revérifiés avant
d’appliquer une réponse asynchrone.

## Fichiers et entretien

| Composant | Fichiers principaux |
|---|---|
| Déclaration et journal | `src/common/loaded_filament_color.hpp`, `src/persistent_stores/store_instances/config_store/store_definition.*` |
| Chargement | `src/common/filament_to_load.cpp`, `src/marlin_stubs/pause/M701_2.cpp`, `src/marlin_stubs/pause/pause.cpp` |
| Menu | `src/gui/screen/screen_preheat.cpp` |
| PrusaLink | `lib/WUI/link_content/prusa_link_api_v1.cpp`, `lib/WUI/nhttp/filament_renderer.*`, `send_json.cpp` |
| Slicer | `LoadedFilamentColor*`, `GUI/Sidebar.*`, catalogues français |

Le BBF publié est construit avec `BOOTLOADER=EMPTY`, `BOOTLOADER_UPDATE=OFF`.
Il n’a pas de TLV de bootloader 11/12 ; le code principal commence à `0x08020200`.
Ses programmes `fw-indx_head.bin`, `fw-tool_offset_sensor.bin` et
`fw-xbuddy-extension.bin` sont identiques octet pour octet à ceux de l’officiel 6.9.1.

Pour reconstruire cette variante, suivre le README amont et conserver la cible,
GCC ARM 13.3.1, Release, le suffixe `-color+1` et `BUILD_NUMBER=1`.
Fournir les trois programmes secondaires officiels 6.9.1 via
`INDX_HEAD_BINARY_PATH`, `TOOL_OFFSET_SENSOR_BINARY_PATH` et
`XBUDDY_EXTENSION_BINARY_PATH`. Une recompilation avec d’autres programmes
secondaires ne reproduit pas le paquet publié.

À chaque mise à jour Prusa, revoir types et correspondances de têtes,
opérations de chargement, journal, contrat HTTP et profils Slicer.
INDX 4T et les autres modèles ne sont pas validés par ce portage.
