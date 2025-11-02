# C++ Shell

A lightweight Unix-like shell implementation in C++, built as part of the CodeCrafters shell challenge.

## Features

### Currently Implemented
- **Basic Shell Loop**: Continuous command execution with `$` prompt
- **Built-in Commands**:
  - `echo`: Print arguments to stdout
  - `exit`: Terminate the shell
  - `type`: Check if command is built-in or executable in PATH
- **External Command Execution**: 
  - Searches for executables in PATH directories
  - Uses `fork()` + `execvp()` for process creation
  - Proper exit code handling
- **PATH Resolution**: Automatically finds executables in system PATH

### Example Usage
```bash
$ echo hello world
hello world
$ type echo
echo is a shell builtin
$ type ls
ls is /usr/bin/ls
$ ls -la
# ... directory listing ...
$ exit
```

## Building

### Prerequisites
- C++17 compatible compiler (g++ 7+ or clang++)
- Unix-like system (Although I try to implement the path separator for windows, the usage of CreateProcess() was making me tired, maybe one day I'll finish this)

### Build commands
- To see all build commands just run
```bash
make help
```

## Future Development Roadmap

### Phase 2: Enhanced Parsing
- Quoting Support
- Escape Characters

### Phase 3: User Experience
- Command Autocompletion
- Command History

### Phase 4: Advanced Features
- I/O Redirection: Support for `>`, `>>`, `<` operators
- Pipes: Command chaining with `|` operator
- Background Jobs: Run commands with `&`
