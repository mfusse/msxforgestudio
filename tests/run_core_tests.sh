#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
"$CC" -std=c11 -O2 -Icore core/engine.c core/rom.c core/msx2_graphics.c tests/test_060_engine_contract.c -o tests/.test_060_engine_contract
./tests/.test_060_engine_contract examples/Demo_0594_NPC_Duplicado.msxf
rm -f tests/.test_060_engine_contract
python3 tests/test_source_contract.py

python3 tests/test_0602_source.py
