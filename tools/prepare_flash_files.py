"""Prepare an ESP32-S3 flash plan that does not write the NVS partition."""
from pathlib import Path
import struct
import shutil
root=Path(__file__).resolve().parents[1]
out=root/'dist'
merged=(out/'YouyouSpace.bin').read_bytes()
partition_data=(out/'build/YouyouSpace.ino.partitions.bin').read_bytes()
nvs=[]
for pos in range(0,len(partition_data)-31,32):
    magic,kind,subtype,offset,size=struct.unpack_from('<HBBII',partition_data,pos)
    if magic!=0x50AA:continue
    if kind==1 and subtype==2:nvs.append((offset,offset+size))
if nvs!=[(0x9000,0xE000)]:
    raise RuntimeError(f'Unexpected NVS partition: {nvs}; flash plan needs review')
plan=[
    (0,'YouyouSpace.bootloader.bin','YouyouSpace.ino.bootloader.bin'),
    (0x8000,'YouyouSpace.partitions.bin','YouyouSpace.ino.partitions.bin'),
    (0x10000,'YouyouSpace.app.bin','YouyouSpace.ino.bin'),
]
ranges=[]
for address,name,source in plan:
    data=(out/'build'/source).read_bytes()
    if merged[address:address+len(data)]!=data:raise RuntimeError(f'Merge mismatch: {name}')
    (out/name).write_bytes(data)
    ranges.append((address,address+len(data),name))
bootapp=merged[0xE000:0x10000]
if len(bootapp)!=8192:raise RuntimeError('Missing boot_app0 image')
(out/'boot_app0.bin').write_bytes(bootapp)
ranges.append((0xE000,0x10000,'boot_app0.bin'))
# Check sector erase ranges as well as exact bytes.
for start,end,name in ranges:
    first=start//4096*4096
    last=(end+4095)//4096*4096
    if any(first<hi and last>lo for lo,hi in nvs):
        raise RuntimeError(f'{name} would erase NVS')
(out/'FLASH-PLAN.txt').write_text(
    'Default script uses separate images and leaves NVS 0x9000..0xDFFF untouched.\n'
    'Use flash-windows.cmd; the merged YouyouSpace.bin is a full-reset image.\n\n'+
    ''.join(f'0x{start:06x} {name} ({end-start} bytes)\n' for start,end,name in ranges))
shutil.copy(root/'tools/flash-windows.cmd',out/'flash-windows.cmd')
print('Verified split flash images and sector erase ranges: NVS is not overwritten')
