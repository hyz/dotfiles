import os, sys
from PIL import Image, ImageFont, ImageDraw
 
text = sys.argv[1] # "这是一段测试文本，test 123。"
 
im = Image.new("RGB", (300, 60), (255, 255, 255))
dr = ImageDraw.Draw(im)

#font = os.path.join("fonts", "simsun.ttc")
#font = os.path.join("fonts", "msyh.ttf")
font = os.path.join(os.environ['HOME'], '.local/share/fonts/Monaco_Yahei.ttf')
font = ImageFont.truetype(font, 50) #.truetype(font, 14)
 
dr.text((1,1), text, font=font, fill="#000000")
 
im.show()
im.save("/tmp/chr.png")

