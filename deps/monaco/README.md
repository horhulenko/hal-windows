# Monaco editor

Vendored runtime of [monaco-editor](https://github.com/microsoft/vscode) 0.57.0 (the editor component of VS Code, MIT licensed),
used by the HAL GUI through Qt WebEngine. Source: the `min/vs` folder of the npm package `monaco-editor@0.57.0`.

Trimmed to what HAL needs: the AMD loader, the editor core, the editor worker and the language definitions for
Python, SystemVerilog/Verilog, Markdown, YAML, shell, PowerShell and batch files.
The language chunks that were removed are loaded lazily by Monaco and are never requested by HAL.

`host.html` and `host.js` are HAL's own page around the editor, see `plugins/gui/src/code_editor/monaco_editor.cpp`.
