#!/bin/bash
# template.sh <name> <path/to/name>
# 新建一个 C 实现 + Rust wrapper 的叶子组件目录：
#   <path>/src/lib.rs  <path>/<name>.c  <path>/<name>.h
#   <path>/Makefile    <path>/Cargo.toml
set -e

if [ $# -ne 2 ]; then
    echo "usage: $0 <name> <path/to/name>" >&2
    exit 1
fi

name="$1"
dir="$2"

if ! [[ "$name" =~ ^[a-z][a-z0-9_]*$ ]]; then
    echo "error: name must match [a-z][a-z0-9_]* (used in C identifiers)" >&2
    exit 1
fi

if [ -e "$dir" ]; then
    echo "error: $dir already exists" >&2
    exit 1
fi

root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$dir/src"
# 组件 Makefile 里 SHYOS_ROOT 的相对深度按实际路径计算
relroot="$(realpath --relative-to="$dir" "$root")"

uname="$(echo "$name" | tr 'a-z' 'A-Z')"

cat > "$dir/Cargo.toml" <<EOF
[package]
name = "shyos-$name"
version = "0.0.0"
edition = "2024"
publish = false

[lib]
path = "src/lib.rs"
crate-type = ["rlib"]

[dependencies]

EOF

cat > "$dir/src/lib.rs" <<EOF
#![no_std]
EOF

cat > "$dir/$name.h" <<EOF
#ifndef SHYOS_${uname}_H
#define SHYOS_${uname}_H

#include "shy_type.h"

#endif /* SHYOS_${uname}_H */
EOF

cat > "$dir/$name.c" <<EOF
#include "$name.h"
EOF

cat > "$dir/Makefile" <<EOF
SHYOS_ROOT ?= \$(abspath $relroot)
include \$(SHYOS_ROOT)/mk/config.mk

OUT_DIR := \$(SHYOS_TARGET_DIR)
${uname}_C_OBJ := \$(OUT_DIR)/${name}_c.o

.PHONY: build clean cargo-check

build: \$(${uname}_C_OBJ)

\$(OUT_DIR):
	mkdir -p \$@

\$(${uname}_C_OBJ): $name.c $name.h \$(SHYOS_CONFIG_STATE) | \$(OUT_DIR)
	\$(CC) \$(CFLAGS) -c -o \$@ $name.c

cargo-check:
	\$(CARGO) check --manifest-path Cargo.toml --target \$(RUST_TARGET)

clean:
	rm -rf target
EOF

echo "created $dir"
