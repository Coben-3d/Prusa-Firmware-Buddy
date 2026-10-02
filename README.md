# Filaments locaux Prusa — MK4 et CORE One + INDX

> Évolution en préparation : [palette visuelle de 60 nuances](doc/INDX-COLOR-PALETTE.md).
> Palette `color+2` : fonctionnement confirmé par Benjamin sur sa machine le 2 octobre 2026.
> Nouvelle [correction de couleur sans recharger](doc/INDX-FILAMENT-COLOR-EDIT.md) `color+3` : essai matériel restant.
> Les téléchargements v0.1.0 conservent leur menu d’origine.

Choisissez la couleur au chargement sur l’écran de l’imprimante, puis
**synchronisez matière et couleur dans PrusaSlicer** par PrusaLink local.
Pour INDX, la synchronisation porte sur les huit emplacements physiques.
Prusa Connect n’est pas nécessaire.

**Prototype communautaire expérimental de Coben-3d.** L’auteur a confirmé le
fonctionnement sur ses machines le 2 octobre 2026 ; le périmètre des contrôles
et les limites sont documentés. Ce fork n’est pas une version officielle Prusa.

- **[Téléchargements v0.1.0](https://github.com/Coben-3d/Prusa-Firmware-Buddy/releases/tag/local-filaments-v0.1.0)**
- **[Installer et utiliser](doc/LOCAL-FILAMENTS.md)**
- [Tests et limites](doc/LOCAL-FILAMENTS-VALIDATION.md)
- [Firmware MK4](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/mk4-loaded-filament-color) · [firmware CORE One INDX](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/coreone-indx-loaded-filament-colors)
- [PrusaSlicer modifié et ses sources](https://github.com/Coben-3d/PrusaSlicer/tree/feature/indx-local-filaments)

| Version | Périmètre |
|---|---|
| MK4 6.5.7-color+4 | Une bobine, sans MMU actif |
| CORE One INDX 6.9.1-color+1 | Huit têtes, profil HF0.4 de départ |
| PrusaSlicer 2.9.6+FilamentLocal-INDX | MK4 et INDX ; paquet Apple Silicon, macOS 26.2 minimum |

Le firmware personnalisé non signé demande la rupture irréversible de la
languette xBuddy `!`. Lire les [conditions Prusa](https://help.prusa3d.com/article/flashing-custom-firmware-core-one-l-core-one-mk4-s-mk3-9-s-mk3-5-s_814967)
et le guide avant installation. Les profils, versions et BBF doivent correspondre
à votre machine.

**English:** community prototype that stores a user-selected filament color on
the printer and syncs loaded material and color into PrusaSlicer over local
PrusaLink. Supports single-spool MK4 and CORE One INDX 8-tool. Downloaded macOS
binaries require Apple Silicon and macOS 26.2+. Custom unsigned firmware needs
the irreversible xBuddy appendix removal. Read the installation guide first.

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
