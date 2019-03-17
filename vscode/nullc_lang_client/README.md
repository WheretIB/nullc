# nullc language client

This extension provides support for editing and debugging nullc programming language inside Visual Studio Code.

## Features

* Syntax Highlight

![Syntax Highlight Example](vscode/nullc_lang_client/img/example_coloring.png)

---
* Diagnostics

![Diagnostics Example](vscode/nullc_lang_client/img/example_diagnostics.png)

---
* Code region folding

![Code Folding Example](vscode/nullc_lang_client/img/example_folding.png)

---
* Information on symbol hover

![Symbol Hover Example](vscode/nullc_lang_client/img/example_hovers.png)

---
* Code completion

![Code Completion Example](vscode/nullc_lang_client/img/example_completion.png)

---
* Function signature help

![Signature Help Example](vscode/nullc_lang_client/img/example_signature.png)

---
* Goto definition

![Goto Definition Example](vscode/nullc_lang_client/img/example_definition.png)

---
* Find all references

![Find All References Example](vscode/nullc_lang_client/img/example_references.png)

---
* Document symbol highlights

![Document Symbol Highlight Example](vscode/nullc_lang_client/img/example_highlight.png)

---
* Document symbol list

![Document Symbol List Example](vscode/nullc_lang_client/img/example_symbols.png)

---
* Debugging
    * Launch, Restart & Terminate
    * Step Over, Step Into, Step Out
    * Breakpoints
    * Stack trace
    * Variable information
    * Source files from module debug data
    * Variable value update

## Extension Settings

This extension contributes the following settings:

* `nullc.module_path`: Default non-project module search path.
* `nullc.trace.server`: Traces the communication between VS Code and the nullc language server.

## Known Issues

* Windows-only
* Error recovery and symbol search from location is a work in progress 

## Release Notes

### Version 0.3.1
* Package configuration fixes

### Version 0.2.2
* Debugger improvements

### Version 0.2.1
* Debugger improvements

### Version 0.2.0
* Added debugger

### Version 0.1.0
* Initial release
