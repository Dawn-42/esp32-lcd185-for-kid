#!/usr/bin/env bash
set -euo pipefail
# Pass an Arduino CLI configured with esp32 core 3.2.0 and lvgl 8.4.0.
task_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
task_cli="${ARDUINO_CLI:-arduino-cli}"
"$task_cli" compile \
  --fqbn 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=cdc,USBMode=hwcdc' \
  --build-property "compiler.c.extra_flags=-DLV_CONF_INCLUDE_SIMPLE -I$task_root/firmware/YouyouSpace" \
  --build-property "compiler.cpp.extra_flags=-DLV_CONF_INCLUDE_SIMPLE -I$task_root/firmware/YouyouSpace" \
  --output-dir "$task_root/dist/build" \
  "$task_root/firmware/YouyouSpace"
cp "$task_root/dist/build/YouyouSpace.ino.merged.bin" "$task_root/dist/YouyouSpace.bin"

python3 "$task_root/tools/prepare_flash_files.py"

# Keep only the NVS-preserving delivery by default. Opt in to ELF/full-image outputs.
if [[ "${YOUYOU_KEEP_BUILD:-0}" != "1" ]]; then
  python3 - "$task_root" <<'CLEAN_BUILD'
from pathlib import Path
import shutil,sys
out=Path(sys.argv[1])/'dist'
(out/'YouyouSpace.bin').unlink(missing_ok=True)
shutil.rmtree(out/'build')
CLEAN_BUILD
fi
