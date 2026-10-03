# Installation — CORE One / CORE One+ avec INDX 8 têtes

**Cible : CORE One équipée d’INDX avec huit têtes ; base officielle 6.9.1 INDX.**
Une CORE One normale, y compris V1, n’utilise pas ce firmware.
Le projet demande notre firmware et notre version complète de PrusaSlicer ;
ce n’est pas un module à installer dans l’officiel.

## Télécharger

Depuis la [release v0.2.0](https://github.com/Coben-3d/Prusa-Firmware-Buddy/releases/tag/local-filaments-v0.2.0), prendre :

- **`COREONE_INDX_6.9.1-color+3.bbf`**.
- **`PrusaSlicer-Local-Filaments-macOS-arm64-v0.2.0.zip`** : Apple Silicon,
  macOS 26.2 minimum, le même exécutable compatible avec MK4 et INDX.
- `SHA256SUMS.txt` et `release-manifest.json` pour vérifier les téléchargements.

Firmware SHA-256 :
`01e03cf82f170e9ffbaaf1f96397f1f980c8db3af2a0967821b2700e55bddce3`.

## Installer

1. Lire les [conditions du firmware personnalisé et la procédure USB](LOCAL-FILAMENTS.md).
   La languette xBuddy `!` doit être rompue pour ce BBF non signé ; c’est irréversible.
2. Mettre uniquement le BBF **COREONE_INDX** à la racine d’une clé FAT32 et
   suivre la procédure Prusa du guide commun. Ne pas utiliser le BBF MK4.
3. Extraire tout le ZIP et ouvrir **PrusaSlicer Filaments INDX.app**, ou
   `Lancer-PrusaSlicer-INDX.command`. Garder le dossier extrait complet.
4. Vérifier le profil **COREONE_INDX8T, huit buses HF0.4**, huit lignes de
   filament et MMU inactif ; les buses doivent correspondre à la machine.
   Les réglages du lanceur sont dans `settings-indx`.
5. Créer et sélectionner une **imprimante physique PrusaLink**, avec adresse
   locale et identifiants PrusaLink, puis utiliser **Tester**.

## Choisir les couleurs et leurs nuances

Pendant le chargement de la tête voulue, ouvrir **Couleur du filament**.
La palette affiche **60 nuances en trois pages**, avec plusieurs bleus,
marrons et autres familles. Le choix indique un carré et son code `#RRGGBB`.
Choisir ensuite la matière réelle et terminer le chargement pour confirmer.
La molette permet de naviguer ; le code prend aussi en charge le tactile activé.
`Retour` annule ; l’état inconnu est distinct du noir.
La couleur est déclarée manuellement, sans mesure optique.

Voir [les illustrations et détails de la palette](https://github.com/Coben-3d/Prusa-Firmware-Buddy/blob/feature/coreone-indx-loaded-filament-colors/doc/INDX-COLOR-PALETTE.md).

## Corriger une couleur sans recharger

1. Imprimante à l’arrêt : **Filament → Couleurs des filaments**.
2. Choisir **Extrudeur 1** à **Extrudeur 8**. Chaque ligne indique matière,
   carré et code RGB, ou `Inconnu`.
3. Choisir une case : la déclaration est enregistrée immédiatement pour cette
   tête, sans chauffe, déplacement, extrusion ni changement d’outil.
4. Synchroniser ensuite dans notre Slicer.

Une tête vide ou désactivée est grisée. Une impression ou une opération de
filament/préchauffage/cold pull bloque l’édition. `Retour` conserve l’ancienne
couleur ; `Inconnu` efface sa déclaration sans changer la matière. Un véritable
remplacement de bobine ou de matière suit les opérations de filament habituelles.

## Synchroniser les huit lignes

Dans l’onglet **Plateau** de notre Slicer, sélectionner l’imprimante physique
PrusaLink et cliquer sur **Sync filament from PrusaLink** sous les huit lignes.
Extrudeurs physiques 1 à 8 = lignes 1 à 8 = indices API 0 à 7. La synchronisation
récupère matière et couleur. Un changement de profil matière change ses
paramètres d’impression ; vérifier les températures et les éventuels dialogues.

Prusa Connect reste disponible pour le compte et les imprimantes cloud.
Il ne remplace pas la configuration locale : notre bouton lit
`GET /api/v1/filaments` via PrusaLink et n’utilise pas une API Connect.
Si le bouton est absent, vérifier le lanceur personnalisé et l’onglet Plateau.

## Versions, sources et limites

v0.1.0 contient **color+1**, menu nommé initial. **color+2** ajoute la palette ;
**color+3**, fourni dans v0.2.0, ajoute la correction directe et conserve la
palette. Le binaire Slicer reste identique ; les lanceurs v0.2.0 réactivent
Prusa Connect et le trousseau et portent des noms explicites.

- [Branche firmware INDX](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/coreone-indx-loaded-filament-colors).
- [Détails techniques INDX](https://github.com/Coben-3d/Prusa-Firmware-Buddy/blob/feature/coreone-indx-loaded-filament-colors/doc/COREONE-INDX-local-filaments.md).
- [Menu de correction directe](https://github.com/Coben-3d/Prusa-Firmware-Buddy/blob/feature/coreone-indx-loaded-filament-colors/doc/INDX-FILAMENT-COLOR-EDIT.md).
- [Slicer commun](https://github.com/Coben-3d/PrusaSlicer/tree/feature/indx-local-filaments).
- [Tests et limites](LOCAL-FILAMENTS-VALIDATION.md).
- [Guide commun : dépannage, profils, garantie et retour officiel](LOCAL-FILAMENTS.md).

L’auteur a rapporté le fonctionnement global après installation ; ce retour
n’est pas une campagne exhaustive de toutes les têtes et opérations.
INDX 4T, CORE One L et CORE One sans INDX ne sont pas validées par ce paquet.
