// Host page that embeds the Monaco editor (the editor component of VS Code) into the HAL GUI.
// Qt talks to it through two channels: Qt -> page by calling functions of window.halEditor,
// page -> Qt by posting {type, args} messages through window.chrome.webview (Microsoft WebView2).
(function () {
    'use strict';

    var host = (window.chrome && window.chrome.webview) ? window.chrome.webview : null;
    var editor = null;
    var savedVersion = 0;
    var pendingCompletions = {};
    var nextCompletionId = 1;
    var applying = false;
    var highlightIds = [];

    window.MonacoEnvironment = {
        getWorkerUrl: function () { return 'vs/editor/editor.worker.js'; }
    };

    function post(type) {
        if (host) host.postMessage({ type: type, args: Array.prototype.slice.call(arguments, 1) });
    }

    function isModified() {
        return editor.getModel().getAlternativeVersionId() !== savedVersion;
    }

    function defineTheme(colors) {
        colors = colors || {};
        monaco.editor.defineTheme('hal', {
            base: 'vs-dark',
            inherit: true,
            rules: [],
            colors: {
                'editor.background': colors.background || '#1e1e1e',
                'editor.foreground': colors.foreground || '#d4d4d4',
                'editor.lineHighlightBackground': colors.lineHighlight || '#2a2d2e',
                'editorLineNumber.foreground': colors.lineNumber || '#858585',
                'editorLineNumber.activeForeground': colors.lineNumberActive || '#c6c6c6',
                'editor.selectionBackground': colors.selection || '#264f78',
                'editorCursor.foreground': colors.cursor || '#aeafad',
                'minimap.background': colors.background || '#1e1e1e'
            }
        });
        monaco.editor.setTheme('hal');
    }

    window.halEditor = {
        setText: function (text) {
            applying = true;
            editor.getModel().setValue(text);
            editor.getModel().pushStackElement();
            applying = false;
            savedVersion = editor.getModel().getAlternativeVersionId();
            post('contentChanged', text, false);
        },
        markSaved: function () {
            savedVersion = editor.getModel().getAlternativeVersionId();
            post('modifiedChanged', false);
        },
        markModified: function () {
            savedVersion = -1;
            post('modifiedChanged', true);
        },
        setLanguage: function (id) { monaco.editor.setModelLanguage(editor.getModel(), id); },
        setOptions: function (o) { editor.updateOptions(o); },
        setTheme: defineTheme,
        focus: function () { editor.focus(); },
        undo: function () { editor.trigger('hal', 'undo'); },
        redo: function () { editor.trigger('hal', 'redo'); },
        find: function () { editor.trigger('hal', 'actions.find'); },
        // search for text and select the next/previous match, flags: bit0 regex, bit1 case sensitive, bit2 whole word
        search: function (text, flags, backwards) {
            var model = editor.getModel();
            var selection = editor.getSelection();
            var from = backwards ? selection.getStartPosition() : selection.getEndPosition();
            var find = backwards ? model.findPreviousMatch : model.findNextMatch;
            var separators = (flags & 4) ? editor.getOption(monaco.editor.EditorOption.wordSeparators) : null;
            var m = find.call(model, text, from, !!(flags & 1), !!(flags & 2), separators, false);
            if (m) {
                editor.setSelection(m.range);
                editor.revealRangeInCenterIfOutsideViewport(m.range);
            }
            return !!m;
        },
        // highlight all matches of text (flags as for search), an empty text clears the highlighting
        highlight: function (text, flags) {
            var matches = [];
            if (text) {
                var separators = (flags & 4) ? editor.getOption(monaco.editor.EditorOption.wordSeparators) : null;
                matches = editor.getModel().findMatches(text, false, !!(flags & 1), !!(flags & 2), separators, false);
            }
            highlightIds = editor.deltaDecorations(highlightIds, matches.map(function (m) {
                return { range: m.range, options: { inlineClassName: 'halFindMatch' } };
            }));
            if (matches.length) editor.revealRangeInCenterIfOutsideViewport(matches[0].range);
            return matches.length;
        },
        goToLine: function (line) {
            editor.revealLineInCenter(line);
            editor.setPosition({ lineNumber: line, column: 1 });
        },
        completionsReady: function (id, items) {
            var cb = pendingCompletions[id];
            delete pendingCompletions[id];
            if (cb) cb(items);
        },
        ready: function () { return !!editor; }
    };

    function createEditor() {
        editor = monaco.editor.create(document.getElementById('editor'), {
            value: '',
            language: 'python',
            automaticLayout: true,
            scrollBeyondLastLine: false,
            fontFamily: "Consolas, 'Courier New', monospace",
            fontSize: 14,
            tabSize: 4,
            insertSpaces: true,
            renderWhitespace: 'selection',
            minimap: { enabled: true }
        });
        defineTheme();

        editor.onDidChangeModelContent(function () {
            if (applying) return;
            post('contentChanged', editor.getValue(), isModified());
        });
        editor.onKeyDown(function () { post('keyPressed'); });
        editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.KeyS, function () { post('saveRequested'); });
        editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.Enter, function () { post('runRequested'); });

        // code completion is answered by HAL's embedded Python interpreter
        monaco.languages.registerCompletionItemProvider('python', {
            triggerCharacters: ['.'],
            provideCompletionItems: function (model, position) {
                return new Promise(function (resolve) {
                    if (!host) { resolve({ suggestions: [] }); return; }
                    var id = nextCompletionId++;
                    var word = model.getWordUntilPosition(position);
                    pendingCompletions[id] = function (items) {
                        resolve({
                            suggestions: items.map(function (it) {
                                return {
                                    label: it.label,
                                    kind: monaco.languages.CompletionItemKind.Variable,
                                    // jedi reports only the part that is still missing, the range covers the word typed so far
                                    insertText: word.word + it.insertText,
                                    range: { startLineNumber: position.lineNumber, endLineNumber: position.lineNumber, startColumn: word.startColumn, endColumn: position.column }
                                };
                            })
                        });
                    };
                    var before = model.getValueInRange({ startLineNumber: 1, startColumn: 1, endLineNumber: position.lineNumber, endColumn: position.column });
                    post('completionRequested', id, before);
                });
            }
        });

        // keywords, built-ins and snippets, available without the Python interpreter
        var keywords = ['False', 'None', 'True', 'and', 'as', 'assert', 'async', 'await', 'break', 'class', 'continue', 'def', 'del', 'elif', 'else', 'except', 'finally',
                        'for', 'from', 'global', 'if', 'import', 'in', 'is', 'lambda', 'nonlocal', 'not', 'or', 'pass', 'raise', 'return', 'try', 'while', 'with', 'yield'];
        var builtins = ['abs', 'all', 'any', 'bool', 'bytes', 'dict', 'enumerate', 'filter', 'float', 'format', 'getattr', 'hasattr', 'hex', 'input', 'int', 'isinstance',
                        'len', 'list', 'map', 'max', 'min', 'next', 'open', 'ord', 'print', 'range', 'repr', 'reversed', 'round', 'set', 'setattr', 'sorted', 'str', 'sum',
                        'super', 'tuple', 'type', 'zip'];
        var snippets = [
            { label: 'def', detail: 'function', text: 'def ${1:name}(${2:args}):\n    ${0:pass}' },
            { label: 'class', detail: 'class', text: 'class ${1:Name}:\n    def __init__(self${2}):\n        ${0:pass}' },
            { label: 'for', detail: 'for loop', text: 'for ${1:item} in ${2:iterable}:\n    ${0:pass}' },
            { label: 'if', detail: 'if statement', text: 'if ${1:condition}:\n    ${0:pass}' },
            { label: 'while', detail: 'while loop', text: 'while ${1:condition}:\n    ${0:pass}' },
            { label: 'try', detail: 'try/except', text: 'try:\n    ${1:pass}\nexcept ${2:Exception} as ${3:e}:\n    ${0:raise}' },
            { label: 'with', detail: 'with statement', text: 'with ${1:expression} as ${2:name}:\n    ${0:pass}' },
            { label: 'main', detail: 'entry point', text: 'if __name__ == "__main__":\n    ${0:main()}' }
        ];
        monaco.languages.registerCompletionItemProvider('python', {
            provideCompletionItems: function (model, position) {
                var word = model.getWordUntilPosition(position);
                var range = { startLineNumber: position.lineNumber, endLineNumber: position.lineNumber, startColumn: word.startColumn, endColumn: word.endColumn };
                var kinds = monaco.languages.CompletionItemKind;
                var rules = monaco.languages.CompletionItemInsertTextRule;
                var suggestions = [];
                keywords.forEach(function (k) { suggestions.push({ label: k, kind: kinds.Keyword, insertText: k, range: range }); });
                builtins.forEach(function (b) { suggestions.push({ label: b, kind: kinds.Function, insertText: b, range: range }); });
                snippets.forEach(function (s) {
                    suggestions.push({ label: s.label, kind: kinds.Snippet, detail: s.detail, insertText: s.text, insertTextRules: rules.InsertAsSnippet, range: range, sortText: 'z' + s.label });
                });
                return { suggestions: suggestions };
            }
        });
    }

    require.config({ paths: { vs: 'vs' } });
    require(['vs/editor/editor.main'], function () {
        createEditor();
        post('ready');
    });
})();
