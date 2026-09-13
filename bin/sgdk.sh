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

    local in_deps=0
    local deps=()

    while IFS= read -r line || [ -n "$line" ]; do
        local clean_line="${line%%#*}"
        local trimmed
        trimmed="$(echo "$clean_line" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
        [ -z "$trimmed" ] && continue

        if [[ "$trimmed" =~ ^(dependencies|deps): ]]; then
            in_deps=1
            continue
        fi

        if [ "$in_deps" -eq 1 ]; then
            if [[ "$line" =~ ^[^\ [[:space:]]] ]]; then
                in_deps=0
                continue
            fi

            if [[ "$trimmed" =~ ^-[[:space:]]*(.+)$ ]]; then
                local val="${BASH_REMATCH[1]}"
                val="$(echo "$val" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e 's/^["'\''"]//' -e 's/["'\''"]$//')"
                if [ -n "$val" ]; then
                    val="${val//#/@}"
                    deps+=("$val")
                fi
            fi
        fi
    done < "$yml_file"

    echo "${deps[*]}"
}

init_project() {
    local destination="${1:-$PWD}"
    local template="$GDK/project/template"

    if [ "$#" -gt 1 ]; then
        echo "[ERROR] init accepts at most one destination directory." >&2
        return 1
    fi
    if [ ! -d "$template" ]; then
        echo "[ERROR] Cannot find SGDK project template at: $template" >&2
        return 1
    fi
    if [ ! -e "$destination" ]; then
        mkdir -p "$destination" || return 1
    elif [ ! -d "$destination" ]; then
        echo "[ERROR] Init destination is not a directory: $destination" >&2
        return 1
    elif [ "$(find "$destination" -mindepth 1 -maxdepth 1 ! -name '$RECYCLE.BIN' -print -quit)" ]; then
        echo "[ERROR] Init destination is not empty: $destination" >&2
        return 1
    fi

    cp -R "$template"/. "$destination"/ || return 1
    echo "[SGDK] Initialized project in: $destination"
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

    local in_builds=0
    local in_configs=0
    local current_bname=""
    local current_configs=()

    flush_build() {
        if [ -n "$current_bname" ]; then
            local cfg_str=""
            local first=1
            for cfg in "${current_configs[@]}"; do
                if [ "$first" -eq 1 ]; then
                    cfg_str="$cfg"
                    first=0
                else
                    cfg_str="$cfg_str;$cfg"
                fi
            done
            echo "$current_bname|$cfg_str"
        fi
    }

    while IFS= read -r line || [ -n "$line" ]; do
        local clean_line="${line%%#*}"
        local trimmed
        trimmed="$(echo "$clean_line" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
        [ -z "$trimmed" ] && continue

        if [[ "$clean_line" =~ ^[a-zA-Z0-9_]+: ]]; then
            if [[ "$trimmed" =~ ^builds: ]]; then
                in_builds=1
            else
                if [ "$in_builds" -eq 1 ]; then
                    flush_build
                    current_bname=""
                    current_configs=()
                fi
                in_builds=0
                in_configs=0
            fi
            continue
        fi

        if [ "$in_builds" -eq 1 ]; then
            if [[ "$trimmed" =~ ^-[[:space:]]*name:[[:space:]]*(.+)$ ]]; then
                flush_build
                current_bname="${BASH_REMATCH[1]}"
                current_bname="$(echo "$current_bname" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e 's/^["'\''"]//' -e 's/["'\''"]$//')"
                current_configs=()
                in_configs=0
                continue
            fi

            if [[ "$trimmed" =~ ^sgdkConfigs: ]]; then
                in_configs=1
                continue
            fi

            if [ "$in_configs" -eq 1 ]; then
                if [[ "$trimmed" =~ ^-[[:space:]]*([A-Za-z0-9_]+):[[:space:]]*(.+)$ ]]; then
                    local key="${BASH_REMATCH[1]}"
                    local val="${BASH_REMATCH[2]}"
                    key="$(echo "$key" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
                    val="$(echo "$val" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e 's/^["'\''"]//' -e 's/["'\''"]$//')"
                    current_configs+=("$key=$val")
                fi
            fi
        fi
    done < "$yml_file"

    if [ "$in_builds" -eq 1 ]; then
        flush_build
    fi
}

# Parse project name from sgdk.yml if present
parse_sgdk_yml_name() {
    local yml_file="sgdk.yml"
    [ -f "$yml_file" ] || return 0

    while IFS= read -r line || [ -n "$line" ]; do
        local clean_line="${line%%#*}"
        local trimmed
        trimmed="$(echo "$clean_line" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
        [ -z "$trimmed" ] && continue

        if [[ "$trimmed" =~ ^name:[[:space:]]*(.+)$ ]]; then
            local val="${BASH_REMATCH[1]}"
            val="$(echo "$val" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e 's/^["'\''"]//' -e 's/["'\''"]$//')"
            echo "$val"
            return 0
        fi
    done < "$yml_file"
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
    PROJECT_NAME=$(parse_sgdk_yml_name)
    OUT_PREFIX="${PROJECT_NAME:-rom}"

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
                $GDK/bin/make -C "$GDK" -f makelib.gen "clean-$LIB_TARGET"
                $GDK/bin/make -C "$GDK" -f makelib.gen "$LIB_TARGET"
            fi

            echo "[SGDK] Executing project build target '$target'..."
            $GDK/bin/make -f "$MAKEFILE_GEN" clean "${extra_args[@]}"
            if [ -n "$DEPENDENCIES" ]; then
                $GDK/bin/make -f "$MAKEFILE_GEN" "$target" DEPENDENCIES="$DEPENDENCIES" "${extra_args[@]}"
            else
                $GDK/bin/make -f "$MAKEFILE_GEN" "$target" "${extra_args[@]}"
            fi

            if [ -f "out/rom.bin" ]; then
                cp "out/rom.bin" "out/${OUT_PREFIX}-${bname}.bin"
                echo "[SGDK] Created output artifact: out/${OUT_PREFIX}-${bname}.bin"
            elif [ -f "out/$target/rom.bin" ]; then
                cp "out/$target/rom.bin" "out/$target/${OUT_PREFIX}-${bname}.bin"
                echo "[SGDK] Created output artifact: out/$target/${OUT_PREFIX}-${bname}.bin"
            fi
        done <<< "$YML_BUILDS"

        restore_sgdk_config_h
    else
        restore_sgdk_config_h
        echo "[SGDK] Executing build target '$target'..."
        if [ -n "$DEPENDENCIES" ]; then
            $GDK/bin/make -f "$MAKEFILE_GEN" "$target" DEPENDENCIES="$DEPENDENCIES" "${extra_args[@]}"
        else
            $GDK/bin/make -f "$MAKEFILE_GEN" "$target" "${extra_args[@]}"
        fi

        if [ -f "out/rom.bin" ]; then
            cp "out/rom.bin" "out/${OUT_PREFIX}.bin"
            echo "[SGDK] Created output artifact: out/${OUT_PREFIX}.bin"
        elif [ -f "out/$target/rom.bin" ]; then
            cp "out/$target/rom.bin" "out/$target/${OUT_PREFIX}.bin"
            echo "[SGDK] Created output artifact: out/$target/${OUT_PREFIX}.bin"
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
    echo "  init                      Initialize folder with SGDK project structure"
    echo "  build, compile [target]   Build project (default target: release)"
    echo "  release                   Build project in release mode"
    echo "  debug                     Build project in debug mode (with symbols)"
    echo "  asm                       Generate assembly output"
    echo "  clean [target]            Clean build output (targets: all, release, debug, asm)"
    echo "  rebuild [target]          Clean and rebuild project"
    echo "  deps, install             Fetch and clone dependencies defined in sgdk.yml"
    echo "  lib, build-lib [target]   Build SGDK library itself"
    echo "  run                       Launch ROM in emulator"
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
    if [ -x "$GDK/bin/make" ]; then
        echo "Make executable: found ($GDK/bin/make)"
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
        $GDK/bin/make -f "$MAKEFILE_GEN" install DEPENDENCIES="$DEPENDENCIES" "$@"
        ;;
    init)
        shift
        init_project "$@"
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
        $GDK/bin/make -f "$MAKEFILE_GEN" "$CLEAN_TARGET" "$@"
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
        $GDK/bin/make -f "$MAKELIB_GEN" "$LIB_TARGET" "$@"
        ;;
    run)
        shift
        ROM_PATH="${1:-}"
        PROJECT_NAME=$(parse_sgdk_yml_name)
        OUT_PREFIX="${PROJECT_NAME:-rom}"

        if [ -z "$ROM_PATH" ]; then
            if [ -f "out/${OUT_PREFIX}.bin" ]; then
                ROM_PATH="out/${OUT_PREFIX}.bin"
            elif [ -f "out/release/${OUT_PREFIX}.bin" ]; then
                ROM_PATH="out/release/${OUT_PREFIX}.bin"
            elif [ -f "out/debug/${OUT_PREFIX}.bin" ]; then
                ROM_PATH="out/debug/${OUT_PREFIX}.bin"
            elif [ -f "out/rom.bin" ]; then
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
