# Installation — Prusa MK4

**Cible : MK4, une bobine, sans MMU actif ; base officielle 6.5.7.**
Ce projet demande notre firmware et notre version complète de PrusaSlicer.
Ce n’est pas un module à ajouter au Slicer officiel.

## Télécharger

Depuis la [release v0.2.1](https://github.com/Coben-3d/Prusa-Firmware-Buddy/releases/tag/local-filaments-v0.2.1), prendre :

- **`MK4_6.5.7-color+4.bbf`**.
- **`PrusaSlicer-Local-Filaments-macOS-arm64-v0.2.1.zip`** : Apple Silicon,
  macOS 26.2 minimum, le même exécutable compatible avec MK4 et INDX.
- `SHA256SUMS.txt` et `release-manifest.json` pour vérifier les téléchargements.

Firmware SHA-256 :
`c71e8ca2316fea52c5434fb963bbb78b402717ca9f4a86c79e0d2f0f80e90178`.

## Installer et utiliser

1. Lire les [conditions du firmware personnalisé et la procédure USB](LOCAL-FILAMENTS.md).
   La languette xBuddy `!` doit être rompue pour ce BBF non signé ; c’est irréversible.
2. Mettre uniquement le BBF **MK4** à la racine d’une clé FAT32, puis suivre
   la procédure Prusa du guide commun. Ne pas utiliser le fichier INDX.
3. Extraire tout le ZIP et ouvrir **PrusaSlicer Filaments MK4.app**, ou
   `Lancer-PrusaSlicer-MK4.command`. Garder le dossier extrait complet.
   Autoriser le lanceur concerné si macOS demande l’accès au réseau local.
4. Vérifier le profil **MK4 Input Shaper, une buse 0,4 mm** et ses réglages avant
   impression. Les réglages du lanceur sont dans `settings-mk4`.
5. Créer et sélectionner une **imprimante physique PrusaLink**, avec adresse
   locale et identifiants PrusaLink, puis utiliser **Tester**.
6. Pendant un chargement normal, choisir **Couleur du filament**, puis PLA ou
   la matière réelle. Terminer le chargement pour confirmer la déclaration.
7. Dans l’onglet **Plateau** de notre Slicer, cliquer sur le bouton placé sous
   la ligne de filament. Matière et couleur sont récupérées.

La MK4 utilise actuellement une liste de couleurs nommées : noir, blanc, gris,
rouge, orange, jaune, vert, bleu, violet, marron, rose, plus l’état inconnu.
**La palette visuelle de 60 nuances et le menu de correction sans recharge
INDX ne sont pas encore portés sur MK4.** Le BBF MK4 n’a pas changé entre
v0.1.0, v0.2.0 et v0.2.1. La v0.2.1 corrige le lanceur macOS pour le réseau
local ; aucun nouveau flash n’est requis si ce BBF est déjà installé.

Le compte Prusa Connect reste utilisable dans cette version. Il ne remplace
pas la connexion locale PrusaLink. Si le bouton est absent, vérifier que le
raccourci ouvre **PrusaSlicer Filaments**, pas le Slicer officiel.
Le [guide commun](LOCAL-FILAMENTS.md) détaille le dépannage, l’import de profils,
les conditions de garantie, la maintenance et le retour au firmware officiel.

## Sources et limites

- [Branche firmware MK4](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/mk4-loaded-filament-color).
- [Détails techniques MK4](https://github.com/Coben-3d/Prusa-Firmware-Buddy/blob/feature/mk4-loaded-filament-color/doc/mk4_loaded_filament_color.md).
- [PrusaSlicer commun](https://github.com/Coben-3d/PrusaSlicer/tree/feature/indx-local-filaments).
- [Tests et limites](LOCAL-FILAMENTS-VALIDATION.md).

MK4S et MMU ne sont pas validés par ce BBF. CORE One normale V1 : aucun BBF
fourni, portage en pause. Les fichiers de machines différentes ne sont pas interchangeables.
