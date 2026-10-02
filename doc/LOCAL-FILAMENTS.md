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

## Télécharger et choisir sa version

Les fichiers et leurs empreintes SHA-256 sont dans la [publication v0.1.0](https://github.com/Coben-3d/Prusa-Firmware-Buddy/releases/tag/local-filaments-v0.1.0).

| Machine | Firmware du projet | Base officielle | Configuration prévue |
|---|---|---|---|
| MK4 | `MK4_6.5.7-color+4.bbf` | 6.5.7 | Une bobine, sans MMU actif |
| CORE One + INDX | `COREONE_INDX_6.9.1-color+1.bbf` | 6.9.1 INDX | Huit têtes, profil `COREONE_INDX8T` |

Le paquet `PrusaSlicer-Local-Filaments-macOS-arm64-v0.1.0.zip` contient le
Slicer modifié **2.9.6+FilamentLocal-INDX**, compatible avec les deux protocoles.
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

La documentation Prusa indique un appui au logo. Pendant l’essai de l’auteur,
les tentatives avec un appui n’ont pas ouvert le flash ; un essai avec deux
appuis rapides a ensuite été proposé avant sa confirmation globale de succès.
Le nombre d’appuis qui a finalement fonctionné n’a pas été confirmé séparément.
Si l’accueil revient directement, contrôlez le moment de l’appui et la clé.
Prusa précise qu’une clé lisible pour les impressions peut échouer au flash ;
une autre clé FAT32 est alors une piste. Cela ne justifie pas un reset usine.

Voir aussi la [mise à jour USB officielle incluant INDX](https://help.prusa3d.com/article/how-to-update-firmware-core-one-l-core-one-core-one-indx-mk4-s-mk3-9-s-mk3-5-s-xl_453086).

## 3. Lancer le Slicer et configurer PrusaLink

1. Décompressez le ZIP dans un dossier où vous pouvez écrire.
2. Lancez `Lancer-PrusaSlicer-MK4.command` ou `Lancer-PrusaSlicer-INDX.command`.
   Ces lanceurs utilisent deux dossiers de réglages séparés, sans installation
   dans Applications. Les configurations distribuées ne contiennent aucun hôte,
   identifiant ou mot de passe.
3. Dans le Slicer ouvert, créez une **imprimante physique** liée au profil choisi.
   Choisissez le type d’hôte **PrusaLink**, renseignez l’adresse locale de votre
   imprimante et ses identifiants PrusaLink, puis utilisez **Tester**.
4. Le profil de départ MK4 utilise une buse 0,4 mm. Celui d’INDX utilise huit
   buses **HF0.4**. Vérifiez la correspondance avec les buses installées avant
   toute impression.

Activez PrusaLink depuis le menu réseau de l’imprimante si nécessaire.
Prusa Connect seul ne fournit pas cette connexion locale.
Les lanceurs du prototype bloquent l’accès au trousseau macOS ; utilisez les
champs d’authentification de PrusaLink dans Slicer, sans stockage dans le
trousseau via ce paquet. Ils désactivent également le polling Connect et la
mise à jour automatique des presets dans ces réglages de départ.

Le catalogue inclus est **PrusaResearch 2.5.10**. Le rattachement du profil à
Prusa Research est conservé pour que PrusaLink soit proposé dans la fenêtre
Imprimante physique. Une erreur mentionnant OctoPrint et un mauvais appariement
avec PrusaLink invite à vérifier ce type d’hôte et ce rattachement.

## 4. Charger et synchroniser

1. Sur l’imprimante, ouvrez le chargement du filament de la tête voulue.
2. Choisissez **Couleur du filament**, puis la matière habituelle, et terminez
   le chargement. La couleur est confirmée seulement après réussite.
3. Dans Slicer, sélectionnez l’imprimante physique configurée et cliquez sur
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

## Questions courantes

| Symptôme | Vérification |
|---|---|
| 401 / 403 | Identifiants PrusaLink et type d’authentification |
| 404 sur la synchronisation | Firmware du projet installé, cible prise en charge et MMU inactif sur MK4 |
| Imprimante introuvable | Adresse locale actuelle, PrusaLink activé et accès réseau depuis le Mac |
| Couleur inconnue | Déclarer la couleur pendant un chargement normal terminé |
| Matière inchangée dans Slicer | Profil compatible installé ; confirmer un éventuel dialogue de choix |
| INDX refusée par le bouton | Profil `COREONE_INDX8T`, huit buses et huit lignes ; MMU désactivé |
| Slicer ne démarre pas | Apple Silicon, macOS 26.2 minimum, archive entièrement extraite |

## Retour à l’officiel et limites

Téléchargez le BBF officiel adapté à la machine : [MK4 6.5.7](https://github.com/prusa3d/Prusa-Firmware-Buddy/releases/tag/v6.5.7)
ou [CORE One INDX 6.9.1](https://github.com/prusa3d/Prusa-Firmware-Buddy/releases/tag/v6.9.1),
puis utilisez la procédure USB Prusa. Le journal conserve ses versions et ses
clés de calibration ; un reset usine n’est pas requis par notre extension.
Le retour matériel à l’officiel n’a pas été validé dans cette session.

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
