# Modular Pipeline System

A high-performance, modular text processing pipeline implemented in C. This system allows for dynamic loading of plugins to process text streams in a multi-threaded environment using a Producer-Consumer architecture.

## 🚀 Features

*   **Modular Architecture**: Plugins are loaded dynamically at runtime using `dlopen`.
*   **Multi-threading**: Each plugin runs in its own thread, maximizing concurrency.
*   **Synchronization**: Thread-safe queues implemented with Monitors and Mutexes.
*   **Memory Safety**: Rigorously tested with Valgrind and AddressSanitizer to ensure zero memory leaks.
*   **Robust Error Handling**: Graceful shutdown and error reporting mechanisms.

## 🛠️ Build & Install

The project includes a build script that compiles the main analyzer and all plugins.

### Prerequisites
*   GCC Compiler (supporting C11)
*   Make (optional, script provided)
*   Linux/macOS environment (Tested on Ubuntu 24.04 & macOS)

### Building
Run the provided build script:
```bash
./build.sh
```
This will create an `output/` directory containing the `analyzer` executable and plugin `.so` files.

## 💻 Usage

Run the analyzer by specifying the queue size and the sequence of plugins to execute.

```bash
./output/analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>
```

### Arguments
*   `queue_size`: Maximum number of items in each plugin's input queue.
*   `pluginX`: Name of the plugin to load (e.g., `uppercaser`, `logger`).

### Example
```bash
# Run a pipeline: Uppercase -> Rotate -> Log
echo "hello world" | ./output/analyzer 10 uppercaser rotator logger
```
**Output:**
```
[logger] DHELLO WORL
Pipeline shutdown complete
```

## 🔌 Available Plugins

| Plugin | Description |
|--------|-------------|
| `logger` | Logs the input string to stdout. Usually the last plugin in a chain. |
| `uppercaser` | Converts all characters to uppercase. |
| `rotator` | Rotates the string one position to the right (circular shift). |
| `flipper` | Reverses the string. |
| `expander` | Adds a space between each character. |
| `typewriter` | Prints characters with a slight delay to simulate typing. |

## 🧪 Testing

The project comes with a comprehensive test suite covering functionality, memory management, and edge cases.

```bash
./test.sh
```

## 📂 Project Structure

*   `main.c`: Entry point, handles plugin loading and pipeline initialization.
*   `plugins/`: Source code for all plugins.
    *   `plugin_common.c`: Shared logic for plugin threads.
    *   `sync/`: Synchronization primitives (Monitor, Queue).
*   `build.sh`: Compilation script.
*   `test.sh`: Automated test suite.

## 👥 Authors

*   Nadav Ramon
