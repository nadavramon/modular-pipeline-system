set -euo pipefail

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
print_status(){ echo -e "${GREEN}[TEST]${NC} $1"; }
print_pass(){   echo -e "${GREEN}[PASS]${NC} $1"; }
print_fail(){   echo -e "${RED}[FAIL]${NC} $1"; }

[[ -x ./build.sh ]] || { print_fail "build.sh missing or not executable"; exit 1; }

print_status "Rebuilding project (clean + build)"
./build.sh rebuild

run_pipe() {
  local input="$1"; shift
  printf "%b" "$input" | ./output/analyzer 10 "$@"
}

echo -e "\n=== logger only ==="
out="$(run_pipe "hello world\n<END>\n" logger | grep '^\[logger\]' | head -n1 || true)"
[[ "$out" == "[logger] hello world" ]] && print_pass "logger only" || { print_fail "expected '[logger] hello world', got '$out'"; exit 1; }

echo -e "\n=== uppercaser -> logger ==="
out="$(run_pipe 'hello world\n<END>\n' uppercaser logger | grep '^\[logger\]' | head -n1 || true)"
[[ "$out" == "[logger] HELLO WORLD" ]] && print_pass "uppercaser -> logger" || { print_fail "expected '[logger] HELLO WORLD', got '$out'"; exit 1; }

echo -e "\n=== flipper -> logger ==="
out="$(run_pipe 'hello world\n<END>\n' flipper logger | grep '^\[logger\]' | head -n1 || true)"
[[ "$out" == "[logger] dlrow olleh" ]] && print_pass "flipper -> logger" || { print_fail "expected '[logger] dlrow olleh', got '$out'"; exit 1; }

echo -e "\n=== rotator -> logger ==="
out="$(run_pipe 'hello world\n<END>\n' rotator logger | grep '^\[logger\]' | head -n1 || true)"
[[ "$out" == "[logger] dhello worl" ]] && print_pass "rotator -> logger" || { print_fail "expected '[logger] dhello worl', got '$out'"; exit 1; }

echo -e "\n=== expander -> logger ==="
out="$(run_pipe 'hello world\n<END>\n' expander logger | grep '^\[logger\]' | head -n1 || true)"
[[ "$out" == "[logger] h e l l o   w o r l d" ]] && print_pass "expander -> logger" || { print_fail "expected '[logger] h e l l o   w o r l d', got '$out'"; exit 1; }

echo -e "\n=== typewriter -> logger ==="
out="$(run_pipe 'hello world\n<END>\n' typewriter logger)"
grep -q '^\[typewriter\] ' <<<"$out" && grep -q '^\[logger\] hello world$' <<<"$out" \
  && print_pass "typewriter -> logger" \
  || { print_fail "expected typewriter tag and '[logger] hello world'"; exit 1; }

echo -e "\n=== upper -> flip -> rot13 -> log ==="
out="$(run_pipe 'hello world\n<END>\n' uppercaser flipper rotator logger | grep '^\[logger\]' | head -n1 || true)"
[[ "$out" == "[logger] HDLROW OLLE" ]] && print_pass "upper -> flip -> rot13 -> log" \
  || { print_fail "expected '[logger] HDLROW OLLE', got '$out'"; exit 1; }

echo -e "\n=== missing args ==="
if ./output/analyzer >/dev/null 2>&1; then
  print_fail "analyzer should fail with missing args"; exit 1
else
  print_pass "missing args rejected"
fi

echo -e "\n=== invalid plugin ==="
if printf "<END>\n" | ./output/analyzer 10 notaplugin >/dev/null 2>&1; then
  print_fail "unknown plugin should fail"; exit 1
else
  print_pass "invalid plugin rejected"
fi

echo -e "\n=== empty input line ==="
out="$(printf "\n<END>\n" | ./output/analyzer 10 logger | grep '^\[logger\]' | head -n1 || true)"
[[ "$out" == "[logger] " ]] && print_pass "empty input handled" || { print_fail "expected '[logger] ' (trailing space), got '$out'"; exit 1; }

echo -e "\n=== long line (1024 chars) ==="
LONG="$(python3 - <<'PY'
print("a"*1024)
PY
)"
count="$(printf "%s\n<END>\n" "$LONG" | ./output/analyzer 10 uppercaser logger | grep '^\[logger\]' | wc -c)"
[[ "$count" -gt 10 ]] && print_pass "1024-char line handled" || { print_fail "long line not handled"; exit 1; }

echo "[TEST] invalid queue size separation"
OUT_FILE=$(mktemp); ERR_FILE=$(mktemp)
./output/analyzer 0 uppercaser 1>"$OUT_FILE" 2>"$ERR_FILE" || true
grep -qF "Usage: ./analyzer <queue_size>" "$OUT_FILE" && grep -qF "error: invalid queue size '0'" "$ERR_FILE" \
  && print_pass "usage -> stdout, error -> stderr" \
  || { print_fail "stdout/stderr separation wrong"; echo "---STDOUT---"; cat "$OUT_FILE"; echo "---STDERR---"; cat "$ERR_FILE"; rm "$OUT_FILE" "$ERR_FILE"; exit 1; }
rm "$OUT_FILE" "$ERR_FILE"

echo -e "\n=== final shutdown message ==="
last="$(printf "hello\n<END>\n" | ./output/analyzer 10 logger | tail -n1)"
if [[ "$last" == "Pipeline shutdown complete" ]]; then
  print_pass "final shutdown message printed"
else
  print_fail "expected 'Pipeline shutdown complete', got '$last'"
  exit 1
fi

print_status "[OK] All tests passed."