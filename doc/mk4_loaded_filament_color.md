# MK4 : déclaration locale de matière et couleur

Pour installer et utiliser le prototype, voir [le guide MK4](INSTALL-MK4.md) et [le guide commun](LOCAL-FILAMENTS.md).
Base officielle 6.5.7 : `7119a302d6d0bc144c57b631d778656c8a2745f6`.
Version du prototype : **6.5.7-color+4**, MK4 mono-bobine, sans MMU actif.
Le flux écran → PrusaLink → Slicer a été confirmé par l’auteur sur sa machine.
Voir [la validation et ses limites](LOCAL-FILAMENTS-VALIDATION.md).

## Interaction et persistance

Dans le menu de chargement, choisir Couleur du filament avant la matière.
La palette comprend inconnu, noir, blanc, gris, rouge, orange, jaune, vert,
bleu, violet, marron et rose. Le violet de la palette est `#800080`.
Le choix reste temporaire tant que le chargement n’est pas terminé avec succès.
Annuler le préchauffage conserve la déclaration précédente.

La clé `MK4 Loaded Filament Color v1` associe matière et RGB dans une valeur
64 bits : RGB aux bits 0–23, connu au bit 24, matière encodée aux bits 32–39.
Inconnu et noir sont distincts. Les nouveaux chargements complets et changements
invalident la couleur avant le stationnement ; seule la fin réussie la confirme.
Déchargement, retrait détecté et autre matière l’invalident. Une purge seule et
le chargement limité aux engrenages ne remplacent pas la déclaration confirmée.
Le journal ne change pas de version ni de clés de calibration.

M600 garde son argument `C` existant lorsqu’il est présent. Il n’y a pas de
nouveau menu de couleur dans le dialogue M600 pendant une impression.

## API locale et Slicer

`GET /api/v1/filaments`, sous l’authentification PrusaLink existante :

```json
{
  "schema_version": 1,
  "slots": [
    {"slot": 0, "material": "PLA", "color": "#800080", "source": "user_declared"}
  ]
}
```

Matière absente ou couleur inconnue : `null`. Le renderer possède l’instantané
du couple matière/couleur pendant toute la réponse. Autres méthodes
authentifiées : 405 ; autres cibles et MMU actif : route indisponible.

Le Slicer publié accepte ce protocole v1 et le protocole INDX v2. Il sélectionne
un profil matière compatible, ou conserve celui qui l’est déjà, et applique
la couleur à `extruder_colour`. Une nouvelle matière modifie normalement les
paramètres du profil, dont les températures. Les annulations, erreurs et réponses
tardives conservent les réglages. La couleur peut être conservée dans un projet 3MF.

## Développement

Les composants principaux sont `loaded_filament_color.hpp`, `filament_to_load.*`,
le config store, les menus de préchauffage, le code Pause et le renderer PrusaLink.
Les tests hôte sont lancés avec `python3 utils/run_loaded_filament_color_tests.py`
en fournissant un dossier de compilation. Le firmware virtuel utilise
`tests/integration/test_loaded_filament_color.py`.
La variante distribuée utilise le mode bootloader vide et un BBF non signé.
Les instructions générales de compilation amont restent dans le README.

La release v0.2.0 réutilise ce BBF color+4 inchangé. La palette de nuances
et le menu de correction directe INDX ne sont pas encore portés sur MK4.
Le Slicer est une application personnalisée complète, commune aux deux machines.
