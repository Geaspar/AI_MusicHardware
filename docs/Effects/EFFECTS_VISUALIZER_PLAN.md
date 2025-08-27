# Effects Visualizer – Plan

Goal: Add a lightweight, live visualization panel to the Effects page that reflects the currently focused effect. Start with a single shared panel and a Delay visual (bouncing balls), expanding later to other effects.

## Rationale
- Improves intuitiveness by giving visual feedback tied to effect parameters.
- UI-only implementation avoids audio thread coupling and keeps performance predictable.
- Uses existing parameter values (from UI/effect getters) as the source of truth.

## MVP Scope
- One shared panel on the Effects page (to the right of the effect type/page controls).
- Tracks the most recently interacted effect slot/type.
- Step 1: Panel scaffold renders a border and the active effect name (no animation yet).
- Step 2: Wire parameter getters so visualizers can query current values.
- Step 3: Delay visualizer – bouncing balls indicating echo taps.
  - Time → spacing/velocity.
  - Feedback → number of echoes and decay (size/alpha/velocity attenuation).
  - Mix → trail intensity or background tint.
- Step 4: Pause visuals when Effects screen is inactive; safe defaults when no effect.

## Technical Plan (Incremental)
1. EffectsVisualizerPanel (UIComponent)
   - API: `setEffectType(std::string)`, `setParamGetter(std::function<float(const std::string&)>)`.
   - Render: bordered box with label “Effect: <type>” (placeholder).
   - Placement: single panel to the right of the slot controls (approx 300×200).
2. Wiring
   - On effect-type dropdown change for any slot, update the panel’s effect type.
   - Provide a param getter bound to the current slot/type’s UI controls or effect object.
3. Visualizers
   - Delay: simple bouncing balls; animate in `update(dt)` at UI frame rate.
   - Later: Reverb (room energy), Chorus/Flanger (LFO rings), etc.
4. Polish
   - Screen-visibility aware updates.
   - Graceful handling when parameters are missing.

## Performance
- Update/render at ~60 FPS; simple primitives using current `DisplayManager`.
- No heavy allocations in render path; use small fixed arrays where needed.

## Risks & Mitigations
- Risk: UI clutter – keep a single panel and modest size.
- Risk: Parameter sync – rely on existing UI values first, then add optional direct effect getters.

## Next Steps
- Implement Step 1 (panel scaffold + placement), test, then proceed to wiring and the Delay visualizer.

