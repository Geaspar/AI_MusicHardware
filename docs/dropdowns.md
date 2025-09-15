Dropdowns Implementation Overview

Scope
- Component: `DropdownMenu` (UI)
- Pages using dropdowns: Main (LFO selector, quick FX), Effects (per-slot Effect Type + Page), Sensors (per-lane Slot, FX name, Param), Settings (Envelope)

Core behaviors
- Population:
  - Add items via `addItem`/`addItems` at creation or on-demand.
  - Always ensure a visible selection using `selectItemSilently(0)` after populating so the control never appears blank.
  - When a dependent control changes (e.g., Sensors lane Slot), repopulate the dependent dropdown (Param) immediately and preselect item 0.

- Rendering (z-order):
  - Each dropdown renders its closed state in the normal component pass (`DropdownMenu::render`).
  - The open list is drawn later using `DropdownMenu::renderDropdownList(DisplayManager*)` so it appears above other UI elements.
  - The app’s main loop explicitly renders open lists after the active screen’s render. Coverage includes:
    - Effects: iterate `fx_type_*`, `fx_page_*`, plus main quick `effect_type_*` and LFO/preset/MIDI dropdowns.
    - Sensors: iterate `sens_slot_*` and `sens_param_*` (for lanes 0..7).
    - Settings (Envelope): render `env_qr_dd` and `env_min_dd` when open.

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
- Effects population and z-ordering: `src/main_integrated_simple.cpp` (search `fx_type_`, `fx_page_`)
- Sensors population and z-ordering: `src/main_integrated_simple.cpp` (search `sens_slot_`, `sens_param_`)
- Settings (Envelope) population & persistence: `src/main_integrated_simple.cpp` (search `env_qr_dd`, `env_min_dd`; load/save `envelope.*`)

Notes
- Text rendering uses SDL_ttf when `HAVE_SDL_TTF` is defined; the integrated app target defines this flag to ensure dropdown text is visible.
- If runtime environments lack SDL_ttf, consider adding a fallback text routine (e.g., simple bitmap font) or verify dylib search paths.

Sequencer specifics
- Controls on Sequencer page:
  - `seq_jump`: quick Jump dropdown populated from `Sequencer::getSectionNames()` (prefixed with `"Jump: "`).
  - `seq_sec_name_0..4`: section name dropdowns for the inline editor.
  - `seq_sec_bar_0..4`: bar index dropdowns for the inline editor.
- Rendering (z-order):
  - The Sequencer timeline overlay is drawn after the screen render; Sequencer dropdown open lists are rendered after the overlay so lists remain on top.
- Input handling:
Settings (Envelope) specifics
- Controls on Settings screen:
  - `env_qr_dd` (Retrigger Quick Release):
    - Items: `"20 ms"`, `"8 ms"`, `"5 ms"`.
    - Behavior: Updates the per-voice one-shot release override used on voice stealing; persists to user config `envelope.quick_release_s`.
  - `env_min_dd` (Envelope Minimums):
    - Items: `"Normal (A5ms/R10ms)"`, `"Aggressive (A2ms/R5ms)"`, `"Ultra (A1.5ms/R3ms)"`.
    - Behavior: Sets global ModEnvelope minimums for attack/release; persists to user config `envelope.min_attack_s` and `envelope.min_release_s`.
- Population & defaults:
  - Items are added at Settings build; we `selectItemSilently(...)` to ensure a visible default.
  - On startup, persisted values are loaded and selections synced.
- Z‑order & click‑through:
  - Settings dropdowns follow the same render-after-screen and click-through prevention pattern; open lists render above controls and consume clicks outside the list to prevent background activation.
  - The main loop checks for open `seq_jump`, `seq_sec_name_i`, and `seq_sec_bar_i` first when the Sequencer screen is active, mirroring the Effects/Sensors click-through prevention pattern.
