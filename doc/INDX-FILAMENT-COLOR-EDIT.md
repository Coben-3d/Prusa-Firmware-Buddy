# Corriger une couleur sans recharger : INDX color+3

Cette évolution ajoute un accès direct aux couleurs déclarées des huit têtes
de la CORE One + INDX. Elle réutilise la palette visuelle de 60 nuances du
prototype [color+2](INDX-COLOR-PALETTE.md), dont Benjamin a signalé le
fonctionnement sur sa machine le 2 octobre 2026.

**Color+3 est disponible dans la [release v0.2.0](https://github.com/Coben-3d/Prusa-Firmware-Buddy/releases/tag/local-filaments-v0.2.0).** Après son
installation, l’auteur a rapporté que l’ensemble fonctionnait. Ce retour global
ne documente pas chaque parcours matériel ci-dessous. Les tests logiciels ne
remplacent pas ces observations. La release v0.1.0 est conservée avec color+1.

## Utilisation

1. À l’arrêt, ouvrir **Filament → Couleurs des filaments**.
2. Choisir **Extrudeur 1** à **Extrudeur 8**. Chaque ligne affiche sa matière
   chargée, son carré de couleur et son code `#RRGGBB`, ou `Inconnu`.
3. Choisir une case de la palette. La nouvelle déclaration est enregistrée
   immédiatement pour cette tête et la ligne est actualisée.
4. Dans le PrusaSlicer du projet, appuyer sur le bouton de synchronisation
   locale habituel pour récupérer la nouvelle couleur.

`Retour` annule la sélection. `Inconnu` efface explicitement la couleur connue,
en conservant la matière chargée ; le noir reste un choix distinct. Une couleur
personnalisée déjà stockée est préservée à l’ouverture et à l’annulation.

Les huit numéros restent visibles. Une tête désactivée ou sans matière chargée
est grisée. L’édition est également bloquée pendant une impression ou une
opération de chargement, de préchauffage ou de cold pull. Une matière déjà
chargée mais dépourvue d’ancienne déclaration peut recevoir une couleur ici,
sans être rechargée.

Ce menu ne change ni le matériau, ni les températures, ni le profil partagé
PLA/PETG, ni la couleur temporaire du prochain chargement. Il ne sélectionne
pas physiquement la tête : aucune commande de déplacement, de chauffe,
d’extrusion ou de changement d’outil n’est envoyée.

## Pourquoi ce menu séparé

Dans le code officiel 6.9.1, le
[détail du filament](https://github.com/prusa3d/Prusa-Firmware-Buddy/blob/f1a123aba502bf8b043fcd9c779ee20b9253a62e/src/gui/screen/filament/screen_filament_detail.cpp)
édite un preset de matière ; ce preset peut servir à plusieurs bobines. Une
couleur de bobine appartient ici à une tête, pas au preset PLA. Le nouveau menu
est placé juste sous « Filaments chargés » dans le menu Filament et affiche les
déclarations par extrudeur.

Les actions de chargement/changement restent utiles pour remplacer une bobine.
La nouvelle entrée permet de corriger seulement sa représentation, par exemple
passer le bleu clair de l’extrudeur 6 au bleu foncé choisi par erreur.

## Implémentation

| Fichier | Modification |
|---|---|
| `src/gui/screen_menu_filament.hpp` | Nouvelle entrée pour les variantes INDX |
| `src/gui/screen/filament/screen_filament_colors.*` | Huit lignes matière/carré/RGB, ouverture de la palette, actualisation et contrôles d’état |
| `src/common/loaded_filament_color_edit.hpp` | Validation du matériau et comparaison de l’ancienne déclaration avant écriture |
| `src/gui/screen/filament/CMakeLists.txt` | Compilation du nouvel écran uniquement sur INDX |
| `src/lang/po/Prusa-Firmware-Buddy.pot`, `src/lang/po/fr/Prusa-Firmware-Buddy_fr.po` | Deux messages nouveaux et leurs traductions françaises |
| `tests/unit/common/loaded_filament_color_tests.cpp` | Correction de chaque tête, isolation, persistance, rejet d’une sélection périmée, écriture interrompue |
| `src/persistent_stores/journal/store_item_array.hpp` | Même assertion de taille, déplacée dans le corps du constructeur pour être utilisable avec Clang sur le Mac |

L’écriture utilise `loaded_filament_colors.transform(slot, ...)`, sous le
verrou du journal. La tête doit toujours être active, sa matière doit rester
identique et son ancienne déclaration doit correspondre à celle capturée à
l’ouverture. Le compteur de transitions FSM est aussi vérifié : une opération
qui s’est terminée pendant l’ouverture de la palette invalide la sélection,
même si elle a rechargé le même matériau avec la même couleur.

Le journal conserve son encodage et sa clé existants. La route locale du projet
`GET /api/v1/filaments`, protégée par l’authentification PrusaLink, expose la
nouvelle déclaration au prochain GET. Aucun endpoint d’écriture, nouveau
credential ou appel à Prusa Connect n’est ajouté. Le Slicer publié avec v0.1.0
est compatible ; son exécutable n’a pas été modifié.

## Vérifications logicielles

- Déclarations/journal/API : **14 cas, 3 856 assertions réussies**, dont quatre
  nouveaux cas de correction directe. Les tests emploient le vrai backend du
  journal et son tableau `JournalItemArray`, avec stockage en mémoire.
- Chaque tête est corrigée à son tour ; le matériau, les sept autres codes et
  le choix temporaire de chargement sont conservés. Le même choix confirmé
  deux fois ne provoque pas une deuxième écriture.
- Une déclaration remplacée entre-temps, un matériau différent, une tête
  indisponible ou une matière vide empêchent la correction.
- Après redémarrage simulé, le code corrigé est conservé. Une coupure simulée
  à chaque budget de 0 à 24 octets laisse l’ancien ou le nouveau code, avec
  les autres têtes intactes.
- Palette : **4 cas, 614 assertions réussies** ; aucune modification du
  comportement de sélection de color+2.
- Contrat Slicer : le JSON issu du renderer firmware après correction de la
  tête 8 est reçu par le client PrusaLink compilé via un serveur fictif sur
  `127.0.0.1`. Les huit indices, matières et couleurs correspondent exactement.
  Aucun accès à une imprimante réelle ou à ses credentials dans ce test.
- Le nouvel écran est soumis à la limite de 4 400 octets de `ScreenFactory`.
  Aucun buffer global de palette supplémentaire n’est ajouté.

## Firmware préparé

La cible est `COREONE_INDX`, carte `XBUDDY`, base officielle **6.9.1**,
`BUILD_NUMBER=3`, suffixe `-color+3`, Release ARM GCC 13.3.1,
`BOOTLOADER=EMPTY` et `BOOTLOADER_UPDATE=OFF`. Compilation ARM réussie
depuis le commit propre `a65b049c353e9d28d949776a1d73a4715e0351eb`.

Fichier local : `COREONE_INDX_6.9.1-color+3-direct-colors-prototype.bbf` (3,641,290 octets).

SHA-256 : `01e03cf82f170e9ffbaaf1f96397f1f980c8db3af2a0967821b2700e55bddce3`.

Les checksums principal et ressources du BBF sont valides. Il ne contient
aucune entrée de bootloader 11/12 ; les trois programmes secondaires INDX,
capteur d’offset et extension xBuddy sont identiques octet pour octet à
l’officiel 6.9.1. Les deux nouveaux messages français et « Extrudeur » ont
été vérifiés dans le `fr.mo` embarqué.

Occupation mesurée à l’édition de liens : FLASH 1,316,192 octets,
RAM 124,056 octets, CCMRAM 62,148 octets. La limite de stockage
du nouvel écran passe l’assertion de `ScreenFactory` à la compilation.

Ce même BBF est publié dans v0.2.0 sous le nom `COREONE_INDX_6.9.1-color+3.bbf`.
Les fichiers v0.1.0 sont conservés. Le fonctionnement global a été rapporté
par l’auteur ; les étapes détaillées du protocole ci-dessous ne sont pas toutes documentées.

Le 2 octobre 2026, le BBF color+3 a été copié sur la clé INDX `NO NAME`,
vérifié par SHA-256 puis la clé a été éjectée. L’ancien BBF color+2 a été
sauvegardé et vérifié sur le SSD avant son retrait de la clé ; les autres
fichiers ont été conservés. Aucun flash ni connexion à l’imprimante par l’agent.

## Essai sur l’imprimante

Après installation manuelle du nouveau BBF, sans lancer d’impression :

1. Ouvrir le nouveau menu et vérifier les numéros, matières, carrés et codes.
2. Corriger une tête chargée, puis vérifier qu’il n’y a ni chauffe ni mouvement
   et que les sept autres déclarations n’ont pas changé.
3. Synchroniser dans Slicer et vérifier la bonne ligne et la même nuance.
4. Recommencer avec une autre tête, tester `Retour`, `Inconnu` et le noir.
5. Redémarrer et vérifier la conservation des déclarations.
6. Vérifier qu’une tête vide ou désactivée reste grisée. Si possible, vérifier
   le refus d’une sélection devenue périmée après une opération distante.

Le retour global de fonctionnement de color+3 ne démontre pas séparément chacun
de ces parcours. Les mesures de pile et la navigation réelle restent à vérifier sur
machine. À chaque mise à jour Prusa, revoir le menu Filament, les indices de
têtes, le journal et les transitions FSM.

Les conditions du firmware personnalisé et le retour au firmware officiel
restent documentés dans le [guide commun](LOCAL-FILAMENTS.md). Aucun nouveau
geste matériel n’est requis par ce menu sur la machine déjà préparée.
