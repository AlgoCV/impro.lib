#!/usr/bin/env bash
# Regenerate the DESKTOP-only, ReleaseFast binary distribution of the ImPro C
# library from a checkout of the source repository (`impro`).
#
# For every supported desktop target this produces:
#
#   lib/<os>-<arch>/libimpro.a              static C ABI archive (Rust / Swift)
#   lib/<os>-<arch>/libimpro_python.<ext>   dynamic bridge library (Python ctypes)
#
# and a single shared copy of the public headers:
#
#   include/impro.h
#   include/impro_types.h
#
# This repository is a *binaries-only* distribution: it carries no Zig sources.
# Point `IMPRO_SRC` at a checkout of the source repo to regenerate.
#
# Knobs (env vars):
#   IMPRO_SRC   Path to the `impro` source checkout   (default: ../impro)
#   ZIG         Zig compiler to use                   (default: zig)
#   OPTIMIZE    Zig optimize mode                     (default: ReleaseFast)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMPRO_SRC="${IMPRO_SRC:-$SCRIPT_DIR/../impro}"
ZIG="${ZIG:-zig}"
OPTIMIZE="${OPTIMIZE:-ReleaseFast}"

if [ ! -f "$IMPRO_SRC/build.zig" ]; then
    printf 'error: IMPRO_SRC (%s) is not an impro source checkout\n' "$IMPRO_SRC" >&2
    exit 1
fi
IMPRO_SRC="$(cd "$IMPRO_SRC" && pwd)"

# --- Desktop target matrix ------------------------------------------------------
#
#   <os>-<arch>|<zig -Dtarget=…>|<zig -Dcpu=…>|<dynamic-lib-filename>
TARGETS=(
    "macos-aarch64|aarch64-macos-none|apple_m1|libimpro_python.dylib"
    "macos-x86_64|x86_64-macos-none|x86_64_v3|libimpro_python.dylib"
    "linux-x86_64|x86_64-linux-gnu|x86_64_v3|libimpro_python.so"
    "linux-aarch64|aarch64-linux-gnu|cortex_a76|libimpro_python.so"
    "windows-x86_64|x86_64-windows-gnu|x86_64_v3|impro_python.dll"
)

build_target() {
    local name="$1" triple="$2" cpu="$3" pylib="$4"
    local tmp="$SCRIPT_DIR/.tmp/$name"
    local out="$SCRIPT_DIR/lib/$name"

    rm -rf "$tmp" && mkdir -p "$tmp" "$out"

    local common=(
        "-Doptimize=$OPTIMIZE"
        "-Dstrip=true"
        "-Dtarget=$triple"
        "-Dcpu=$cpu"
        "--prefix" "$tmp"
    )

    # Static C ABI archive + headers.
    ( cd "$IMPRO_SRC" && "$ZIG" build "${common[@]}" )
    # Dynamic Python bridge library.
    ( cd "$IMPRO_SRC" && "$ZIG" build python-libs "${common[@]}" )

    # Static archive: Windows-gnu emits `impro.lib`; Unix-like emit `libimpro.a`.
    local src_static
    src_static="$(ls "$tmp"/lib/libimpro.a "$tmp"/lib/impro.lib 2>/dev/null | head -n1)"
    cp -f "$src_static" "$out/libimpro.a"

    # Dynamic bridge library (name differs per OS). Zig installs shared
    # libraries under lib/ on Unix-likes but DLLs under bin/ on Windows.
    local src_dyn
    src_dyn="$(ls "$tmp"/lib/"$pylib" "$tmp"/bin/"$pylib" 2>/dev/null | head -n1)"
    cp -f "$src_dyn" "$out/$pylib"

    # Headers are target-independent; refresh the single shared copy.
    cp -f "$tmp"/include/*.h "$SCRIPT_DIR/include/"

    printf '  [OK] %-16s -> lib/%s/{libimpro.a,%s}\n' "$name" "$name" "$pylib"
    rm -rf "$tmp"
}

printf 'Building ImPro desktop binaries from %s\n' "$IMPRO_SRC"
for entry in "${TARGETS[@]}"; do
    IFS='|' read -r name triple cpu pylib <<<"$entry"
    build_target "$name" "$triple" "$cpu" "$pylib"
done
rm -rf "$SCRIPT_DIR/.tmp"
printf 'Done. Artifacts under %s/lib and %s/include\n' "$SCRIPT_DIR" "$SCRIPT_DIR"
