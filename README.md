# Filaments locaux Prusa — firmware + PrusaSlicer personnalisé

Choisissez la couleur sur l’écran de l’imprimante, puis synchronisez
**matière et couleur dans PrusaSlicer** via **PrusaLink local**.
Sur INDX : palette de 60 nuances et correction directe par extrudeur.

**Il faut télécharger notre version complète de PrusaSlicer et installer le
firmware correspondant à sa machine. Ce n’est pas un plugin/add-on de l’officiel.**
Les fonctions habituelles restent disponibles, y compris Prusa Connect facultatif.
La synchronisation des filaments utilise PrusaLink local, sans dépendance au cloud.

**Vous consultez ici : CORE One + INDX.** Prototype communautaire expérimental de
Coben-3d, sans validation officielle Prusa.

## Choisir sa machine

| Machine | Firmware et fonctions | Guide | Branche sources |
|---|---|---|---|
| MK4, une bobine, sans MMU | 6.5.7-color+4 ; couleurs nommées au chargement | [MK4](doc/INSTALL-MK4.md) | [MK4](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/mk4-loaded-filament-color) |
| CORE One / CORE One+ équipée d’INDX 8 têtes | 6.9.1-color+3 ; 60 nuances et correction sans recharge | [INDX](doc/INSTALL-COREONE-INDX.md) | [CORE One INDX](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/coreone-indx-loaded-filament-colors) |
| CORE One normale V1, sans INDX | Portage en pause ; aucun BBF fourni | — | — |

Deux branches firmware distinctes dans le même dépôt, un [Slicer commun](https://github.com/Coben-3d/PrusaSlicer/tree/feature/indx-local-filaments)
compatible avec les deux. La palette INDX n’est pas encore portée sur MK4.

- **[Téléchargements v0.2.1](https://github.com/Coben-3d/Prusa-Firmware-Buddy/releases/tag/local-filaments-v0.2.1)** : BBF de sa machine + ZIP Slicer.
- **[Guide complet](doc/LOCAL-FILAMENTS.md)** : installation, lanceurs/Dock,
  PrusaLink et Connect, profils, dépannage et retour officiel.
- [Tests et limites](doc/LOCAL-FILAMENTS-VALIDATION.md).
- [Sources PrusaSlicer personnalisées](https://github.com/Coben-3d/PrusaSlicer/tree/feature/indx-local-filaments). Les sources GitHub ne sont pas le binaire.

Binaire Slicer : **Apple Silicon et macOS 26.2 minimum**. Windows, Linux et Mac
Intel : sources disponibles, aucun binaire ou validation fourni ici.
Les BBF non signés nécessitent la rupture **irréversible** de la languette
xBuddy `!` : lire [les conditions Prusa](https://help.prusa3d.com/article/flashing-custom-firmware-core-one-l-core-one-mk4-s-mk3-9-s-mk3-5-s_814967)
et le guide. Les BBF de machines différentes ne sont pas interchangeables.

**English:** community custom firmware plus a full customized PrusaSlicer
download, not a plugin. Local material/color sync over PrusaLink. Separate MK4
and CORE One INDX firmware branches, one shared Slicer, optional Prusa Connect.
macOS binary requires Apple Silicon and macOS 26.2+. Standard CORE One without
INDX is not supported yet. Read the installation guide first.

---

# Buddy
This repository includes source code and firmware releases for the Original Prusa 3D printers based on the 32-bit ARM microcontrollers.

The currently supported models are:
- Original Prusa MINI/MINI+
- Original Prusa MK3.5
- Original Prusa MK3.9
- Original Prusa MK4
- Original Prusa XL
- Prusa CORE One

## Getting Started

### Requirements

- Python 3.8 or newer
- system installation of Python's `requests` package (use either pip or your system package manager)

### Cloning this repository

Run `git clone https://github.com/prusa3d/Prusa-Firmware-Buddy.git`.

### Building (on all platforms, without an IDE)

Run `python utils/build.py`. The binaries are then going to be stored under `./build/products`.

- Without any arguments, it will build a release version of the firmware for all supported printers and bootloader settings.
- Use `--build-type` to select build configurations to be built (`debug`, `release`).
- Use `--preset` to select for which printers the firmware should be built.
- By default, it will build the firmware in "prerelease mode" set to `beta`. You can change the prerelease using `--prerelease alpha`, or use `--final` to build a final version of the firmware.
- Use `--host-tools` to include host tools in the build
- Find more options using the `--help` flag!

#### Examples:

Build the firmware for MINI and XL in `debug` mode:

```bash
python utils/build.py --preset mini,xl --build-type debug
```

Build the firmware for MINI using a custom version of gcc-arm-none-eabi (available in `$PATH`) and use `Make` instead of `Ninja` (not recommended):

```bash
python utils/build.py --preset mini --toolchain cmake/AnyGccArmNoneEabi.cmake --generator 'Unix Makefiles'
```

#### Windows 10 troubleshooting

If you have python installed and in your PATH but still getting cmake error `Python3 not found.` Try running python and python3 from cmd. If one of it opens Microsoft Store instead of either opening python interpreter or complaining `'python3' is not recognized as an internal or external command,
operable program or batch file.` Open `manage app execution aliases` and disable `App Installer` association with `python.exe` and `python3.exe`.

### Development

The build process of this project is driven by CMake and `build.py` is just a high-level wrapper around it. As most modern IDEs support some kind of CMake integration, it should be possible to use almost any editor for development. Below are some documents describing how to setup some popular text editors.

- [Visual Studio Code](doc/editor/vscode.md)
- [Vim](doc/editor/vim.md)
- [Eclipse, STM32CubeIDE](doc/editor/stm32cubeide.md)
- [Other LSP-based IDEs (Atom, Sublime Text, ...)](doc/editor/lsp-based-ides.md)

#### Contributing

If you want to contribute to the codebase, please read the [Contribution Guidelines](doc/contributing.md).

#### XL and Puppies

With the XL, the situation gets a bit more complex. The firmware of XLBuddy contains firmwares for the puppies (Dwarf and Modularbed) to flash them when necessary. We support several ways of dealing with those firmwares when developing:

1. Build Dwarf/Modularbed firmware automatically and flash it on startup by XLBuddy (the default)
    - The Dwarf & ModularBed firmware will be built from this repo.
    - The puppies are going to be flashed on startup by the XLBuddy. The puppies have to be running the [Puppy Bootloader](http://github.com/prusa3d/Prusa-Bootloader-Puppy).

2. Use pre-built Dwarf/Modularbed firmware and flash it on startup by xlBuddy
    - Specify the location of the .bin file with `DWARF_BINARY_PATH`/`MODULARBED_BINARY_PATH`.
    - For example
    ```
    cmake .. --preset xl_release_boot -DDWARF_BINARY_PATH=/Downloads/dwarf-4.4.0-boot.bin
    ```

3. Do not include any puppy firmware, and do not flash the puppies by XLBuddy.
    ```
    -DENABLE_PUPPY_BOOTLOAD=NO
    ```
    - With the `ENABLE_PUPPY_BOOTLOAD` set to false, the project will disable Puppy flashing & interaction with Puppy bootloaders.
    - It is up to you to flash the correct firmware to the puppies (noboot variant).

5. Keep bootloaders but do not write firmware on boot.
    ```
    -DPUPPY_SKIP_FLASH_FW=YES
    ```
    - With the `PUPPY_SKIP_FLASH_FW` set to true, the project will disable Puppy flashing on boot.
    - You can keep other puppies that are not debugged in the same state as before.
    - Use puppy build config with bootloaders (e.g. `xl-dwarf_debug_boot`) on one or more puppies.
    - Recommend breakpoint at the end of `puppy_task_body()` to prevent buddy from resetting the puppy immediately when puppy stops on breakpoint.

See /ProjectOptions.cmake for more information about those cache variables.

#### Running tests
See the detailed testing guide in our [comprehensive testing guide].

[comprehensive testing guide]: tests/unit/README.md

## Flashing Custom Firmware

To install custom firmware, you have to break the appendix on the board. Learn how to in the following article https://help.prusa3d.com/article/zoiw36imrs-flashing-custom-firmware.

## Feedback

- [Feature Requests from Community](https://github.com/prusa3d/Prusa-Firmware-Buddy/labels/feature%20request)

## Credits

- [Marlin](https://marlinfw.org/) - 3D printing core driver
- [Klipper](https://www.klipper3d.org/) - input shaper code based on Klipper

## License

The firmware source code is licensed under the GNU General Public License v3.0 and the graphics and design are licensed under Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0). Fonts are licensed under different license (see [LICENSE](LICENSE.md)).
