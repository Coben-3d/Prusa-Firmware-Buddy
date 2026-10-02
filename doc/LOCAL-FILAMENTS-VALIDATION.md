# Validation du prototype v0.1.0

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

## Contrôles logiciels terminés

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
