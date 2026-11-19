"""Compose the 20 native framebuffer shots; export separate PNGs only on request."""
from pathlib import Path
import argparse
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--input-dir',type=Path,default=root/'dist',help='Directory containing the 20 native .ppm shots')
parser.add_argument('--output',type=Path,default=root/'dist/preview.png',help='Combined preview PNG')
parser.add_argument('--export-scenes',type=Path,help='Optional directory for individual PNGs; omitted by default')
args=parser.parse_args()
labels=['首页入口','喂小A吃鱼','月亮探险','装饰生日蛋糕','烟花与惊喜','追球小游戏','哄小A睡觉','选择三个星球','火星救援','彩环颜色任务','点亮再许愿','六张探险贴纸','银渐层幼崽的小窝','洗澡：打泡泡','洗澡：冲洗','洗澡：擦干','点击铲屎','成长：小猫','成长：成猫','吃饱后的提示']
files=sorted(args.input_dir.glob('[0-9][0-9]-*.ppm'))
if len(files)!=len(labels):parser.error(f'Expected {len(labels)} PPM shots in {args.input_dir}, found {len(files)}; run youyou_preview first')
sheet=Image.new('RGB',(1520,2000),'#eef1f5')
font=ImageFont.truetype('/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc',22)
if args.export_scenes:args.export_scenes.mkdir(parents=True,exist_ok=True)
for i,p in enumerate(files):
    with Image.open(p) as source:
        im=source.convert('RGB')
    if im.size!=(360,360):parser.error(f'Invalid shot dimensions: {p}')
    if args.export_scenes:im.save(args.export_scenes/(p.stem+'.png'))
    x=15+(i%4)*380;y=15+(i//4)*400
    sheet.paste(im,(x,y))
    ImageDraw.Draw(sheet).text((x+180,y+365),labels[i],font=font,fill='#182944',anchor='mt')
args.output.parent.mkdir(parents=True,exist_ok=True)
sheet.save(args.output)
print(args.output)
