# Validation du projet — v0.1.0 à v0.2.1

## Essais matériels déclarés par l’auteur

Le 2 octobre 2026, l’auteur a confirmé le chargement avec couleur violette et
la synchronisation locale sur sa MK4. La synchronisation du profil matière a
ensuite été ajoutée à Slicer. Après le portage, il a confirmé : « Tout est
fonctionnel » sur sa CORE One + INDX équipée du firmware de base 6.9.1.
Les essais ont été réalisés et observés par l’auteur ; aucune capture de réponse
authentifiée réelle ou trace de chaque tête n’est publiée.

Cette confirmation couvre le flux utilisateur essayé. Elle ne démontre pas une
campagne exhaustive sur les huit têtes, toutes les couleurs, chaque séquence
M600/autoload/runout, les coupures physiques, les longues impressions ou le retour
au firmware officiel. Ces points restent à vérifier et documenter individuellement.

## Contrôles logiciels de la version initiale color+1 / Slicer

| Contrôle final INDX / Slicer | Résultat |
|---|---|
| Firmware natif : encodage, couleur atomique, vrai journal, huit têtes, compactage, interruptions, rendu JSON progressif | 9 cas, 2 434 assertions réussies |
| Protocole et application de réglages Slicer | 9 cas, 112 assertions réussies |
| Client HTTP réel sur serveur local factice : clé, Digest, erreurs, délais, annulation, redirections et huit emplacements | 19 cas réussis |
| Interface wx native : huit profils, conservation des modifications, annulation et réponses tardives ; régressions MK4 | 22 cas réussis, dont 9 INDX |
| Réponse produite par le vrai renderer firmware et lue par le vrai client Slicer | Contrat réussi, réponse de 1 044 octets |

Total : **59 cas réussis**, plus le contrôle du contrat firmware/Slicer.
Le firmware ARM et le Slicer ARM64 ont été compilés. Le firmware utilise
66,76 % de FLASH, 63,13 % de RAM et 94,83 % de CCMRAM dans cette compilation.
Ces chiffres ne sont pas une comparaison avec un build officiel identique.

Le portage conserve les tests MK4 : 7 cas hôte et 460 assertions, ainsi que
six scénarios vérifiés de firmware MK4 virtuel. Le chargement complet dans ce
simulateur restait limité par le modèle mécanique ; les protections n’ont pas
été désactivées pour le faire passer.

## Retrouver les contrôles

Les sources des tests firmware sont sous `tests/unit/common/loaded_filament_color_tests.cpp`
et `tests/integration/test_loaded_filament_color.py` sur la branche MK4.
Le helper MK4 est `utils/run_loaded_filament_color_tests.py`.
Les suites Slicer sont dans `tests/slic3rutils/loaded_filament_color_*`,
`test_loaded_filament_color_http.py`, `test_loaded_filament_color_gui.py`
et `test_loaded_filament_indx_gui.py`.

Les tests réseau utilisent des identifiants factices et des serveurs loopback.
Les scénarios graphiques ont des réglages jetables et bloquent le trousseau.
Aucun identifiant réel, réglage personnel, journal de machine ou adresse LAN
de l’auteur n’est inclus dans la distribution publique.

## Évolutions color+2 / color+3 et paquet v0.2.0

La palette color+2 a été signalée fonctionnelle par l’auteur le 2 octobre 2026.
Après préparation et installation de color+3, il a rapporté que l’ensemble
était en place et fonctionnel. C’est un retour global de l’auteur, sans relevé
publié de chaque étape du menu. Le protocole matériel détaillé reste à documenter.

| Contrôle supplémentaire color+3 | Résultat |
|---|---|
| Déclarations, vrai journal, correction des huit têtes, états périmés et coupures simulées | 14 cas, 3 856 assertions réussies |
| Palette visuelle et couleurs | 4 cas, 614 assertions réussies |
| JSON après correction de la tête 8 reçu par le client compilé sur loopback | Huit indices, matières et codes RGB exactement conservés |
| BBF principal, ressources et programmes secondaires | Checksums valides, secondaires identiques à l’officiel 6.9.1, aucun TLV bootloader 11/12 |

Ces suites recouvrent des tests historiques ; leurs comptes ne doivent pas
être additionnés pour annoncer un nouveau total global. Build color+3 :
FLASH 1 316 192 octets, RAM 124 056 octets, CCMRAM 62 148 octets.

Prusa Connect : l’auteur a confirmé le 2 octobre la connexion rétablie après
réactivation du polling et retrait du blocage du trousseau dans le lanceur.
Le 3 octobre, le raccourci officiel du Dock a été remplacé localement par un
lanceur du Slicer personnalisé. Le chemin du processus, son binaire inchangé
et ses options Connect ont été vérifiés. La présence du bouton a été vérifiée
dans le code, sans nouvelle observation instrumentée de l’écran.

Le paquet public v0.2.0 reprend le même exécutable et des réglages neufs, avec
deux lanceurs `.app` et `.command`, Connect activé et sans blocage du trousseau.
Ses contrôles d’archive, de chemins, de permissions, de profils et d’empreintes
sont dans `release-manifest.json`. Cela ne constitue pas une nouvelle campagne
du moteur ou une connexion automatisée à un compte réel.
Aucun réglage personnel ni identifiant n’est publié.

Les essais physiques détaillés sur les huit têtes, redémarrages, longues
impressions et le retour officiel restent à documenter. Le portage de la
palette MK4 et la version CORE One normale V1 sont en pause.

## Correctif du lanceur macOS — v0.2.1

Le 6 octobre 2026, l’auteur a rencontré une erreur de synchronisation malgré
une adresse locale correcte. Des GET sans authentification ont confirmé la
réponse HTTP locale de la machine ; les journaux système macOS montraient une
boucle d’autorisation et de réinitialisation du cache d’identité du Slicer.
Après remplacement du script `.app` par un exécutable principal natif qui
reste actif et lance le même Slicer avec ses réglages, l’auteur a confirmé
« La synchronisation fonctionne » sur sa CORE One INDX.

Le binaire Slicer, ses réglages et les deux BBF sont conservés. Les lanceurs
natifs déclarent `NSLocalNetworkUsageDescription` et ont un UUID propre à
chaque variante ; leurs bundles sont signés ad hoc. Deux essais via
LaunchServices avec des exécutables factices, dans des chemins contenant
des espaces, ont vérifié le parent natif, les arguments de réglages, l’option
single-instance et l’absence du blocage de trousseau. Les contrôles du paquet
v0.2.1 sont consignés dans `release-manifest.json`.

Cette confirmation est un essai utilisateur sur un Mac sous macOS 26.6.2,
sans capture de requête authentifiée ni lecture d’identifiants. Elle ne couvre
pas toutes les versions macOS, les mises à jour de signature ni une nouvelle
campagne physique MK4. Apple demande une identité de signature Apple pour un
suivi fiable de l’identité ; le projet ne dispose que de signatures ad hoc :
[TN3179](https://developer.apple.com/documentation/technotes/tn3179-understanding-local-network-privacy).
