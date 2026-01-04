# OVERSEER

![Status](https://img.shields.io/badge/Status-Development-orange) ![Build](https://img.shields.io/badge/Build-Passing-green) ![License](https://img.shields.io/badge/License-GPLv3-blue)

**Overseer** is a terminal-based network discovery and communication tool. It utilizes UDP broadcasting for service discovery and TCP sockets for secure command transmission, visualized through a lightweight TUI (Text User Interface) built with `ncurses`.

---

## ⚠️ PROJECT STATUS: PRE-ALPHA

This repository currently represents the **architectural skeleton** of the final product.

*   **Under Construction:** The codebase is in active development. Features, APIs, and protocols are subject to change without backward compatibility.
*   **Proof of Concept:** The current implementation demonstrates the core networking logic (Beacon/Listener) and the TUI rendering engine. It is not yet feature-complete.
*   **Stability:** Core logic is functional, but edge cases and error handling are currently being implemented.

---

## 📂 ARCHITECTURE

The project follows a strict separation of concerns between system logic and the frontend interface.

```text
─── src
    ├── client
    │   ├── globals.h
    │   ├── main.c
    │   ├── system
    │   │   ├── api.c
    │   │   ├── api.h
    │   │   ├── atomic.c
    │   │   ├── atomic.h
    │   │   ├── network.c
    │   │   └── network.h
    │   └── tui
    │       ├── components.c
    │       ├── input.c
    │       ├── interface.c
    │       ├── interface.h
    │       ├── path_security.c
    │       ├── path_security.h
    │       ├── popup_file.c
    │       ├── popups.c
    │       └── render.c
    └── server
        ├── client_handler.c
        ├── main.c
        ├── net.c
        ├── server.h
        ├── stats.c
        └── utils.c
```

### Core Protocols
1.  **Discovery:** Servers broadcast UDP beacons on port `9999` containing their TCP port and Server ID.
2.  **Connection:** Clients listen for beacons, aggregate the list, and initiate TCP handshakes on the advertised ports.

---

## 🛠️ BUILDING AND RUNNING

### Prerequisites
*   GCC Compiler
*   `libncurses` development library
*   `pthread` library

### Compilation
A bootstrap script is provided to compile both binary targets.

```bash
chmod +x compile.sh
./compile.sh
```

### Usage

**1. Start the Server(s)**
Run one or more server instances. A custom TCP port can be specified as an argument.
```bash
./server        # Defaults to port 8080
./server 8081   # Starts on port 8081
```

**2. Start the Client**
Launch the TUI interface.
```bash
./client
```

---

## ⚖️ CONTRIBUTION GUIDELINES

Strict quality control is enforced to ensure the codebase remains maintainable. **Pull Requests that do not adhere to the following standards will be closed.**

### 1. Code Style & Formatting
*   **Indentation:** **8-character tabs**. No soft tabs (spaces).
*   **Comments:** **No comments inside function logic.** The code must be self-explanatory. Header files (`.h`) may contain brief API definitions.
*   **Naming:** Use `snake_case` for variables and functions. Use `PascalCase` for Structs.

### 2. Architectural Integrity
*   **No TUI in System:** Do not import `ncurses.h` or print UI elements inside `src/client/system/`.
*   **No Logic in TUI:** Do not perform socket operations or complex calculations inside `src/client/tui/`. The TUI is for rendering state and capturing input only.
*   **Global State:** Modifications to `globals.h` must be thread-safe.

### 3. Submission Process
*   **Atomic PRs:** Do not bundle refactoring, bug fixes, and new features into a single Pull Request.
*   **Commit Messages:** Must be imperative and descriptive (e.g., `Add TCP timeout handling`, not `fixed bug`).

---

## 📄 LICENSE

This project is licensed under the **GNU General Public License v3.0**.

See the [LICENSE](LICENSE) file for details.
