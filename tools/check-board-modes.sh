#!/usr/bin/env bash
# Compile each product board for Cortex-M0 in every implemented transport
# mode. Combinations that have no implementation are skipped and named.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cc="${CC:-arm-none-eabi-gcc}"

if ! command -v "$cc" >/dev/null 2>&1; then
    echo "skip: $cc not found; board mode matrix not compiled"
    exit 0
fi

soc="$root/infra/soc/v85xxp/v8500"
freertos="${FREERTOS_KERNEL_DIR:-$root/infra/third_party/freertos/FreeRTOS-KernelV11.3.3}"

common_includes=(
    -I "$root"
    -I "$root/infra/kernel"
    -I "$root/application/adapters/dl698"
    -I "$soc"
    -I "$soc/vendor"
    -I "$soc/vendor/Lib_Driver/inc"
    -I "$root/infra/third_party/arm/cmsis"
)

flags=(-std=gnu11 -mcpu=cortex-m0 -mthumb -ffreestanding -Os -g
       -ffunction-sections -fdata-sections
       -Wno-error=implicit-function-declaration)

status=0

board_sources() {
    local board="$1"
    if [ -f "$board/CMakeLists.txt" ]; then
        sed -n '/set(BOARD_SOURCES/,/)/p' "$board/CMakeLists.txt" \
            | tr ' ()' '\n\n\n' | grep '\.c$' | sed "s|^|$board/|"
    else
        find "$board" -maxdepth 1 -name '*.c' | sort
    fi
}

compile_mode() {
    local board="$1"
    local name="$2"
    local extra="$3"
    local defines="$4"
    local mode_status=0
    local source

    # shellcheck disable=SC2206
    local include_args=(${extra})
    echo "== $(basename "$(dirname "$board")")/$name: $defines"
    while IFS= read -r source; do
        if ! "$cc" "${flags[@]}" "${common_includes[@]}" \
             -I "$board" -I "$(dirname "$board")" -I "$(dirname "$board")/app" \
             "${include_args[@]}" ${defines} -c "$source" -o /dev/null; then
            echo "FAIL $name $source"
            mode_status=1
        fi
    done < <(board_sources "$board")
    if [ "$mode_status" -eq 0 ]; then
        echo "ok $name"
    else
        status=1
    fi
}

run_board() {
    local board="$1"
    local freertos_extra=""
    local base_defines=""
    local target_dir
    target_dir="$(dirname "$board")"

    if [ "$(basename "$target_dir")" = "idis_ip_meter_freertos" ]; then
        if [ ! -f "$freertos/tasks.c" ]; then
            echo "skip: $(basename "$target_dir") needs a FreeRTOS kernel checkout"
            return
        fi
        freertos_extra="-I $freertos/include -I $freertos/portable/GCC/ARM_CM0 -I $target_dir/app"
        base_defines="-DND_BOARD_USE_FREERTOS=1"
    fi
    if [ ! -d "$target_dir" ]; then
        echo "FAIL: missing $target_dir"
        status=1
        return
    fi

    compile_mode "$board" "POLL" "$freertos_extra" \
        "$base_defines -DND_BOARD_UART_MODE=0 -DND_BOARD_SPI_MODE=0 -DND_BOARD_IIC_MODE=0"
    compile_mode "$board" "UART-IRQ" "$freertos_extra" \
        "$base_defines -DND_BOARD_UART_MODE=1 -DND_BOARD_SPI_MODE=0 -DND_BOARD_IIC_MODE=0"
    compile_mode "$board" "SPI-DMA" "$freertos_extra" \
        "$base_defines -DND_BOARD_UART_MODE=0 -DND_BOARD_SPI_MODE=2 -DND_BOARD_IIC_MODE=0"
    compile_mode "$board" "IRQ-DMA" "$freertos_extra" \
        "$base_defines -DND_BOARD_UART_MODE=1 -DND_BOARD_SPI_MODE=2 -DND_BOARD_IIC_MODE=0"
    compile_mode "$board" "IIC-STUB" "$freertos_extra" \
        "$base_defines -DND_BOARD_UART_MODE=0 -DND_BOARD_SPI_MODE=0 -DND_BOARD_IIC_MODE=2"
}

run_board "$root/targets/stategrid_meter/board"
run_board "$root/targets/idis_ip_meter/board"
run_board "$root/targets/idis_ip_meter_freertos/board"

echo "skip: UART-DMA (RN8209 byte protocol has no buffer API)"
echo "skip: SPI-IRQ (no byte-interrupt transport; ESAM uses DMA, flash stays bit-bang)"
echo "skip: IIC-IRQ (compiles the stub path, not a transport)"
echo "skip: IIC-DMA (same stub path, not a transport)"

exit "$status"
