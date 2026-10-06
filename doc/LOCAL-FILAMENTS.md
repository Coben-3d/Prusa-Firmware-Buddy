# Filaments locaux Prusa — guide d’installation et d’utilisation

Choisissez une couleur sur l’écran pendant le chargement du filament, puis
synchronisez **matière et couleur** dans PrusaSlicer par le réseau local.
La sélection de matière habituelle reste disponible sur l’imprimante.

Projet communautaire de **Coben-3d**, basé sur les sources Prusa. Il s’agit
d’un **prototype expérimental**, sans publication ni validation officielle Prusa.
Le 2 octobre 2026, l’auteur a confirmé le fonctionnement sur sa MK4 puis sur
sa CORE One équipée d’INDX. Cette confirmation du flux utilisateur ne constitue
pas une validation de toutes les couleurs, de chaque tête, des longues impressions
ou du retour au firmware officiel.

**Ce projet demande deux éléments : notre firmware sur l’imprimante et
notre version complète personnalisée de PrusaSlicer à télécharger. Ce n’est
pas un plugin, module ou add-on à installer dans PrusaSlicer officiel.**
Le Slicer conserve ses fonctions habituelles et ajoute la synchronisation
locale. Le raccourci de l’application officielle ne contient pas ce bouton.

- [Installer sur MK4](INSTALL-MK4.md) : une bobine, sans MMU actif.
- [Installer sur CORE One / CORE One+ avec INDX 8 têtes](INSTALL-COREONE-INDX.md).
- **CORE One normale V1 sans INDX : portage en pause, aucun BBF fourni.**

Deux branches firmware distinctes sont entretenues dans le même dépôt, avec
un Slicer commun compatible avec les deux. Les BBF ne sont pas interchangeables.
La palette visuelle et la correction directe INDX ne sont pas encore portées
sur MK4. Les modèles MK4S, INDX 4T et CORE One L ne sont pas validés ici.

## Télécharger et choisir sa version

Les fichiers et leurs empreintes SHA-256 sont dans la [publication v0.2.1](https://github.com/Coben-3d/Prusa-Firmware-Buddy/releases/tag/local-filaments-v0.2.1).

| Machine | Firmware du projet | Base officielle | Configuration prévue |
|---|---|---|---|
| MK4 | `MK4_6.5.7-color+4.bbf` | 6.5.7 | Une bobine, sans MMU actif |
| CORE One + INDX | `COREONE_INDX_6.9.1-color+3.bbf` | 6.9.1 INDX | Huit têtes, palette de 60 nuances et correction directe ; profil `COREONE_INDX8T` |

Le paquet `PrusaSlicer-Local-Filaments-macOS-arm64-v0.2.1.zip` contient le
Slicer modifié **2.9.6+FilamentLocal-INDX**, compatible avec les deux protocoles.
Malgré son suffixe historique INDX, ce Slicer prend en charge MK4 et INDX.
Le binaire fourni cible **Apple Silicon et macOS 26.2 minimum**. Aucun binaire
Windows, Linux ou Mac Intel n’est fourni ; les sources Slicer sont disponibles
pour les porter et les compiler. Ces plateformes ne sont pas validées ici.

Le Slicer fourni possède une signature ad hoc, sans certificat Developer ID ni
notarisation Apple. En cas de blocage à l’ouverture, consulter
[la procédure Apple](https://support.apple.com/fr-fr/102445) et le fichier
`MACOS-OUVERTURE.md` inclus dans le ZIP.

## 1. Préparer l’imprimante

Les BBF sont **non signés**. La carte xBuddy accepte un firmware personnalisé
après rupture de la petite languette de sécurité marquée `!`. Ce retrait est
**physique et irréversible**. L’installation d’un firmware officiel ne rétablit
pas cette languette. Prusa précise que sa rupture seule n’annule pas la garantie,
mais publie aussi un avertissement de responsabilité concernant les dommages
ou préjudices qu’une imprimante avec ce sceau rompu pourrait causer.
Consultez les [conditions et la photo officielles Prusa](https://help.prusa3d.com/article/flashing-custom-firmware-core-one-l-core-one-mk4-s-mk3-9-s-mk3-5-s_814967)
avant de décider de cette modification.

La manipulation se fait sur une imprimante refroidie, arrêtée et débranchée.
Refermez le boîtier et retirez tout fragment avant la remise sous tension.
L’opération concerne uniquement la languette ; la carte xBuddy reste montée
et câblée. Notre fonction ne requiert pas de modification des têtes, des capteurs
ou des protections thermiques. Le firmware INDX fourni conserve les trois
programmes secondaires officiels 6.9.1 et ne transporte pas de mise à jour du
bootloader. Les vérifications de paquet figurent dans `release-manifest.json`.

## 2. Installer le firmware correspondant

1. Conservez les réglages et un firmware officiel correspondant à votre machine.
2. Utilisez une clé USB FAT32. Placez **un seul BBF à sa racine**, celui de votre
   machine. Mettez les anciens BBF à l’abri sur votre ordinateur.
3. Insérez la clé et redémarrez avec RESET. Pour une même version ou une version
   plus ancienne, suivez la [procédure de flash forcé Prusa](https://help.prusa3d.com/article/how-to-downgrade-firmware-core-one-mk4-s-mk3-9-s-mk3-5-s-xl_725930) :
   au logo, appuyez sur la molette, puis sélectionnez **FLASH** et confirmez.
4. Laissez l’opération se terminer et vérifiez la version affichée et le démarrage
   normal avant de charger un filament.

Si l’accueil revient directement, contrôlez le moment de l’appui, le BBF et
la clé. Une autre clé FAT32 peut aider ; un reset usine n’est pas une étape
normale de cette extension.

Voir aussi la [mise à jour USB officielle incluant INDX](https://help.prusa3d.com/article/how-to-update-firmware-core-one-l-core-one-core-one-indx-mk4-s-mk3-9-s-mk3-5-s-xl_453086).

## 3. Lancer le Slicer et configurer PrusaLink

1. Décompressez le ZIP dans un dossier où vous pouvez écrire.
2. Ouvrez **PrusaSlicer Filaments MK4.app** ou **PrusaSlicer Filaments INDX.app**.
   Les lanceurs `Lancer-PrusaSlicer-MK4.command` et `Lancer-PrusaSlicer-INDX.command`
   restent disponibles. Ils ouvrent le même exécutable avec `settings-mk4` ou
   `settings-indx`, distincts des réglages de PrusaSlicer officiel. Les réglages
   distribués sont neufs, sans hôte, identifiant ni mot de passe.
   **Gardez le dossier extrait complet** : déplacer seulement l’app casse ses
   chemins. Ajoutez si nécessaire ce lanceur au Dock ; l’ancien raccourci
   PrusaSlicer continue d’ouvrir l’officiel. Le ZIP de sources d’une branche
   GitHub ne contient pas le Slicer compilé : prenez le ZIP de la release.
3. Dans le Slicer ouvert, créez une **imprimante physique** liée au profil choisi.
   Choisissez le type d’hôte **PrusaLink**, renseignez l’adresse locale de votre
   imprimante et ses identifiants PrusaLink, puis utilisez **Tester**.
4. Le profil de départ MK4 utilise une buse 0,4 mm. Celui d’INDX utilise huit
   buses **HF0.4**. Vérifiez la correspondance avec les buses installées avant
   toute impression.

Activez PrusaLink depuis le menu réseau de l’imprimante si nécessaire.
Prusa Connect seul ne fournit pas cette connexion locale.
**Prusa Connect reste utilisable dans notre Slicer**, pour le compte et les
fonctions cloud habituelles. Le paquet v0.2.0 active le polling Connect et
utilise le trousseau macOS normal ; la connexion reste facultative pour notre
synchronisation locale. Les identifiants PrusaLink sont ceux de l’imprimante,
ils sont distincts du compte Connect.

Notre bouton lit `GET /api/v1/filaments` directement sur le LAN avec
l’authentification PrusaLink existante. Il n’appelle pas une API Connect et
n’envoie pas ces couleurs au cloud. Une imprimante visible dans Connect doit
également être configurée comme **imprimante physique PrusaLink** pour ce
bouton. Cette version ne propose pas de synchronisation à distance via Connect.
Voir [la distinction officielle Prusa](https://help.prusa3d.com/article/prusa-connect-and-prusalink-explained_302608).
La mise à jour automatique des presets et les notifications de version restent
désactivées dans les réglages de départ pour conserver la base distribuée.

Avec le firmware officiel, la route ajoutée par notre firmware est absente :
le Slicer seul ne suffit pas. Aucune solution sans flash n’est distribuée ici.

Le catalogue inclus est **PrusaResearch 2.5.10**. Le rattachement du profil à
Prusa Research est conservé pour que PrusaLink soit proposé dans la fenêtre
Imprimante physique. Une erreur mentionnant OctoPrint et un mauvais appariement
avec PrusaLink invite à vérifier ce type d’hôte et ce rattachement.

## 4. Charger et synchroniser

1. Sur l’imprimante, ouvrez le chargement du filament de la tête voulue.
2. Choisissez **Couleur du filament**, puis la matière habituelle, et terminez
   le chargement. La couleur est confirmée seulement après réussite.
3. Dans l’onglet **Plateau** de notre Slicer, sélectionnez l’imprimante physique configurée et cliquez sur
   le bouton de synchronisation placé sous les lignes de filament.

Sur MK4, une ligne est synchronisée. Sur INDX, les têtes physiques 1 à 8
correspondent aux huit lignes ; l’API utilise les indices 0 à 7.
Un profil matière déjà compatible est conservé, avec ses personnalisations.
Sinon, le profil Generic compatible unique est privilégié ; plusieurs candidats
demandent un choix. **Changer de profil applique ses paramètres d’impression**,
dont les températures. Les dialogues habituels de modifications sont conservés.

Une tête vide ou désactivée conserve ses réglages Slicer. Une matière sans profil
compatible conserve le profil actuel ; une couleur connue peut néanmoins être
appliquée. Une couleur inconnue conserve la couleur de sa ligne.
Les erreurs et les réponses devenues obsolètes laissent les réglages inchangés.

La couleur est une **déclaration manuelle**, pas une mesure optique ni une
référence de bobine. L’état initial est inconnu. Un chargement interrompu ou
un déchargement invalide la déclaration concernée ; une purge seule la conserve.
La persistance est vérifiée par les tests du journal ; une campagne matérielle
complète avec redémarrages et toutes les opérations reste à documenter.

Sur INDX color+3, on peut aussi corriger une couleur sans recharger :
**Filament → Couleurs des filaments → Extrudeur 1…8**, imprimante à l’arrêt.
Les lignes affichent matière, carré et code RGB. La palette comprend 60 nuances
sur trois pages ; le choix est enregistré immédiatement pour la tête visée.
`Retour` annule, `Inconnu` efface la déclaration ; le noir reste distinct.
Une tête vide/désactivée ou une opération en cours empêche l’édition. Ce menu
ne commande ni chauffe ni mouvement et ne change pas la matière. Synchronisez
ensuite dans Slicer. Un vrai changement de bobine utilise les opérations normales.

## Réseau local sur macOS

Un message « PrusaLink inaccessible ou requête échouée » peut aussi venir
de l’autorisation réseau de l’application, même avec une IP correcte. Ouvrez
d’abord la page PrusaLink dans le navigateur du même Mac, puis essayez
**Tester** dans l’imprimante physique pour distinguer l’accès réseau de la
route de synchronisation et des identifiants.

Autorisez **PrusaSlicer Filaments MK4** ou **PrusaSlicer Filaments INDX** si
macOS le demande. Retrouvez ce réglage dans **Réglages Système →
Confidentialité et sécurité → Réseau local** : [procédure Apple](https://support.apple.com/fr-fr/guide/mac-help/mchla4f49138/mac).
En cas d’état incohérent, désactivez puis réactivez uniquement l’application
concernée, puis réessayez Tester.

Le 6 octobre 2026, les `.app` basées sur un script ont présenté une boucle
d’autorisation sur le Mac de l’auteur. Un lanceur natif a rétabli la
synchronisation INDX, confirmée par l’auteur. **Utilisez v0.2.1**, avec un
exécutable principal natif, un UUID distinct par variante et une description
d’accès local. Cela corrige le paquet ; les firmwares et le moteur Slicer
restent ceux déjà distribués. Aucun flash supplémentaire n’est requis.

Enregistrez et quittez l’ancienne session avant d’ouvrir la nouvelle.
Le lanceur reste actif pendant la session et revient sur Slicer lors d’une
nouvelle ouverture ; quittez Slicer par son menu. Gardez tout le dossier extrait.
Les `.command` ouverts depuis Terminal restent disponibles. Apple documente
l’autorisation automatique du réseau local pour les outils lancés depuis
Terminal et leurs enfants, ainsi que les exigences de signature et d’UUID :
[TN3179](https://developer.apple.com/documentation/technotes/tn3179-understanding-local-network-privacy).
Les lanceurs sont signés ad hoc ; le projet ne possède pas de certificat
Developer ID ni de notarisation. Le correctif confirmé sur ce Mac ne constitue
pas une validation sur toutes les versions et configurations macOS.

## Questions courantes

| Symptôme | Vérification |
|---|---|
| Bouton entièrement absent | Ouvrir **PrusaSlicer Filaments**, pas l’officiel ; onglet Plateau, panneau de droite visible, profil FFF |
| Bouton grisé | Sélection physique PrusaLink avec adresse locale ; une buse MK4 ou huit INDX, sans mélange virtuel |
| Compte connecté mais imprimantes Connect absentes | Paquet v0.2.0 et reconnexion si nécessaire ; l’ancien v0.1.0 désactivait le polling et bloquait le trousseau |
| 401 / 403 | Identifiants PrusaLink et type d’authentification |
| 404 sur la synchronisation | Firmware du projet installé, cible prise en charge et MMU inactif sur MK4 |
| Imprimante introuvable | Adresse locale actuelle, PrusaLink activé et accès réseau depuis le Mac |
| Page PrusaLink accessible, Slicer en échec réseau | Paquet v0.2.1, autorisation Réseau local du lanceur, puis Tester ; voir la section macOS |
| Couleur inconnue | Déclarer la couleur pendant un chargement normal terminé |
| Matière inchangée dans Slicer | Profil compatible installé ; confirmer un éventuel dialogue de choix |
| INDX refusée par le bouton | Profil `COREONE_INDX8T`, huit buses et huit lignes ; MMU désactivé |
| Slicer ne démarre pas | Apple Silicon, macOS 26.2 minimum, archive entièrement extraite |

## Mise à jour et reprise de ses profils

La release **v0.1.0 est conservée comme historique** : INDX color+1 avec les
couleurs nommées initiales, et anciens lanceurs de diagnostic. La v0.2.0
contient INDX color+3 et les lanceurs Slicer corrigés. Le BBF MK4 et l’exécutable
Slicer sont inchangés ; ce dernier reste 2.9.6+FilamentLocal-INDX.
La v0.2.1 conserve les mêmes BBF et le même moteur Slicer ; elle ajoute les
lanceurs macOS natifs et le dépannage de leur autorisation réseau local.

Une nouvelle extraction fournit des réglages neufs. Gardez votre ancien dossier,
exportez vos profils depuis le menu Fichier de l’ancien Slicer puis importez-les
dans la nouvelle version. Aucun profil personnel n’est repris automatiquement.
Les bundles peuvent contenir des informations privées : vérifiez-les avant
partage. Reconnectez-vous à Prusa Connect dans le Slicer personnalisé si nécessaire.

Les mises à jour officielles ne reprennent pas automatiquement notre bouton.
Conservez le lanceur de cette version ; une nouvelle base nécessite un portage.

## Retour à l’officiel et limites

Téléchargez le BBF officiel adapté à la machine : [MK4 6.5.7](https://github.com/prusa3d/Prusa-Firmware-Buddy/releases/tag/v6.5.7)
ou [CORE One INDX 6.9.1](https://github.com/prusa3d/Prusa-Firmware-Buddy/releases/tag/v6.9.1),
puis utilisez la procédure USB Prusa. Le journal conserve ses versions et ses
clés de calibration ; un reset usine n’est pas requis par notre extension.
Le retour matériel à l’officiel n’a pas été validé dans cette session.
Pour utiliser le Slicer officiel, ouvrez son application habituelle ; elle
n’expose pas notre bouton. La languette xBuddy retirée reste irréversible.

Ce prototype vise MK4 mono-bobine et CORE One INDX 8T. Les autres modèles,
INDX 4T, MMU et correspondances physiques/virtuelles non identiques sont hors
du périmètre validé. L’interface du changement M600 pendant une impression
ne reçoit pas un nouveau sélecteur de couleur. Chaque nouvelle version Prusa
nécessite de revoir et tester le portage.

## Sources, tests et contributions

- [Firmware INDX](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/coreone-indx-loaded-filament-colors) · [firmware MK4](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/mk4-loaded-filament-color)
- [PrusaSlicer modifié](https://github.com/Coben-3d/PrusaSlicer/tree/feature/indx-local-filaments)
- [Résultats et protocole](https://github.com/Coben-3d/Prusa-Firmware-Buddy/blob/feature/coreone-indx-loaded-filament-colors/doc/LOCAL-FILAMENTS-VALIDATION.md)
- [Détails INDX](https://github.com/Coben-3d/Prusa-Firmware-Buddy/blob/feature/coreone-indx-loaded-filament-colors/doc/COREONE-INDX-local-filaments.md)
- [Détails MK4](https://github.com/Coben-3d/Prusa-Firmware-Buddy/blob/feature/mk4-loaded-filament-color/doc/mk4_loaded_filament_color.md)

Signalez un problème dans [Issues](https://github.com/Coben-3d/Prusa-Firmware-Buddy/issues)
avec modèle, versions, système d’exploitation et étapes pour le reproduire.
Retirez adresses, mots de passe et clés API des captures et journaux partagés.

Les licences des projets amont sont conservées : firmware GPLv3, PrusaSlicer
AGPLv3 ; les ressources graphiques et dépendances conservent leurs licences propres.
Ce projet n’ajoute aucune lecture cloud des couleurs : PrusaLink communique
directement sur le réseau local, sans dépendance à Prusa Connect.
