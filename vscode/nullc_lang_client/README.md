# nullc language client

This extension provides support for editing and debugging nullc programming language inside Visual Studio Code.

## Features

* Syntax Highlight

![Syntax Highlight Example](img/example_coloring.png)

---
* Diagnostics

![Diagnostics Example](img/example_diagnostics.png)

---
* Code region folding

![Code Folding Example](img/example_folding.png)

---
* Information on symbol hover

![Symbol Hover Example](img/example_hovers.png)

---
* Code completion

![Code Completion Example](img/example_completion.png)

---
* Function signature help

![Signature Help Example](img/example_signature.png)

---
* Goto definition

![Goto Definition Example](img/example_definition.png)

---
* Find all references

![Find All References Example](img/example_references.png)

---
* Document symbol highlights

![Document Symbol Highlight Example](img/example_highlight.png)

---
* Document symbol list

![Document Symbol List Example](img/example_symbols.png)

---
* Debugging
    * Launch & Terminate
    * Step Over, Step Into, Step Out
    * Breakpoints
    * Stack trace
    * Variable information

## Requirements

Native nullc language server is required to be built using nullc_lang_server project. 
Native nullc language debugger is required to be built using nullc_lang_debugger project. 

## Extension Settings

This extension contributes the following settings:

* `nullc.module_path`: Default non-project module search path.
* `nullc.trace.server`: Traces the communication between VS Code and the nullc language server.

## Known Issues

* Extension must be set up manually
* Language server must be build from separate project
* Language debugger must be build from separate project
* Error recovery and symbol search from location is a work in progress 

## Release Notes

### Version 0.2.0
* Added debugger

### Version 0.1.0
* Initial release