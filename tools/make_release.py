"""Package source, previews and any successfully compiled firmware for Windows."""
from pathlib import Path
import hashlib
import zipfile
root=Path(__file__).resolve().parents[1]
out=root/'dist'
paths=[root/'README.md']
for folder in ['firmware','sd-card','tools','assets']:
    paths.extend(p for p in (root/folder).rglob('*') if p.is_file() and not any(part in {'__pycache__','.cache','.pytest_cache','build'} for part in p.parts) and p.suffix not in {'.pyc','.ppm','.log','.part','.tmp'})
paths.extend(out/name for name in ['preview.png','pet-walk.png','pet-memories.png','pixel-shenzhen.png'] if (out/name).exists())
paths.extend(p for pattern in ['pet-care.mp4','pet-care.gif','pet-room.mp4','pet-room.gif','pixel-shenzhen.mp4','pixel-shenzhen.gif'] for p in out.glob(pattern))
paths.extend(p for pattern in ['BUILD-STATUS.txt','PERFORMANCE.txt'] for p in out.glob(pattern))
# Package the verified, NVS-preserving split images directly.
paths.append(out/'flash-windows.cmd')
for name in ['YouyouSpace.bootloader.bin','YouyouSpace.partitions.bin','YouyouSpace.app.bin','boot_app0.bin','FLASH-PLAN.txt']:
    p=out/name
    if not p.exists():raise RuntimeError('Run tools/prepare_flash_files.py first: '+name)
    paths.append(p)
manifest=''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(root))+'\n' for p in paths)
(out/'SHA256SUMS.txt').write_text(manifest)
with zipfile.ZipFile(out/'YouyouSpace-v8.zip','w',zipfile.ZIP_DEFLATED) as z:
    for p in paths+[out/'SHA256SUMS.txt']:
        z.write(p,Path('YouyouSpace')/p.relative_to(root))
print(out/'YouyouSpace-v8.zip')
