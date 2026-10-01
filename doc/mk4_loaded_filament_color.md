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
declaration, even when the new material is identical. Only a successful end of
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
confirmation writes. The initial suite passes 460 assertions in 7 cases. These do not demonstrate
GUI navigation, HTTP authentication dispatch or complete Marlin integration.

Before a machine trial, finish this matrix using a suitable simulator/test
environment and then the authorized machine:

| Case | Expected result | Current status |
| --- | --- | --- |
| Normal load with each palette color | Color survives success and reboot | Host encoding/journal passed; GUI/Marlin pending |
| Back from palette | Pending choice unchanged | Existing dialog behavior; GUI pending |
| Back from preheat | Prior confirmed color untouched | Journal separation passed; integration pending |
| Stop during load / runout / purge retry | No premature confirmed color | Integration pending |
| Purge-only of an existing spool | Confirmed color preserved | Integration pending |
| Successful unload / sensor removal | Color unknown, including after reboot | Journal invalidation passed; integration pending |
| Same-material spool replacement | Old color cleared before load, new one committed only on success | Integration pending |
| Autoload and Custom material confirm | Palette choice retained after material response | Integration pending |
| M600 with/without C | Provided RGB / unknown on successful completion | Integration pending |
| GET with existing auth, invalid/no auth | 200 / existing authentication rejection | Real HTTP dispatch pending |
| POST/PUT/DELETE/HEAD, MMU, other models | 405 / 404 as specified | Real HTTP dispatch/build matrix pending |
| Official firmware return then custom again | Existing printer settings preserved; reload spool declaration | Hardware/rollback pending |

No ARM firmware build or machine test has yet been completed. Downloading the
official toolchain/dependencies is awaiting separate authorization. The build
helper's project-local `.dependencies` and `.venv`, build outputs, TMPDIR and
caches must remain on the external SSD. No new APFS volume is needed for the
host tests; their paths containing spaces have been exercised. Full upstream
formatting tools and cross compilation still need verification.

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
