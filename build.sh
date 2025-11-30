#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status()
{
echo -e "${GREEN}[BUILD]${NC} $1"
}
print_warning()
{
echo -e "${YELLOW}[WARNING]${NC} $1"
}
print_error()
{
echo -e "${RED}[ERROR]${NC} $1"
}

# Clean function
clean_artifacts() {
  echo -e "${YELLOW}[CLEAN]${NC} removing build artifacts..."
  rm -f output/analyzer output/*.so
  echo -e "${GREEN}[OK]${NC} cleanup complete"
}

# Handle commands
case "${1:-}" in
  clean)
    clean_artifacts
    exit 0
    ;;
  rebuild)
    clean_artifacts
    echo -e "${YELLOW}[INFO]${NC} rebuilding project..."
    ;;
esac

# Create output directory
mkdir -p output

# Build main application
print_status "Building analyzer"
gcc -o output/analyzer main.c -ldl -lpthread || { 
    print_error "Failed to build analyzer"; 
    exit 1; 
}

# Build plugins exactly as specified in the assignment
for plugin_name in logger uppercaser rotator flipper expander typewriter; do
    print_status "Building plugin: $plugin_name"
    gcc -fPIC -shared -o output/${plugin_name}.so \
        plugins/${plugin_name}.c \
        plugins/plugin_common.c \
        plugins/sync/monitor.c \
        plugins/sync/consumer_producer.c \
        -ldl -lpthread || {
        print_error "Failed to build $plugin_name"
        exit 1
    }
done

print_status "Build complete"
