#!/bin/python

import sys, json

d0 = 440000
if len(sys.argv) > 1:
    d0 = int(sys.argv[1])

js = json.load(sys.stdin)
for r in js['result']:
    for x in filter(lambda x: d0<=int(x['id'])<=d0+9999, r):
        print(x)

# # curl "http://apis.map.qq.com/ws/district/v1/list?key=CD7BZ-6RTKX-U7A4W-74LRA-L3BZK-VNFUD" > district.txt
# 
# import json
# >>> js = json.load(open('district.txt'))
# >>> r1, r2, r3 = js['result']
# >>> len(r1) , len(r2) , len(r3)
# 34 , 493 , 3103
# >>>
# >>> r1[0]
# {'id': '110000', 'name': '北京', 'fullname': '北京市', 'pinyin': ['bei', 'jing'], 'location': {'lat': 39.90469, 'lng': 116.40717}, 'cidx': [0, 15]}
# 
# >>> for x in filter(lambda x: '广东' in x['name'], r1): print(x)
# {'id': '440000', 'name': '广东', 'fullname': '广东省', 'pinyin': ['guang', 'dong'], 'location': {'lat': 23.13171, 'lng': 113.26627}, 'cidx': [246, 266]}
# 
# >>> for x in filter(lambda x: '深圳' in x['name'], r2): print(x)
# {'id': '440300', 'name': '深圳', 'fullname': '深圳市', 'pinyin': ['shen', 'zhen'], 'location': {'lat': 22.54286, 'lng': 114.05956}, 'cidx': [1688, 1695]}
# 
# >>> for x in filter(lambda x: '广州' in x['name'], r2): print(x)
# {'id': '440100', 'name': '广州', 'fullname': '广州市', 'pinyin': ['guang', 'zhou'], 'location': {'lat': 23.12908, 'lng': 113.26436}, 'cidx': [1667, 1677]}
# 
# >>> for x in filter(lambda x: '汕尾' in x['name'], r2): print(x)
# {'id': '441500', 'name': '汕尾', 'fullname': '汕尾市', 'pinyin': ['shan', 'wei'], 'location': {'lat': 22.78566, 'lng': 115.37514}, 'cidx': [1753, 1756]}
# 
# >>> for x in filter(lambda x: '陆丰' in x['fullname'], r3): print(x)
# {'id': '441581', 'fullname': '陆丰市', 'location': {'lat': 22.94511, 'lng': 115.64462}}
# 
# >>> for x in filter(lambda x: x['id'].startswith('4415'), r3): print(x)
# {'id': '441502', 'fullname': '城区', 'location': {'lat': 22.7787, 'lng': 115.36502}}
# {'id': '441521', 'fullname': '海丰县', 'location': {'lat': 22.96657, 'lng': 115.32341}}
# {'id': '441523', 'fullname': '陆河县', 'location': {'lat': 23.30148, 'lng': 115.65996}}
# {'id': '441581', 'fullname': '陆丰市', 'location': {'lat': 22.94511, 'lng': 115.64462}}

