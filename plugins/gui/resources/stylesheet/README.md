# GUI style

`modern.qss` is a stylesheet **template**. Every color, font and metric is a design token written between two at-signs, for
example the token `surface-base`. The values live in `src/style/theme.cpp` (`Theme::forMode`), one set for dark and one for
light. `Theme::apply` renders the template once at startup and when the user switches themes, it also sets the Fusion style,
the palette, the UI font and the system color scheme (dark title bars).

Do not write literal colors in the template or in widget code. Add a token if a role is missing.

## Principles

Derived from Qt's styling guide, KDAB's advice on style sheets, Microsoft's Windows 11 guidance and published dark-theme
design systems.

- **Palette first.** Fusion paints from the palette built from the tokens, the sheet only adds shape, spacing and states.
- **Layered surfaces.** `surface-canvas` (wells, inputs, graph) < `surface-base` (window) < `surface-raised` (panels, toolbars)
  < `surface-overlay` (menus, dialogs, tooltips). Nearer means lighter. No pure black or white.
- **Translucent states.** `state-hover`, `state-pressed`, `fill-control*` are overlays, so they work on every surface.
- **Text hierarchy** with `text`, `text-muted` and `text-disabled`, about 14:1, 7.5:1 and a low level on the base surface.
- **One accent** (`accent*`) for selection, focus and primary actions. Status colors (`success`, `warning`, `danger`) are for
  small signals, their `-subtle` variants are tinted backgrounds instead of saturated fills.
- **Shape.** 4 px radius for controls (`radius-control`), 8 px for overlays (`radius-overlay`), 1 px strokes, 28 px control height.
- **Type.** Segoe UI Variable at 13 px for the interface, the monospace font only for code, logs and the graph.

## Primary buttons

Set the dynamic property `primary` to `true` on a `QPushButton` for the accent style. The default button of a dialog gets it
automatically.
