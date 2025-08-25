Dropdowns Implementation Overview

Scope
- Component: `DropdownMenu` (UI)
- Pages using dropdowns: Main (LFO selector, quick FX), Effects (per-slot Effect Type + Page), Sensors (per-lane Slot, FX name, Param)

Core behaviors
- Population:
  - Add items via `addItem`/`addItems` at creation or on-demand.
  - Always ensure a visible selection using `selectItemSilently(0)` after populating so the control never appears blank.
  - When a dependent control changes (e.g., Sensors lane Slot), repopulate the dependent dropdown (Param) immediately and preselect item 0.

- Rendering (z-order):
  - Each dropdown renders its closed state in the normal component pass (`DropdownMenu::render`).
  - The open list is drawn later using `DropdownMenu::renderDropdownList(DisplayManager*)` so it appears above other UI elements.
  - The app’s main loop explicitly renders open lists after the active screen’s render:
    - Effects: iterate `fx_type_*`, `fx_page_*`, plus main quick `effect_type_*` and LFO/preset/MIDI dropdowns.
    - Sensors: iterate `sens_slot_*` and `sens_param_*` (for lanes 0..7).

- Input handling (click-through prevention):
  - In the event loop, before handling other controls, check if any dropdown is open on the active screen and pass the input to it.
  - Process in reverse order (top-most first) and, if a TouchPress isn’t handled by the dropdown, intentionally consume it to avoid opening/activating controls underneath the open list.
  - This pattern is implemented for Effects dropdowns and mirrored for Sensors (`sens_slot_*`, `sens_param_*`).

Sensors specifics
- Per-lane controls under each lane (i=0..7):
  - `sens_slot_i`: target slot selection (None, S1..S5)
  - `sens_fx_i`: displays the effect name currently loaded in the selected slot (read-only visual; single-item list)
  - `sens_param_i`: auto-populated list based on `sens_fx_i` effect type (e.g., Tremolo: depth/rate/shape/stereo_phase_deg)
  - After changing `sens_slot_i`, we update `sens_fx_i` and repopulate `sens_param_i`, then preselect the first param.

Effects specifics
- Per-slot Controls (s=0..fxSlotCount-1):
  - `fx_type_s`: populated with `"None" + getAvailableEffects()` once at build time; selection triggers chain rebuild + per-slot UI reconfiguration.
  - `fx_page_s`: populated with Page 1/2; selection switches parameter pages for multi-page effects.
  - Main quick FX `effect_type_0..2` proxy into the Effects page dropdowns and mirror selection both ways.

Common pitfalls avoided
- Blank lists: After repopulating, always `selectItemSilently(0)` and optionally store the selected string to avoid empty displays.
- Hidden lists: Always call `renderDropdownList` after the active screen render, for all open dropdowns on that screen.
- Click-through: In the input loop, handle open dropdowns first and consume the event when a click occurs in (or just outside) the open list bounds.

Code reference (key places)
- UI component: `src/ui/DropdownMenu.cpp` (render, renderDropdownList, handleInput)
- Effects population and z-ordering: `src/main_integrated_simple.cpp` (look for `fx_type_`, `fx_page_`, `renderDropdownList` calls)
- Sensors population and z-ordering: `src/main_integrated_simple.cpp` (look for `sens_slot_`, `sens_fx_`, `sens_param_` setup and list rendering)

Notes
- Text rendering uses SDL_ttf when `HAVE_SDL_TTF` is defined; the integrated app target defines this flag to ensure dropdown text is visible.
- If runtime environments lack SDL_ttf, consider adding a fallback text routine (e.g., simple bitmap font) or verify dylib search paths.

