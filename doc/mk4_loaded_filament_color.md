# MK4 loaded filament color prototype

This development branch starts at official MK4 firmware v6.5.7,
`7119a302d6d0bc144c57b631d778656c8a2745f6`. It is a prototype for the
single-spool MK4 without an active MMU. It has not been flashed or tested on a
printer. INDX and multi-tool behavior are outside this patch.

## User interaction

The existing material preheat menu gains a **Filament Color** item above its
material list during Load, Autoload, and Change_phase2. Open this item and choose
Unknown, Black, White, Gray, Red, Orange, Yellow, Green, Blue, Purple, Brown, or
Pink, then choose the material as usual. The default for a new operation is
Unknown. Returning from the palette without a selection preserves the pending
choice; returning from the preheat menu cancels the operation.

The palette uses the existing modal select-menu implementation and the new
strings use the translation extraction convention `N_`/`_`. Translation
catalog updates and visual review of all languages remain to be done.

The material presets and their heating parameters are not changed. The color
is a user declaration, not a physical measurement or spool inventory identifier.
For transparent, blended, silk or multicolor filament, choose a representative
palette color or Unknown. An existing G-code RGB outside the palette is shown
as its hex value and preserved; the palette does not provide an RGB editor.

## State and persistence

The existing requested purge-display color becomes an atomic 32-bit value for
communication between the GUI and Marlin tasks. It remains provisional RAM
state. Unknown and black are distinct.

The new journal item `MK4 Loaded Filament Color v1` is a uint64_t:

| Bits | Meaning |
| --- | --- |
| 0–23 | RGB, 0xRRGGBB |
| 24 | Color is known |
| 25–31 | Reserved, zero |
| 32–39 | Existing EncodedFilamentType material tag |
| 40–63 | Reserved, zero |

Zero means there is no confirmed declaration. A completed load with an unknown
color still records its material tag. The material and color share one journal
item so an HTTP response cannot pair a confirmed color with another material
halfway through an unload or load. The HTTP renderer copies its state once
and retains it for every JSON chunk.

Preheat and palette selection do not write this item. Starting a full load,
autoload, M600 change or stuck-filament replacement invalidates the old
declaration before the FSM holder starts parking, even when the new material
is identical. Only a successful end of
the Pause load loop commits the pending color. Stop, failed load, or interruption
therefore leave the color unknown. A purge-only operation and load-to-gears do
not replace a confirmed color. Unload, sensor-confirmed removal, and a different
material set through the config store invalidate it. The existing journal avoids
writing an unchanged item.

An initial installation has no item and starts unknown without changing the
schema version or existing keys. The official hash generator checks the new
key for collisions. A normal reboot with filament present retains a confirmed
declaration; the existing sensor logic clears it if filament is certainly absent.
No automatic factory reset is added.

M600 uses its existing `C` argument as the pending RGB when provided; without
it the declaration is unknown. This patch does not add a color menu to the
in-print M600 dialog. Its menu addition covers the existing material-choice
preheat screens for manual load, autoload, and menu filament change.

## Local PrusaLink contract

New endpoint: **GET /api/v1/filaments**.

It is inside the existing authenticated `/api/v1/` selector. It uses the same
PrusaLink authentication as the other endpoints and does not depend on Prusa
Connect. It returns 404 on other printer builds or when the MK4 MMU is active.
It only accepts GET; other authenticated methods return 405 through `get_only`.
It adds no write endpoint and no new credentials.

```json
{
  "schema_version": 1,
  "slots": [
    {
      "slot": 0,
      "material": "PLA",
      "color": "#F8651B",
      "source": "user_declared"
    }
  ]
}
```

- `slot` is a zero-based physical mono-spool slot, not a G-code extruder mapping.
- `material` is the firmware material name or null when absent/unknown.
- `color` is `#RRGGBB` or null. Null must never be interpreted as black.
- `source` describes the declaration mechanism, including unknown color.
- A request during loading may show an existing material with a null color.
- Values represent the captured request snapshot; another request may differ.
- Future slot extensions must retain explicit slot identifiers. INDX mapping
  requires separate design and tests; it is not claimed compatible here.

There is no automatic propagation of this custom field into Connect. This
prototype does not call mobile APIs or depend on cloud material catalogs.

## PrusaSlicer follow-up (not implemented)

Use the configured physical printer's existing PrusaLink connection and auth.
Add an asynchronous Refresh filament colors button below the material/color
list. On click, GET the endpoint, require a supported schema version and slot 0,
validate the RGB format, and update only the current project/tool color through
the existing project configuration and undo mechanisms. Keep the selected
filament preset, extrusion and temperature settings unchanged. A differing
material name can be shown as information rather than automatically selecting
a preset. Unknown color preserves the project's current color with a visible
Unknown state; offline, 401, 404 and malformed JSON leave the project unchanged.

PrusaSlicer 3.x follow-up locations are
`src/slic3r-shared/src/Slic3r/App/SidebarBed.cpp` (MaterialListView) and
`src/slic3r-shared/src/Slic3r/Biz/PrintHost/PrintHostPrusaLink.cpp` (existing auth).
The development API/color path must be rechecked against its chosen commit.
No second fork or Slicer change is part of this firmware branch.

## Validation

The small host suite uses existing clang++ and the vendored Catch2 without
bootstrap or downloads. It compiles the production color, atomic pending
state, JSON renderer, and EEPROM journal backend. Run from the repository:

```sh
python3 utils/run_loaded_filament_color_tests.py \
  --build-dir '/Volumes/JAUNE - SAVE BEN/PrusaDev/build/loaded-color-tests'
```

The same test source is registered with the repository's native unit tests.
This helper creates two generated option headers only in the test build
directory, sets TMPDIR there, and passes source paths as subprocess arguments
so spaces in the SSD path are preserved.

Executed host cases: RGB/unknown encoding, pending selection/reset, concurrent
GUI/Marlin reads, journal reload and invalidation, JSON nulls/escaping, snapshot
ownership, every chunk size from 32 to 256 bytes, and simulated interrupted
confirmation writes. The suite passes 460 assertions in 7 cases after formatting. The native CMake
target also compiles and all seven discovered CTest cases pass. A missing
generated-options include path in this target was corrected during validation.
These host tests do not demonstrate GUI navigation or complete Marlin
integration. Running-firmware tests now cover authentication and method dispatch,
black/white/unknown declarations through process restarts, and LCD navigation.
The palette/back/cancel case starts with a confirmed blue PLA record, chooses
Red provisionally, returns from the palette and cancels preheat. The API must
still report the original blue declaration. Captures are retained locally.

Before a machine trial, finish this matrix using a suitable simulator/test
environment and then the authorized machine:

| Case | Expected result | Current status |
| --- | --- | --- |
| Normal load with each palette color | Color survives success and reboot | Host encoding/journal and Red palette passed; full success pending |
| Back from palette | Pending choice unchanged | Passed in Mini404 with an existing blue PLA record |
| Back from preheat | Prior confirmed color untouched | Passed in Mini404; original blue record preserved |
| Early load interruption / runout / purge retry | No premature confirmed color | Interruption during Parking and reboot passed; other phases pending |
| Purge-only of an existing spool | Confirmed color preserved | Integration pending |
| Successful unload / sensor removal | Color unknown, including after reboot | Journal invalidation passed; integration pending |
| Same-material spool replacement | Old color cleared before motion, new one committed only on success | Start of PLA-to-PLA replacement and interrupted reboot passed; success pending |
| Autoload and Custom material confirm | Palette choice retained after material response | Integration pending |
| M600 with/without C | Provided RGB / unknown on successful completion | Integration pending |
| GET with existing auth, invalid/no auth | 200 / existing authentication rejection | Passed against running MK4 firmware in Mini404 |
| POST/PUT/DELETE/HEAD, MMU, other models | 405 / 404 as specified | 405 passed in Mini404; MMU/other model builds pending |
| Official firmware return then custom again | Existing printer settings preserved; reload spool declaration | Hardware/rollback pending |

### Running-firmware integration checks

`tests/integration/test_loaded_filament_color.py` contains six verified cases:
HTTP auth/methods, black/white/unknown declarations with restarts, palette/back/
cancel preserving an existing color, and same-material load interruption. The
interruption case found that the old invalidation followed the FSM holder
constructor, which parks the nozzle. Invalidation now happens before that
constructor, and the running API confirms an unknown color already at Parking
and after restarting with the same EEPROM image.

The seventh case, completed GUI load through the final Yes confirmation and
reboot, requires `--complete-filament-load`. It is skipped by default and is
**not validated**. With Mini404 v0.9.10 it times out during Parking, including
with a longer 180-second allowance and another virtual-clock setting. The
current official Mini404 revision
[`3b8be69`](https://github.com/vintagepc/MINI404/tree/3b8be69b2c02c641b778b3754b78291186f3f71a)
was also compiled (QEMU 11) but did not complete the boot fixture. Its different
console/EEPROM conventions and host build-script issues prevent claiming full
load coverage. No fan check, heater protection or motion safety was disabled
to force this case through. The standard upstream fixture disables filament
sensors; sensor-enabled autoload/runout tests therefore remain separate work.

For the verified simulator, apply
[`mini404_mk4_v0_9_10_compat.patch`](mini404_mk4_v0_9_10_compat.patch) to official
Mini404 v0.9.10 sources before its native build. The local runner provides the
SSD library search paths. Tests use a dedicated Python 3.12 environment with
EasyOCR 1.7.2, pytest 7.3.2 and pytest-asyncio 0.21.2; models and caches stay on
that SSD. Run with the separately built **noboot** firmware:

```sh
BUDDY_NO_VIRTUALENV=1 BUDDY_TEST_OCR_DEVICE=cpu \
EASYOCR_MODULE_PATH='/Volumes/JAUNE - SAVE BEN/PrusaDev/cache/easyocr' \
TORCH_HOME='/Volumes/JAUNE - SAVE BEN/PrusaDev/cache/torch' \
PYTHONPYCACHEPREFIX='/Volumes/JAUNE - SAVE BEN/PrusaDev/cache/pycache' \
'/Volumes/JAUNE - SAVE BEN/PrusaDev/tools/simulator-test-venv/bin/python' \
  -m pytest tests/integration/test_loaded_filament_color.py \
  --firmware '/Volumes/JAUNE - SAVE BEN/PrusaDev/build/firmware-sim/mk4_release_noboot/firmware.bin' \
  --simulator '/Volumes/JAUNE - SAVE BEN/PrusaDev/tools/run-mini404-native.sh'
```

The simulator's HTTP port binds only to 127.0.0.1. The newer console can be
selected with `--simulator-no-greeting`; this option does not imply that a
particular newer machine model passed the suite. All password values used here
are deterministic simulated fixtures, not credentials created for a printer.

### Completed MK4 cross compilation on Apple Silicon

On 2026-10-01, the complete Release MK4 build succeeded with the official
Arm GNU Toolchain 13.3.Rel1, GCC 13.3.1 20240614, and the `empty` bootloader
preset. The `.bin`, `.bbf`, ELF and map were generated locally. The BBF has an
all-zero signature; compilation does not grant permission to flash it.

The repository bootstrap selects the Intel macOS toolchain, whose compiler
requires a missing `/usr/local/opt/zstd/lib/libzstd.1.dylib` on this Mac. The
[official native Apple Silicon archive](https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi.tar.xz)
was installed separately under the external SSD's `PrusaDev/tools`. No global
library installation or compiler binary modification was needed. A local CMake
toolchain file points `ARM_TOOLCHAIN_DIR` to the archive root and
`RECOMMENDED_TOOLCHAIN_BINUTILS` to its `bin` directory, then includes
`cmake/AnyGccArmNoneEabi.cmake`.

GCC's parallel LTO subprocess makefile does not handle this SSD path's spaces.
A no-space temporary-directory symlink pointing physically to the SSD fixes
temporary filenames, and `-flto=1` fixes its unquoted compiler command. LTO
remains enabled with the original Release optimization settings; only its
parallelism changes. No new APFS volume or repartition was used.

The successful command, from this repository, was:

```sh
# This alias contains no build data; its target is the external SSD's tmp directory.
# Create it once if absent. Do not replace an existing unrelated /tmp entry.
ln -s '/Volumes/JAUNE - SAVE BEN/PrusaDev/tmp' /tmp/prusadev-mk4-tmp-20261001

env TMPDIR=/tmp/prusadev-mk4-tmp-20261001 \
  PIP_CACHE_DIR='/Volumes/JAUNE - SAVE BEN/PrusaDev/cache/pip' \
  PYTHONPYCACHEPREFIX='/Volumes/JAUNE - SAVE BEN/PrusaDev/cache/pycache' \
  CMAKE_BUILD_PARALLEL_LEVEL=3 \
  .venv/bin/python utils/build.py \
    --preset mk4 --build-type release --bootloader empty --skip-bootstrap \
    --toolchain '/Volumes/JAUNE - SAVE BEN/PrusaDev/tools/mk4-native-toolchain.cmake' \
    --version-suffix=-color-prototype --version-suffix-short=-color \
    --build-dir '/Volumes/JAUNE - SAVE BEN/PrusaDev/build/firmware-native' \
    --products-dir '/Volumes/JAUNE - SAVE BEN/PrusaDev/artifacts/firmware' \
    '-DCMAKE_EXE_LINKER_FLAGS:STRING=-flto=1'
```

The environment uses project-local Python 3.12.12, CMake 3.28.3 and Ninja
1.10.2. Firmware build dependencies remain in the project environment. The EasyOCR
stack is isolated in a separate simulator-test environment on the external SSD. Formatting was applied
with the repository's pinned clang-format 16, cmake-format 0.6.13 and yapf
0.40.2. The formatter itself uses a locally built Intel zstd library through
DYLD_LIBRARY_PATH; nothing was installed in Homebrew or system directories.

Previous linker report for commit `623b17b`, before the integration fixes:

| Region | Used | Available | Usage |
| --- | --- | --- | --- |
| FLASH | 1,881,492 B | 1,919 KiB | 95.75% |
| RAM | 114,656 B | 196,508 B | 58.35% |
| CCMRAM | 61,276 B | 64 KiB | 93.50% |

This is an absolute link-time footprint, not a measured difference against a
fresh official build. Runtime stack headroom and GUI timing remain unmeasured.
That earlier validation build reports `6.5.7-color+3`; generated product
filenames use `mk4_release_emptyboot_6.5.7-color-prototype`. This development
build number comes from the shallow local Git history, not Prusa's official
release build counter, and increases with subsequent commits. The external
`artifacts/mk4-color-validation.json` records the final commit, product hashes
and current linker footprint without requiring another documentation-only
rebuild cycle.

### Remaining execution limits

The native CMake test build required a local host-only CMake hook replacing
the upstream GNU linker option `--gc-sections` with Apple ld's `-dead_strip`.
This hook is outside the repository and is not used for firmware compilation.
The larger upstream `nhttp_tests` target remains blocked by Apple Clang rejecting
existing `constexpr strlen(...)` expressions in `src/common/e2ee/e2ee.hpp`.
That unrelated code was not changed. The color renderer has been exercised
through production sources, and the new URL has also been dispatched over
a running firmware HTTP server in Mini404.

Mini404 v0.9.10 was rebuilt natively for Apple Silicon with external-SSD
GLib, pixman, libpng, libslirp and libfdt libraries. The downloaded Intel binary
was not made runnable through global Homebrew installs. Local compatibility
work used the official LCD 3-bit-color fix
[`2d476c4`](https://github.com/vintagepc/MINI404/commit/2d476c4f50847183bda68b834ac5747d9b4a4ac6),
BOM ID 38 and the current MK4 tachometer multiplexing design. This is simulator
board data and wiring, not a change to the printer's firmware protections.
The old simulator still stalls at Parking in the full-load test. The current
revision also has unvalidated fixture compatibility, as recorded above.

The running HTTP test found that the existing shared StatusPage/get_only path
sent a body for HEAD. It now suppresses the body while retaining the ordinary
response length header. Authenticated HEAD returns 405; unauthenticated HEAD
returns 401 without a body. Non-HEAD method rejection keeps its existing forced
connection-close behavior. Test fixtures now include a NUL in the simulated
PrusaLink password and merge EEPROM data without mutating global defaults.
The test runner drains QEMU stdout continuously so device trace output cannot
block execution between script commands. OCR can be explicitly run on CPU via
BUDDY_TEST_OCR_DEVICE=cpu. HTTP readiness is awaited separately from GUI boot.

All dependencies, caches, temporary build data and products remain under
`PrusaDev` on the external SSD. The `/tmp` aliases are symlinks only. There
was no printer connection, credential creation, flash, physical seal change,
upstream PR or contact with Prusa.

## Machine boundary and rollback

Do not flash this branch as part of development validation. Prusa's factory
MK4 xBuddy only accepts Prusa-signed firmware. Their documented custom-firmware
procedure requires breaking the physical appendix seal; that modification is
irreversible. No seal has been broken and no printer has been connected here.

[Prusa custom firmware instructions](https://help.prusa3d.com/article/flashing-custom-firmware-core-one-l-core-one-mk4-s-mk3-9-s-mk3-5-s_814967)
state that breaking the appendix does not itself void the warranty, while
damage caused by modifications/custom firmware is not blanket-covered.
There is no change to thermal, motion, extrusion safety or heater protections
in this patch. The added metadata writes still need timing/regression checks.

Official software can be restored using Prusa's documented
[firmware update](https://help.prusa3d.com/article/how-to-update-firmware-core-one-l-core-one-core-one-indx-mk4-s-mk3-9-s-mk3-5-s-xl_453086)
or [downgrade](https://help.prusa3d.com/article/how-to-downgrade-firmware-core-one-mk4-s-mk3-9-s-mk3-5-s-xl_725930)
procedure. Restoring it does not restore a broken physical seal. The official
firmware does not know the custom journal key and may retain or discard it
during journal compaction. After any period running official firmware, perform
a new full spool load/declaration before trusting color telemetry from this
branch; a same-material spool swap under official firmware cannot be inferred
from an old custom record. Verify rollback and existing calibration settings
before any later trial; no factory reset is requested by this patch.

## Contribution scope

The branch is published only on the user's fork. No upstream PR or contact is
authorized or performed. Before proposing a contribution, port against the
then-current MK4 branch, run official formatting and build/test matrices,
resolve translation catalogs and agree the local API schema with maintainers.
Acceptance by Prusa is not assumed.
