#!/usr/bin/env bash

# ============================================================================
# SGDK Command Line Interface (Linux / macOS)
# Compatible with SGDK build workflow (Maven / Gradle / NPM style)
# ============================================================================

set -e

# Resolve SGDK root directory (GDK)
if [ -z "$GDK" ]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    export GDK="$(cd "$SCRIPT_DIR/.." && pwd)"
fi

MAKEFILE_GEN="$GDK/makefile.gen"
MAKELIB_GEN="$GDK/makelib.gen"

# Add SGDK bin directory to PATH if not present
if [[ ":$PATH:" != *":$GDK/bin:"* ]]; then
    export PATH="$GDK/bin:$PATH"
fi

CMD="${1:-}"

# Parse dependencies from sgdk.yml if present
parse_sgdk_yml_deps() {
    local yml_file="sgdk.yml"
    [ -f "$yml_file" ] || return 0

    if command -v python3 >/dev/null 2>&1; then
        python3 - "$yml_file" << 'EOF'
import sys, re
filepath = sys.argv[1]
try:
    with open(filepath, 'r') as f:
        lines = f.readlines()
except Exception:
    sys.exit(0)

in_deps = False
deps = []

for line in lines:
    clean_line = re.sub(r'\s+#.*$', '', line)
    trimmed = clean_line.strip()
    if not trimmed or trimmed.startswith('#'):
        continue
    if re.match(r'^(dependencies|deps)\s*:', trimmed):
        in_deps = True
        continue
    if in_deps and re.match(r'^[^\s#]', line):
        in_deps = False
    if not in_deps:
        continue

    m = re.match(r'^-\s*(.+)$', trimmed)
    if m:
        val = m.group(1).strip().strip('"').strip("'")
        if val:
            deps.append(val.replace('#', '@'))

print(" ".join(deps))
EOF
    elif command -v perl >/dev/null 2>&1; then
        perl - "$yml_file" << 'EOF'
use strict;
use warnings;

my $file = $ARGV[0];
open(my $fh, '<', $file) or exit 0;

my $in_deps = 0;
my @deps;

while (my $line = <$fh>) {
    my $clean_line = $line;
    $clean_line =~ s/\s+#.*$//;
    my $trimmed = $clean_line;
    $trimmed =~ s/^\s+|\s+$//g;
    next if $trimmed eq '' || $trimmed =~ /^#/;

    if ($trimmed =~ /^(dependencies)\s*:/) {
        $in_deps = 1;
        next;
    }
    if ($in_deps && $line =~ /^[^\s#]/) {
        $in_deps = 0;
    }
    next unless $in_deps;

    if ($trimmed =~ /^-\s*(.+)$/) {
        my $val = $1;
        $val =~ s/^\s+|\s+$//g;
        $val =~ s/^["']|["']$//g;
        if ($val ne '') {
            $val =~ s/#/@/;
            push @deps, $val;
        }
    }
}
close($fh);
print join(" ", @deps);
EOF
    fi
}

YML_DEPS=$(parse_sgdk_yml_deps)
if [ -n "$YML_DEPS" ]; then
    echo "[SGDK] Found dependencies in sgdk.yml: $YML_DEPS"
    if [ -z "$DEPENDENCIES" ]; then
        DEPENDENCIES="$YML_DEPS"
    else
        DEPENDENCIES="$DEPENDENCIES $YML_DEPS"
    fi
fi

# Parse builds and sgdkConfigs from sgdk.yml if present
parse_sgdk_yml_builds() {
    local yml_file="sgdk.yml"
    [ -f "$yml_file" ] || return 0

    if command -v python3 >/dev/null 2>&1; then
        python3 - "$yml_file" << 'EOF'
import sys, re

filepath = sys.argv[1]
try:
    with open(filepath, 'r') as f:
        lines = f.readlines()
except Exception:
    sys.exit(0)

in_builds = False
in_configs = False
builds = []
current_build = None

for line in lines:
    clean_line = re.sub(r'\s+#.*$', '', line)
    trimmed = clean_line.strip()
    if not trimmed or trimmed.startswith('#'):
        continue
    if re.match(r'^[a-zA-Z0-9_]+\s*:', trimmed):
        if re.match(r'^builds\s*:', trimmed):
            in_builds = True
        else:
            in_builds = False
            in_configs = False
        continue
    if not in_builds:
        continue

    m_name = re.match(r'^-\s*name\s*:\s*(.+)$', trimmed)
    if m_name:
        bname = m_name.group(1).strip().strip('"').strip("'")
        current_build = (bname, [])
        builds.append(current_build)
        in_configs = False
        continue

    if re.match(r'^sgdkConfigs\s*:', trimmed):
        in_configs = True
        continue

    if in_configs and current_build:
        m_cfg = re.match(r'^-\s*([A-Za-z0-9_]+)\s*:\s*(.+)$', trimmed)
        if m_cfg:
            key = m_cfg.group(1).strip()
            val = m_cfg.group(2).strip().strip('"').strip("'")
            current_build[1].append((key, val))

for bname, configs in builds:
    cfg_str = ";".join([f"{k}={v}" for k, v in configs])
    print(f"{bname}|{cfg_str}")
EOF
    elif command -v perl >/dev/null 2>&1; then
        perl - "$yml_file" << 'EOF'
use strict;
use warnings;

my $file = $ARGV[0];
open(my $fh, '<', $file) or exit 0;

my $in_builds = 0;
my $in_configs = 0;
my @builds;
my $current_build;

while (my $line = <$fh>) {
    my $clean_line = $line;
    $clean_line =~ s/\s+#.*$//;
    my $trimmed = $clean_line;
    $trimmed =~ s/^\s+|\s+$//g;
    next if $trimmed eq '' || $trimmed =~ /^#/;

    if ($trimmed =~ /^[a-zA-Z0-9_]+\s*:/) {
        if ($trimmed =~ /^builds\s*:/) {
            $in_builds = 1;
        } else {
            $in_builds = 0;
            $in_configs = 0;
        }
        next;
    }
    next unless $in_builds;

    if ($trimmed =~ /^-\s*name\s*:\s*(.+)$/) {
        my $bname = $1;
        $bname =~ s/^\s+|\s+$//g;
        $bname =~ s/^["']|["']$//g;
        $current_build = { name => $bname, configs => [] };
        push @builds, $current_build;
        $in_configs = 0;
        next;
    }

    if ($trimmed =~ /^sgdkConfigs\s*:/) {
        $in_configs = 1;
        next;
    }

    if ($in_configs && $current_build) {
        if ($trimmed =~ /^-\s*([A-Za-z0-9_]+)\s*:\s*(.+)$/) {
            my $key = $1;
            my $val = $2;
            $key =~ s/^\s+|\s+$//g;
            $val =~ s/^\s+|\s+$//g;
            $val =~ s/^["']|["']$//g;
            push @{$current_build->{configs}}, "$key=$val";
        }
    }
}
close($fh);

for my $b (@builds) {
    my $cfg_str = join(";", @{$b->{configs}});
    print "$b->{name}|$cfg_str\n";
}
EOF
    fi
}

apply_sgdk_config_key_val() {
    local key="$1"
    local val="$2"
    local config_h="$GDK/inc/config.h"
    if [ -f "$config_h" ]; then
        local safe_val=$(echo "$val" | sed 's/\//\\\//g')
        if [[ "$OSTYPE" == "darwin"* ]]; then
            sed -i '' "s/#define ${key} .*/#define ${key}         ${safe_val}/g" "$config_h"
        else
            sed -i "s/#define ${key} .*/#define ${key}         ${safe_val}/g" "$config_h"
        fi
    fi
}

restore_sgdk_config_h() {
    if [ -f "$GDK/inc/config.h_original" ]; then
        cp "$GDK/inc/config.h_original" "$GDK/inc/config.h"
    fi
}

run_build_target() {
    local target="$1"
    shift
    local extra_args=("$@")

    if [ ! -f "$MAKEFILE_GEN" ]; then
        echo "[ERROR] Cannot find SGDK makefile.gen at: $MAKEFILE_GEN" >&2
        exit 1
    fi

    YML_BUILDS=$(parse_sgdk_yml_builds)

    if [ -n "$YML_BUILDS" ]; then
        if [ ! -f "$GDK/inc/config.h_original" ] && [ -f "$GDK/inc/config.h" ]; then
            cp "$GDK/inc/config.h" "$GDK/inc/config.h_original"
        fi

        LIB_TARGET="release"
        if [ "$target" = "debug" ]; then
            LIB_TARGET="debug"
        fi

        while IFS='|' read -r bname configs; do
            [ -n "$bname" ] || continue
            echo ""
            echo "============================================================================"
            echo "[SGDK] Building configuration: $bname"
            echo "============================================================================"

            restore_sgdk_config_h

            if [ -n "$configs" ]; then
                IFS=';' read -ra CFG_ARR <<< "$configs"
                for pair in "${CFG_ARR[@]}"; do
                    key="${pair%%=*}"
                    val="${pair#*=}"
                    apply_sgdk_config_key_val "$key" "$val"
                done

                echo "[SGDK] Rebuilding SGDK library ($LIB_TARGET) for build '$bname'..."
                make -C "$GDK" -f makelib.gen "clean-$LIB_TARGET"
                make -C "$GDK" -f makelib.gen "$LIB_TARGET"
            fi

            echo "[SGDK] Executing project build target '$target'..."
            make -f "$MAKEFILE_GEN" clean "${extra_args[@]}"
            if [ -n "$DEPENDENCIES" ]; then
                make -f "$MAKEFILE_GEN" "$target" DEPENDENCIES="$DEPENDENCIES" "${extra_args[@]}"
            else
                make -f "$MAKEFILE_GEN" "$target" "${extra_args[@]}"
            fi

            mkdir -p output
            if [ -f "out/rom.bin" ]; then
                cp "out/rom.bin" "output/${bname}.bin"
                cp "out/rom.bin" "out/rom-${bname}.bin"
                echo "[SGDK] Created output artifact: output/${bname}.bin"
            elif [ -f "out/$target/rom.bin" ]; then
                cp "out/$target/rom.bin" "output/${bname}.bin"
                cp "out/$target/rom.bin" "out/$target/rom-${bname}.bin"
                echo "[SGDK] Created output artifact: output/${bname}.bin"
            fi
        done <<< "$YML_BUILDS"

        restore_sgdk_config_h
    else
        restore_sgdk_config_h
        echo "[SGDK] Executing build target '$target'..."
        if [ -n "$DEPENDENCIES" ]; then
            make -f "$MAKEFILE_GEN" "$target" DEPENDENCIES="$DEPENDENCIES" "${extra_args[@]}"
        else
            make -f "$MAKEFILE_GEN" "$target" "${extra_args[@]}"
        fi
    fi
}

show_help() {
    echo ""
    echo "SGDK Command Line Interface (CLI)"
    echo ""
    echo "Usage: sgdk <command> [target] [options]"
    echo ""
    echo "Commands:"
    echo "  build, compile [target]   Build project (default target: release)"
    echo "  release                   Build project in release mode"
    echo "  debug                     Build project in debug mode (with symbols)"
    echo "  asm                       Generate assembly output"
    echo "  clean [target]            Clean build output (targets: all, release, debug, asm)"
    echo "  rebuild [target]          Clean and rebuild project"
    echo "  deps, install             Fetch and clone dependencies defined in sgdk.yml"
    echo "  lib, build-lib [target]   Build SGDK library itself"
    echo "  run, test [rom_path]      Launch ROM in emulator"
    echo "  version, -v, --version    Display SGDK and toolchain version information"
    echo "  help, -h, --help          Display this help message"
    echo ""
    echo "Targets:"
    echo "  release                   Optimized release build (default)"
    echo "  debug                     Debug build with symbol injection"
    echo "  asm                       Assembly listing target"
    echo ""
    echo "Examples:"
    echo "  sgdk build"
    echo "  sgdk deps"
    echo "  sgdk build debug -j4"
    echo "  sgdk clean"
    echo "  sgdk rebuild release"
    echo "  sgdk run"
    echo "  sgdk lib"
    echo ""
}

show_version() {
    echo "SGDK CLI Version 2.11"
    echo "GDK Directory: $GDK"
    if command -v make >/dev/null 2>&1; then
        echo "Make executable: found ($(command -v make))"
    else
        echo "Make executable: NOT found"
    fi

    if command -v m68k-elf-gcc >/dev/null 2>&1; then
        echo "GCC compiler: m68k-elf-gcc found ($(command -v m68k-elf-gcc))"
    elif command -v gcc >/dev/null 2>&1; then
        echo "GCC compiler: gcc found ($(command -v gcc))"
    else
        echo "GCC compiler: NOT found"
    fi
}

case "$CMD" in
    ""|help|-h|--help)
        show_help
        exit 0
        ;;
    version|-v|--version)
        show_version
        exit 0
        ;;
    deps|dependencies|install)
        shift
        if [ ! -f "$MAKEFILE_GEN" ]; then
            echo "[ERROR] Cannot find SGDK makefile.gen at: $MAKEFILE_GEN" >&2
            exit 1
        fi
        echo "[SGDK] Installing dependencies..."
        if [ -z "$DEPENDENCIES" ]; then
            echo "[SGDK] No dependencies specified in sgdk.yml or DEPENDENCIES variable."
            exit 0
        fi
        make -f "$MAKEFILE_GEN" install DEPENDENCIES="$DEPENDENCIES" "$@"
        ;;
    build|compile)
        shift
        TARGET="release"
        if [ "${1:-}" = "release" ] || [ "${1:-}" = "debug" ] || [ "${1:-}" = "asm" ]; then
            TARGET="$1"
            shift
        fi
        run_build_target "$TARGET" "$@"
        ;;
    release)
        shift
        run_build_target "release" "$@"
        ;;
    debug)
        shift
        run_build_target "debug" "$@"
        ;;
    asm)
        shift
        run_build_target "asm" "$@"
        ;;
    clean)
        shift
        CLEAN_TARGET="clean"
        if [ "${1:-}" = "release" ]; then
            CLEAN_TARGET="clean-release"
            shift
        elif [ "${1:-}" = "debug" ]; then
            CLEAN_TARGET="clean-debug"
            shift
        elif [ "${1:-}" = "asm" ]; then
            CLEAN_TARGET="clean-asm"
            shift
        elif [ "${1:-}" = "all" ]; then
            CLEAN_TARGET="clean-all"
            shift
        fi
        if [ ! -f "$MAKEFILE_GEN" ]; then
            echo "[ERROR] Cannot find SGDK makefile.gen at: $MAKEFILE_GEN" >&2
            exit 1
        fi
        restore_sgdk_config_h
        echo "[SGDK] Executing clean target '$CLEAN_TARGET'..."
        make -f "$MAKEFILE_GEN" "$CLEAN_TARGET" "$@"
        ;;
    rebuild)
        shift
        TARGET="release"
        if [ "${1:-}" = "release" ] || [ "${1:-}" = "debug" ] || [ "${1:-}" = "asm" ]; then
            TARGET="$1"
            shift
        fi
        run_build_target "$TARGET" "$@"
        ;;
    lib|build-lib)
        shift
        LIB_TARGET="release"
        if [ "${1:-}" = "release" ] || [ "${1:-}" = "debug" ] || [ "${1:-}" = "clean" ]; then
            LIB_TARGET="$1"
            shift
        fi
        if [ ! -f "$MAKELIB_GEN" ]; then
            echo "[ERROR] Cannot find SGDK makelib.gen at: $MAKELIB_GEN" >&2
            exit 1
        fi
        echo "[SGDK] Building library target '$LIB_TARGET'..."
        make -f "$MAKELIB_GEN" "$LIB_TARGET" "$@"
        ;;
    run|test)
        shift
        ROM_PATH="${1:-}"
        if [ -z "$ROM_PATH" ]; then
            if [ -f "out/rom.bin" ]; then
                ROM_PATH="out/rom.bin"
            elif [ -f "out/release/rom.bin" ]; then
                ROM_PATH="out/release/rom.bin"
            elif [ -f "out/debug/rom.bin" ]; then
                ROM_PATH="out/debug/rom.bin"
            else
                echo "[ERROR] No ROM file found in out/ or out/release/. Please build first or specify ROM path." >&2
                exit 1
            fi
        fi

        if [ ! -f "$ROM_PATH" ]; then
            echo "[ERROR] Specified ROM file does not exist: $ROM_PATH" >&2
            exit 1
        fi

        EMU="${EMULATOR:-}"
        if [ -z "$EMU" ]; then
            if command -v blastem >/dev/null 2>&1; then
                EMU="blastem"
            elif command -v gens >/dev/null 2>&1; then
                EMU="gens"
            elif command -v fusion >/dev/null 2>&1; then
                EMU="fusion"
            elif command -v mednafen >/dev/null 2>&1; then
                EMU="mednafen"
            fi
        fi

        if [ -z "$EMU" ]; then
            echo "[ERROR] No Sega Genesis emulator found in PATH." >&2
            echo "Please set the EMULATOR environment variable or install blastem/gens/fusion/mednafen." >&2
            exit 1
        fi

        echo "[SGDK] Running ROM with $EMU: $ROM_PATH"
        "$EMU" "$ROM_PATH"
        ;;
    *)
        echo "[ERROR] Unknown command: '$CMD'" >&2
        echo "Run 'sgdk help' for available commands." >&2
        exit 1
        ;;
esac
