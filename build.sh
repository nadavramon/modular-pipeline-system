#!/usr/bin/env bash
set -euo pipefail

#Toolchain and flags
CC=gcc-13
WARN="-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wundef -Wpointer-arith -Wcast-align"
CFLAGS="-std=gnu11 -g -O0 -pthread -fno-common -D_GNU_SOURCE $WARN"
LDFLAGS="-ldl -pthread"
SHARED_FLAGS="-fPIC -shared"

#Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() { 
  echo -e "${GREEN}[BUILD]${NC} $1"; 
}
print_warning() { 
  echo -e "${YELLOW}[WARN]${NC} $1";
}
print_error() {
  echo -e "${RED}[ERROR]${NC} $1"; 
}

usage() {
  cat <<'EOF'
Usage:
  ./build.sh #build
  ./build.sh clean #remove build artifacts
  ./build.sh rebuild #clean and build
EOF
}

clean_artifacts() {
  printf "${YELLOW}[CLEAN]${NC} removing build artifacts...\n"
  rm -f output/analyzer output/*.so
  printf "${GREEN}[OK]${NC} cleanup complete\n"
}

#Handling arguments
case "${1:-}" in
  "") : ;;
  clean) clean_artifacts; exit 0 ;;
  rebuild) clean_artifacts; printf "${YELLOW}[INFO]${NC} rebuilding project...\n" ;;
  -h|--help) usage; exit 0 ;;
  *) usage; exit 2 ;;
esac

#Environment probe
mkdir -p output
print_status "toolchain: $($CC --version | head -n 1)"

#Analyzer
print_status "analyzer"
if ! $CC $CFLAGS -o output/analyzer main.c $LDFLAGS; then
  print_error "failed to build analyzer"
  exit 1
fi

#Plugins
build_plugin() {
  local name="$1"
  print_status "${name}.so"
  if ! $CC $CFLAGS $SHARED_FLAGS \
        -o "output/${name}.so" \
        "plugins/${name}.c" \
        plugins/plugin_common.c \
        plugins/sync/monitor.c \
        plugins/sync/consumer_producer.c \
        $LDFLAGS; then
    print_error "failed to build ${name}"
    exit 1
  fi
}

for plugin in logger uppercaser flipper rotator expander typewriter; do
  build_plugin "$plugin"
done

print_status "[OK] build complete"