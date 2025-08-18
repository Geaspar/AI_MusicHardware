## Preset System Design — Research and Recommendations (with Vital insights)

This document compiles best-in-class practices for synthesizer preset systems, with specific insights from Vital, and proposes a robust, future-proof preset architecture for AIMusicHardware.

### What “best-in-class” looks like
- **Metadata-rich browsing**: Name, category, author, tags, rating, favorites, usage counts, creation date, last modified, version.
- **Fast search/filter**: Instant search on text and tags; filters by categories, types (bass/lead/pad), technique (FM/subtractive/wavetable), tonal (warm/bright), articulation (pluck/sustain), MPE capability, CPU profile.
- **Portable and shareable**: Single-file preset share (embed assets) and bank formats; no broken paths on other machines.
- **Robust schema**: Versioned, forward/backward compatible, graceful partial loads with warnings.
- **Full-state capture**: Synth core params, modulation matrix, macro assignments, envelopes/LFO shapes, effects chain (order, enabled, params), arpeggiator/sequencer, tuning, global vs preset-local settings.
- **Dependencies handled**: Wavetables, samples, IRs included or referenced with content hashes; deduplicated across banks.
- **Macro-first UX**: Clearly exposed macro controls with meaningful labels mapped to multiple destinations with scaling and curves.
- **Preview & consistency**: Consistent gain staging (no “too hot” presets), optional audio preview and visual preview (spectrum/envelope shapes), init patch baseline.
- **DAW integration**: Good behavior with VST3/AU/CLAP state; optional exports to host preset formats; NKS-like metadata optional.
- **Authoring tools**: Per-parameter locks (do-not-overwrite on load), randomize with constraints, compare (A/B), history/undo.

### Vital-specific insights
Vital (by Matt Tytel) is widely praised for preset ergonomics:
- **Macro-centric design**: 4 macros prominently placed, encouraging sound designers to expose musical controls; macros can map to many parameters with depths and polarities. Best presets feel “alive” due to these macro mappings.
- **Shareable preset files**: Users commonly share single preset files and “banks”. Assets (custom wavetables) can be embedded or bundled, ensuring portability.
- **Discoverability**: Good categorization and tags in browser; quick audition with consistent output; community-driven distribution.
- **Modulation visibility**: Matrix and visual mod rings on controls make presets clearer and safer to tweak.
- **Custom LFO shapes**: Presets store user LFO shapes, grid/snap, and timing sync; this fidelity is essential for replication.
- **Gain staging**: Presets are normalized to avoid clipping; output levels remain musical across patches.

Implications for us:
- Make macros first-class and encourage designers to author them. Persist names (“Brightness”, “Movement”) and units.
- Bundle or embed assets with hashes to avoid missing wavetables; maintain a content store and deduplicate.
- Store visual LFO/envelope state entirely (shapes, rates, sync, grid) to enable faithful recall.
- Normalize wet/effect levels and provide output trim. Consider RMS-based wet normalization for reverbs/delays.

### Comparative notes (other synths)
- **Serum (Xfer)**: Simple, fast browser; .fxp/.fxb compatibility; separate wavetable asset management; macros and mod matrix are central; high consistency in preset loudness.
- **Phase Plant (Kilohearts)**: Modular signal chain persisted per preset; “snapins” for FX; strong macro system; preset packs with embedded content; excellent categorization/tagging.
- **Omnisphere (Spectrasonics)**: Extremely rich metadata and search; layered architecture persisted; powerful browser with ratings and extensive tags.
- **u-he (Diva/Hive/Zebra)**: Structured folders; authoring metadata; robust tagging; NKS support; versioned preset schema.
- **Surge XT (open source)**: JSON-ish, tagged, very portable; good example for open, versioned schema and cross-platform asset handling.

### Functional requirements for AIMusicHardware
- **Scope of a preset**
  - Synth engine parameters (including ADSR, oscillators, hybrid/legacy engine selection, wavetable frame/morph).
  - Modulation matrix: sources, destinations (by stable names), amounts, curves, bipolar/unipolar flags, smoothing.
  - Macros: 4–8 macro definitions (name, color, range), connections to multiple destinations with per-connection curves/scales.
  - FX chain: per-slot type, enabled, order, parameters, page selection; global filter presence and parameters.
  - Performance controls: pitch bend range, MPE mode, aftertouch mode, mod wheel mapping.
  - Sequencer/arp: patterns, rates, latch, swing (if present).
  - Tuning/microtuning: scala file or table; A4 reference; per-preset or global.
  - UI hints: favorite, author, category, tags, rating; optional preview gain trim.
  - Assets: referenced/embedded wavetables, IRs; content hashes.

- **Metadata**
  - name, author, category, tags[], rating(1–5), favorite(bool), description, version (semantic), created_at, modified_at, uuid.
  - technical metadata: engine version, requires-hybrid(bool), cpu_profile(estimate), loudness_target(dbfs).

- **Interoperability**
  - VST3/CLAP/AU state: preset loads via plugin state; optional export/import to our preset file.
  - Optional NKS-style tags for ecosystems.

### Data model and IDs
- **Stable IDs**
  - Parameters and destinations must be addressed by stable string IDs (not indices). Example: "filter_cutoff", "FX:Slot3:FDNReverb:mix".
  - Effects slots get stable slot IDs (`slot_1`..`slot_8`), independent of UI order; order is stored separately.
  - Mod connections store source name, destination ID, and amount; on load, remap by ID, ignore unknowns with warnings.

- **Macro connections**
  - Macro N has: name, range [min,max], default, bi/unipolar; connections: list of {dest_id, amount, curve, smoothing}.

- **Assets**
  - Each asset carries: kind (wavetable, IR), name, author, license, sha256, byte_count. Stored in bank or referenced by hash.

### File format options
- **Single preset file (.preset)**
  - JSON-based, human-readable, easier diffing and versioning.
  - Pros: portability, inspectable; Cons: larger size; still fine for synth presets.

- **Bank file (.bank/.zip)**
  - Zip package containing: manifest.json, presets/*.preset, assets/* (wavetables, IRs), with dedup via hashes.
  - Vital-like distribution; easy to share packs.

- **Host formats**
  - VST3 .vstpreset, AU .aupreset, CLAP state: optionally export/import; our canonical format remains JSON.

### Versioning and migration
- **Schema version**: Top-level `schema_version` with semver. Increment minor for additive changes; major for breaking changes.
- **Migration rules**: Loader applies:
  - Key remapping (e.g., `env_attack` -> `envelope_attack`).
  - Unit conversions (e.g., Hz -> normalized 0–1 for cutoff).
  - Default fallbacks for missing fields.
  - Warning collection: expose to user non-fatal load issues.

### Loudness and consistency
- **Gain staging**
  - Normalize output to a target LUFS/peak range per preset; store an `output_trim_db` per-preset.
  - Normalize wet signals (reverbs/delays) in algorithm; avoid surprising “hot” presets.

### UX: Browser
- **Sources**: Factory, User, Packs; merged view.
- **Filters**: Category, Tags, Macros count, Requires Hybrid, CPU level, Author, Favorites, Rating.
- **Search**: Name/Author/Tags.
- **Preview**: Optional audio preview or simple arpeggio trigger; consistent level.
- **Actions**: Load, Save, Save As, Quick Save, Rename, Duplicate, Delete (User), Reveal in Finder.
- **A/B and History**: A/B compare, recent history of loaded presets for quick back/forward.

### Implementation recommendations for AIMusicHardware
- **Canonical format**: JSON `.preset` (we already use), extended schema below; `.bank` is a `.zip` with `manifest.json` + assets.
- **Stable destination IDs**: Already moving to name-based routing; extend to FX with `FX:Slot{n}:{EffectName}:{ParamId}`. Store both display name and id; load remaps by id first, name fallback.
- **Macros**: Add 4 macros minimum: `macro_1..macro_4` with names and connections; expose on UI main page.
- **Assets**: When saving, offer options: embed assets (include wavetable frames or spectral tables) or reference by path+hash. Prefer embedding for portability; use content-addressed store to deduplicate.
- **Quick Save vs Save**
  - Quick Save: write full `.preset` (entire state) under `presets/user/Quick Save <timestamp>.preset` including ADSR, FX, mod matrix, macros, engine toggles, hybrid settings, and UI hints. This will resolve current recall gaps.
  - Save As: dialog for name/author/category/tags; optional pack export.
- **Indexing**
  - Build a lightweight index (SQLite or JSON cache) of metadata for fast search; update incrementally on filesystem changes.
- **API surface**
  - `PresetManager`: load/save (single); `PresetBankManager`: import/export banks; `PresetIndex`: search/filter; `PresetMigration`: apply migrations.
  - Expose `serializeState()`/`deserializeState()` on Synthesizer and UI mediator to capture all preset-relevant state.
- **Safety**
  - Apply loads under audio mutex; update UI after engine state; use name-based mapping; warn/log remaps/missing.

### Proposed `.preset` skeleton
```json
{
  "schema_version": "1.1.0",
  "metadata": {
    "uuid": "...",
    "name": "Warm Keys",
    "author": "You",
    "category": "Keys",
    "tags": ["warm", "analog"],
    "rating": 4,
    "favorite": false,
    "description": "",
    "created_at": 1734048000,
    "modified_at": 1734048000,
    "engine_version": "0.9.3"
  },
  "parameters": {
    "oscillator_type": 2,
    "envelope_attack": 0.02,
    "envelope_decay": 0.1,
    "envelope_sustain": 0.7,
    "envelope_release": 0.5,
    "filter_cutoff": 0.62,
    "filter_resonance": 0.25,
    "master_volume": 0.7,
    "engine.hybrid_enabled": 0,
    "engine.timbre_min_phase": 0
  },
  "macros": [
    { "id": "macro_1", "name": "Brightness", "min": 0, "max": 1, "default": 0.5,
      "connections": [
        {"dest_id": "Filter Cutoff", "amount": 0.5, "curve": "linear", "smooth": 0.1},
        {"dest_id": "FX:Slot2:FDNReverb:mix", "amount": 0.2}
      ]
    }
  ],
  "mod_routing": [
    {"source": "LFO1", "destination": "Pitch", "amount": 0.2},
    {"source": "ModWheel", "destination": "Wavetable Position", "amount": 0.8}
  ],
  "effects": {
    "order": ["slot_1", "slot_2", "slot_3"],
    "slots": [
      {"slot_id": "slot_1", "type": "FDNReverb (Hall)", "enabled": true, "mix": 0.2, "page": 1,
       "params": {"decay_rt60_s": 2.1, "output_trim_db": -3.0}}
    ]
  },
  "assets": [
    {"kind": "wavetable", "name": "MyTable", "sha256": "...", "embedded": true}
  ]
}
```

### Migration and compatibility matrix
- Map legacy keys: `env_attack`/`attack` → `envelope_attack` (and others); `filter_cutoff` Hz → normalized (log mapping).
- Unknown FX or params: ignore with warning; retain raw key/value under `unknown` for round-trip safety.
- Cross-engine: if hybrid-only preset is loaded in legacy, gracefully approximate (e.g., set oscillator frame position); vice versa, map to morph.

### Roadmap to implement
1. Extend current serializer to include: macros, full mod matrix (by names/ids), FX chain with slot IDs and params, engine toggles, tuning (placeholder), and UI hints.
2. Finalize stable destination IDs for all FX params (`FX:SlotN:EffectName:ParamKey`).
3. Add `schema_version` and a migration layer in the loader (we already started key remapping and Hz→normalized conversion).
4. Implement bank packaging (.zip) with manifest and assets folder; embed assets on demand.
5. Build an indexer for `presets/factory`, `presets/user`, and banks; support fast search and tags.
6. Browser UI: categories, tags, favorites, rating, search; A/B compare; per-parameter locks during load.
7. Authoring UX: macro editor; randomizer with locks; gain normalization helper; preview generator.
8. QA pass: load robustness under audio mutex, UI synchronization, loudness consistency.

### Hardware-synth specific considerations
- **Storage + filesystem**
  - Prefer a two-tier layout: read-only factory bank partition and read-write user partition. Consider content-addressed store for assets (`/assets/<sha256>`), with presets referencing hashes.
  - Use append-only journaled writes or double-buffered files for power-loss safety. Each `.preset` (or bank manifest) carries a checksum (CRC32/sha256). On boot, verify and rebuild index if needed.
  - Minimize write wear: batch writes (debounce frequent saves), maintain a compaction routine, keep metadata index in RAM and persist infrequently.

- **Format on device**
  - Keep JSON as canonical for development; ship a compact binary representation on hardware for faster parse and lower RAM (e.g., flatbuffers/cap’n proto or custom TLV). Provide a host-side tool to compile `.preset` and `.bank` to binary banks.
  - Maintain schema version in both JSON and binary. Binary includes an offset table for O(1) access to common fields (macros, matrix, fx, assets).

- **Real-time safety & preset switching**
  - Never block the audio thread. All disk I/O happens in a low-priority worker. Apply state using double-buffered engine state: load → validate → prewarm (wavetables/FX) → atomic swap on block boundary.
  - Target preset switch latency < 50 ms for small patches. Strategies: prefetch next preset (setlist mode), incremental FX instantiation, prewarm Hybrid cache (morph/pitch bands), and optional tail spillover for FX (dual FX buses during transition).
  - Parameter ramps on load to avoid pops: envelope, filter, master gain, and FX wet mix ramp over 5–20 ms.

- **UI/UX on constrained controls**
  - Browser optimized for encoders/buttons: hierarchical navigation (Factory → Category → Preset), quick Favorites, Recent, and Setlists. Text search optional; prefer filters and tags.
  - Macro-first surface: 4–8 macro knobs near performance controls, with clear labels; per-preset color and ranges.
  - Per-parameter locks before load (e.g., lock tempo, master volume, gate/release behavior). A/B compare and History list.

- **Program change + setlists**
  - Map MIDI Bank Select (MSB/LSB) + Program Change to preset UUIDs via an index. Allow user-defined setlists with explicit order and gapless preload of the next item.
  - Snapshot vs Patch distinction: snapshot recalls only a subset (e.g., macros, FX mix) for performance.

- **Asset handling on device**
  - Embed small assets; large assets stored once by hash and referenced. On import, verify hash and deduplicate. Provide a maintenance page to purge unused assets.
  - Bank installer validates signatures (optional) and checks available storage before install; supports partial rollback on failure.

- **Security and authenticity**
  - Optional signed factory banks and vendor packs. Community presets remain unsigned but verified by checksum.
  - Sandboxed import path with validation; reject oversized or malformed assets.

- **Calibration and consistency**
  - Separate global device calibration (output trim, DAC reference) from presets. Factory presets normalized to a target level; enforce per-preset `output_trim_db` on load.

- **Reliability & recovery**
  - On boot, validate indexes; if corrupted, rebuild by scanning banks. Keep a safe-mode minimal bank (Init patch) in ROM.
  - Maintain error logs and counters (failed loads, CRC mismatches) for diagnostics.

- **Performance budgets**
  - RAM: cap max preset size and number of live assets; pool allocators for FX and modulation structures to avoid fragmentation.
  - CPU: limit modulation connections per preset (configurable cap) and require smoothing flags to prevent spikes; precompute curves for macros.

- **Connectivity**
  - Support USB mass storage (drag-and-drop banks), MIDI SysEx for preset/bank transfer, and optional Wi‑Fi/BLE manager app. Ensure transfers are resumable and verified.

- **Developer tooling**
  - CLI to validate, pack, sign, and convert presets/banks to device binaries. CI gates for schema checks and loudness normalization tests.

### Implementation — Gap analysis and phased plan

Current vs target highlights
- **Schema/versioning**: Target has `schema_version`, `uuid`, forward/back compat. Current `.preset` lacks schema/version; legacy key remap added. Gap: add version + migration warnings.
- **Full-state capture**: Target includes synth, FX chain (order/slots), modulation, macros, engine toggles, tuning, UI hints. Current covers synth + FX + modulation (partial), no macros, no engine toggles/tuning/UI hints. Gap: extend serializer/loader.
- **Stable IDs**: Target saves destinations as stable IDs like `FX:SlotN:Effect:Param`. Current saves by display name and applies first match. Gap: design IDs and remapping.
- **Macros**: Not present. Gap: macro model, UI, serialize/deserialize.
- **Assets/banks**: No embedding or bank packaging. Gap: asset store with hashes; `.bank` zip with manifest/assets.
- **Browser/index**: No tags/ratings/favorites/instant search. Gap: indexer and browser UI.
- **RT-safe loading**: Basic locking only. Gap: background load, prewarm, atomic swap + ramps.
- **Hardware format**: None. Gap: compact binary + converter.

Immediate actions (to improve Quick Save reliability)
- Add `schema_version` and `uuid` to `.preset` and record engine version.
- Include engine toggles (`engine.hybrid_enabled`, `engine.timbre_min_phase`) and preset-level `output_trim_db`.
- Convert modulation destinations to stable IDs; keep display name for UI; loader remaps by ID then name.
- Wrap preset load under audio mutex and ramp master/filter/wet 10–20 ms to avoid pops.

Phased plan and effort
- Phase A — Schema hardening (S–M)
  - Versioning, UUIDs, timestamps, warning collection; engine toggles; preset-level trim; stable destination IDs + migration.
- Phase B — Macros & modulation completeness (M–L)
  - Macro model/UI (4–8 knobs), serialize connections (curve, smoothing); extend mod matrix save/load with curves/bipolar.
- Phase C — Browser + Index (M–L)
  - Metadata indexer (JSON/SQLite); browser UI with categories/tags/ratings/favorites/search/history/A‑B.
- Phase D — Banks & Assets (M)
  - `.bank` pack/unpack with manifest and assets; hash dedup; CLI tools; device importer.
- Phase E — RT-safe switching & prewarm (M–L)
  - Worker I/O, validate, prewarm (Hybrid cache/FX), atomic swap on block boundary; optional dual-FX-bus tails.
- Phase F — Hardware prep (L)
  - Binary-on-device format + converter; journaling storage; program change mapping/setlists; optional signing.

Acceptance checkpoints
- A: Old/new presets load; destinations resolve by ID; engine toggles recall.
- B: Macros visible/functional; round-trip preserves mappings.
- C: Browser lists/searches hundreds instantly; favorites/tags persisted.
- D: Bank import/export with assets; no missing wavetables; hashes verified.
- E: Preset switch <50 ms typical; no pops; hybrid cache shows prewarm hits.
- F: Device image loads banks; program changes mapped; power-loss safe saves.

### How this helps Quick Save immediately
- Quick Save will simply dump the canonical `.preset` with the complete state (we’ve added ADSR already). With the above schema, quick saves will recall everything: ADSR, FX chain, mod routes by name, macros, hybrid toggles, and output trims, making it reliable and future-proof.

### Risks and mitigations
- **Schema drift**: Mitigate with `schema_version`, migration functions, and test presets.
- **Missing assets**: Embed or use content hashes with local cache; show warnings and substitute safe defaults.
- **Loudness variability**: Store per-preset output trim; apply wet normalization; provide preset-level limiter guard if desired.
- **Performance**: Lazy-decode on demand; index metadata once; avoid UI-thread file IO.
- **Name collisions**: Use stable IDs in addition to display names; disambiguate FX by slot ID.

### Appendix: Minimal stable ID list (initial)
- Synth params: `oscillator_type`, `envelope_attack`, `envelope_decay`, `envelope_sustain`, `envelope_release`, `filter_cutoff`, `filter_resonance`, `master_volume`.
- Mod sources: `LFO1`, `LFO2`, `ModWheel`, `Aftertouch`, `Velocity`, `Envelope`.
- Destinations (examples): `Pitch`, `Volume`, `Wavetable Position`, `Filter Cutoff`, `Filter Res`.
- FX example: `FX:Slot1:FDNReverb (Hall):mix`, `FX:Slot2:PlateReverb:output_trim_db`, etc.
