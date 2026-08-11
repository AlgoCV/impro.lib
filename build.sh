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

if ! command -v "$ZIG" >/dev/null 2>&1; then
    printf 'error: zig executable ("%s") not found in PATH\n' "$ZIG" >&2
    exit 1
fi

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

    # Build both the static library (+ headers) and the dynamic Python bridge.
    # We do this in a single call to avoid prefix cleanup issues between runs.
    ( cd "$IMPRO_SRC" && "$ZIG" build install python-libs "${common[@]}" )

    # Static archive: Windows-gnu emits `impro.lib`; Unix-like emit `libimpro.a`.
    local src_static=""
    if [ -f "$tmp/lib/libimpro.a" ]; then
        src_static="$tmp/lib/libimpro.a"
    elif [ -f "$tmp/lib/impro.lib" ]; then
        src_static="$tmp/lib/impro.lib"
    fi

    if [ -z "$src_static" ]; then
        printf 'error: static library not found for %s\n' "$name" >&2
        exit 1
    fi
    cp -f "$src_static" "$out/libimpro.a"

    # Dynamic bridge library (name differs per OS). Zig installs shared
    # libraries under lib/ on Unix-likes but DLLs under bin/ on Windows.
    local src_dyn=""
    if [ -f "$tmp/lib/$pylib" ]; then
        src_dyn="$tmp/lib/$pylib"
    elif [ -f "$tmp/bin/$pylib" ]; then
        src_dyn="$tmp/bin/$pylib"
    fi

    if [ -z "$src_dyn" ]; then
        printf 'error: dynamic library (%s) not found for %s\n' "$pylib" "$name" >&2
        exit 1
    fi
    cp -f "$src_dyn" "$out/$pylib"

    # Headers are target-independent; refresh the single shared copy.
    cp -f "$tmp"/include/*.h "$SCRIPT_DIR/include/"

    printf '  [OK] %-16s -> lib/%s/{libimpro.a,%s}\n' "$name" "$name" "$pylib"
    rm -rf "$tmp"
}

printf 'Building ImPro desktop binaries from %s\n' "$IMPRO_SRC"
trap 'rm -rf "$SCRIPT_DIR/.tmp"' EXIT

for entry in "${TARGETS[@]}"; do
    echo "Building ${entry}"
    IFS='|' read -r name triple cpu pylib <<<"$entry"
    build_target "$name" "$triple" "$cpu" "$pylib"
done

# --- macOS Universal Binary ---------------------------------------------------
# If both macOS targets were built, combine them into a universal binary.
if [ -f "$SCRIPT_DIR/lib/macos-aarch64/libimpro.a" ] && [ -f "$SCRIPT_DIR/lib/macos-x86_64/libimpro.a" ]; then
    if command -v lipo >/dev/null 2>&1; then
        printf 'Creating macos-universal...\n'
        mkdir -p "$SCRIPT_DIR/lib/macos-universal"
        lipo -create "$SCRIPT_DIR/lib/macos-aarch64/libimpro.a" \
                     "$SCRIPT_DIR/lib/macos-x86_64/libimpro.a" \
             -output "$SCRIPT_DIR/lib/macos-universal/libimpro.a"
        lipo -create "$SCRIPT_DIR/lib/macos-aarch64/libimpro_python.dylib" \
                     "$SCRIPT_DIR/lib/macos-x86_64/libimpro_python.dylib" \
             -output "$SCRIPT_DIR/lib/macos-universal/libimpro_python.dylib"
        printf '  [OK] %-16s -> lib/%s/{libimpro.a,libimpro_python.dylib}\n' "macos-universal" "macos-universal"
    else
        printf '  [..] skipping macos-universal (lipo not found)\n'
    fi
fi

printf 'Done. Artifacts under %s/lib and %s/include\n' "$SCRIPT_DIR" "$SCRIPT_DIR"
