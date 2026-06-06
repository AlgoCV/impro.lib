# impro.c — ImPro C library desktop binaries

Prebuilt, **desktop-only**, `ReleaseFast` (stripped) binary distribution of the
[ImPro](https://github.com/AlgoCV/ipas.zig) image-processing C library, plus the
public C headers.

This repository carries **no source code** — only the compiled artifacts. It is
meant to be consumed as a **git submodule** by the language wrappers
(`impro.rust`, `impro.swift`, `impro.python`), so they can link against the C ABI
without a Zig toolchain.

## Layout

```
include/
  impro.h            Core grayscale + binary C API
  impro_types.h      Shared C types (Img256, ImgBw, enums, ...)
lib/
  <os>-<arch>/
    libimpro.a            Static C ABI archive   (Rust / Swift link target)
    libimpro_python.<ext> Dynamic bridge library (Python ctypes; .dylib/.so/.dll)
```

Supported desktop targets (`<os>-<arch>`):

| Directory        | Zig target            | CPU baseline |
|------------------|-----------------------|--------------|
| `macos-aarch64`  | `aarch64-macos-none`  | `apple_m1`   |
| `macos-x86_64`   | `x86_64-macos-none`   | `x86_64_v3`  |
| `linux-x86_64`   | `x86_64-linux-gnu`    | `x86_64_v3`  |
| `linux-aarch64`  | `aarch64-linux-gnu`   | `cortex_a76` |
| `windows-x86_64` | `x86_64-windows-gnu`  | `x86_64_v3`  |

## Regenerating

Requires a checkout of the `impro` source repository and a Zig `0.16.x`
toolchain:

```sh
IMPRO_SRC=../impro ./build.sh
```

See `build.sh` for the full target matrix and tunable knobs.
