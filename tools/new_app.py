#!/usr/bin/env python3
"""Create a mixed C/Rust ShyOS app image project."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


APP_NAME = re.compile(r"^[A-Za-z][A-Za-z0-9_-]*$")


def write(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("path", type=Path)
    args = parser.parse_args()

    root = args.root.resolve()
    requested = args.path
    app_dir = (
        (root / requested).resolve()
        if requested.parts and requested.parts[0] == "app"
        else (root / "app" / requested).resolve()
    )
    app_root = (root / "app").resolve()

    if app_dir.parent != app_root:
        parser.error("app path must be app/<name>")
    if not APP_NAME.fullmatch(app_dir.name):
        parser.error("app name must contain only letters, digits, '_' or '-'")
    if app_dir.exists():
        parser.error(f"{app_dir} already exists")

    app_dir.mkdir()
    crate_name = f"shyos_{app_dir.name.replace('-', '_')}"
    package_name = f"shyos-{app_dir.name.replace('_', '-')}"

    write(
        app_dir / "shyos.mk",
        (root / "config" / "shyos.mk").read_text(encoding="utf-8"),
    )
    write(
        app_dir / "Cargo.toml",
        f"""[package]
name = "{package_name}"
version = "0.0.0"
edition = "2021"
publish = false

[lib]
name = "{crate_name}"
path = "main.rs"
crate-type = ["rlib", "staticlib"]

[dependencies]
shyos-data-structure = {{ path = "../../lib/04data_structure" }}
""",
    )
    write(
        app_dir / "Makefile",
        f"""SHYOS_ROOT ?= $(abspath ../..)
SHYOS_CONFIG ?= $(CURDIR)/shyos.mk
include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := target
APP_C_OBJ := $(OUT_DIR)/main_c.o
RUST_STATICLIB := \\
\t$(CARGO_TARGET_DIR)/$(RUST_TARGET)/$(CARGO_PROFILE)/lib{crate_name}.a

$(OUT_DIR):
\tmkdir -p $@

$(APP_C_OBJ): main.c $(SHYOS_CONFIG_STATE) | $(OUT_DIR)
\t$(CC) $(CFLAGS) -c -o $@ $<

$(RUST_STATICLIB): Cargo.toml main.rs $(SHYOS_RUST_INPUTS) \\
\t\t$(SHYOS_CONFIG_STATE) | $(OUT_DIR)
\t$(CARGO) build \\
\t\t--manifest-path Cargo.toml \\
\t\t--target $(RUST_TARGET) \\
\t\t$(CARGO_BUILD_STD_FLAGS) \\
\t\t$(CARGO_PROFILE_ARGS)
\ttouch $@

IMAGE_NAME := app
IMAGE_INPUTS := $(APP_C_OBJ)
IMAGE_RUST_STATICLIB := $(RUST_STATICLIB)
IMAGE_EMIT_BIN := 1

include $(SHYOS_ROOT)/mk/app.mk
""",
    )
    write(
        app_dir / "main.c",
        """#include "shy_type.h"

void rust_main(void);

i32 main(void)
{
\trust_main();
\treturn 0;
}
""",
    )
    write(
        app_dir / "main.rs",
        """#![no_std]

use core::panic::PanicInfo;

#[unsafe(no_mangle)]
pub extern "C" fn rust_main() {}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
""",
    )

    print(app_dir.relative_to(root))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
