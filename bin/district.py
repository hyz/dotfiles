#!/bin/python

import sys, json, fire, re

def gofmt_1():
    def fmt2(subs, top):
        topid = int(top['id']/10000)
        #for x in filter(lambda y:int(y['id']/10000)==topid, subs):
        #    print(topid,x['id'])
        print('"%(name)s": locNameIdxChina{%(id)s, map[string]uint{' % top, end='')
        print(*map(lambda y:'"%(name)s":%(id)s'%y, filter(lambda y:int(y['id']/10000)==topid, subs)), sep=',', end='')
        print('},')
        # "广东": subAddr{4400, map[string]uint{"深圳": 4411, "惠州": 4422}},
    js = json.load(sys.stdin)
    tops, subs, _ = js['result']
    for sub in subs:
        sub['id'] = int(sub['id'])
    for top in tops:
        top['id'] = int(top['id'])
        fmt2(subs, top)
def gofmt():
    def fmt2(subs, top):
        topid = int(top['id']/10000)
        print('%d: map[string]uint{' % top['id'], end='')
        print(*map(lambda y:'"%(name)s":%(id)d'%y, filter(lambda y:int(y['id']/10000)==topid, subs)), sep=',', end='')
        print('},')
        # "广东": subAddr{4400, map[string]uint{"深圳": 4411, "惠州": 4422}},
    js = json.load(sys.stdin)
    tops, subs, _ = js['result']
    for sub in subs:
        sub['id'] = int(sub['id'])
    for top in tops:
        top['id'] = int(top['id'])
        fmt2(subs, top)

def gofmt_r():
    def fmt2(subs, top):
        topid = int(top['id']/10000)
        print('%d: LocNameMap{"%s", map[uint]string{' % (topid,top['fullname']), end='')
        print(*map(lambda y:'%(id)s:"%(fullname)s"'%y, filter(lambda y:int(y['id']/10000)==topid, subs)), sep=',', end='')
        print('}},')
        # "广东": subAddr{4400, map[string]uint{"深圳": 4411, "惠州": 4422}},
    js = json.load(sys.stdin)
    tops, subs, _ = js['result']
    for sub in subs:
        sub['id'] = int(sub['id'])
    for top in tops:
        top['id'] = int(top['id'])
        fmt2(subs, top)

def gensql(opx):
    js = json.load(sys.stdin)
    tops, _, _ = js['result']
    for top in tops:
        loc = top['id']
        if opx.upper() == 'INSERT':
            print(f"INSERT INTO k9loc(loc,man,rip) VALUES('{loc}', 0, '127.0.0.1');")
        elif opx.upper() == 'UPDATE':
            print(f"UPDATE k9loc set man=0, tims='now()' where loc={loc};")

def javafmt():
    def fmt(lis, tag):
        print(f'final int[] {tag}IdArr = {{', end='')
        print(*map(lambda s:'%(id)s'%s, lis), sep=',', end='')
        print('};')
        print(f'final String[] {tag}NameArr = {{', end='')
        print(*map(lambda s:'"%(fullname)s"'%s, lis), sep=',', end='')
        print('};')
    js = json.load(sys.stdin)
    tops, subs, _ = js['result']
    fmt( list( sorted(tops, key=lambda s:int(s['id'])) ), tag='a')
    fmt( list( sorted(subs, key=lambda s:int(s['id'])) ), tag='b')
    #print('int[] idA = {', ','.join(s['id'] for s in idA), '};')
    #print('String[] strA = {', ','.join('"%(name)s"'%s for s in idA), '};')
    #for sub in subs: print('%(id)s: "%(name)s",' % sub)
    #for top in tops: print('%(id)s: "%(name)s",' % sub)

red_ = (
        (('江苏','江西','广西','广东','河南','山东','福建','河北', '山西', '陕西', '辽宁', '浙江', '安徽'), '^[^省]+省([^市]+)市')
        , (('湖南','青海','贵州','四川','甘肃','云南','吉林'), '^[^省]+省([^州市]+)[州市]')
        , (('北京','天津','上海'), '^[^市]+市([^区]+)区')
        , (('湖北','黑龙江')     , '^[^省]+省([^市区州]+)[市区州]')
        , (('重庆',)      , '^重庆市([^区县]+)[区县]')
        , (('海南',)      , '^海南省([^市县]+)[市县]')
        , (('台湾',)      , '^台湾省([^市县]+)[市县]')
        , (('宁夏','广西'), '^.+自治区([^市]+)市')
        , (('新疆',)      , '^.+自治区([^州市区]+)[州市区]')
        , (('内蒙古',)    , '^.+自治区([^市盟]+)[市盟]')
        , (('西藏',)      , '^.+自治区([^市区]+)[市区]')
        , (('香港',)      , '^.+行政区([^区]+)区')
        , (('澳门',)      , '^.+行政区(澳门半岛|氹仔|路氹城|路环)')
        )
def test():
    reLis = []
    for al,r in red_:
        reLis += [ (a,r) for a in al ]

    js = json.load(sys.stdin)
    tops, subs, _ = js['result']
    mtop = {}
    for top in tops:
        id = top['id'] = int(top['id'])
        mtop.setdefault(id, top)

    for x,y in reLis:
        id = 0
        for top in tops:
            if top['name'].startswith(x):
                id = top['id']
        print(f'{{ "{x}", {id}, "{y}" }},')
    print()

    for sub in subs:
        id2 = sub['id'] = int(sub['id'])
        id1 = int(id2/10000)*10000
        top = mtop[id1]
        addr = top['fullname'] + sub['fullname'] + 'XYZ市区'
        pfx2 = addr[:2]

        mh = None
        for a, _re in reLis:
            if a.startswith(pfx2):
                mh = re.search(_re, addr)

        #if addr.startswith('台湾'):
        #    mh = re.search('^(台湾省)([^市县]+[市县])', addr)
        #elif addr.startswith('香港'):
        #    mh = re.search('^(香港特别行政区)([^区]+区)', addr)
        #elif addr.startswith('澳门'):
        #    mh = re.search('^(澳门特别行政区)(澳门半岛|氹仔|路氹城|路环)', addr)
        #elif addr.startswith('新疆'):
        #    mh = re.search('^(新疆维吾尔自治区)([^州区]+[州区])', addr)
        #elif addr.startswith('海南'):
        #    mh = re.search('^(海南省)([^县]+[县])', addr)
        #elif addr.startswith('重庆'):
        #    mh = re.search('^(重庆市)([^县]+[县])', addr)
        #elif addr.startswith('内蒙古'):
        #    mh = re.search('^(内蒙古自治区)([^盟]+[盟])', addr)
        #elif addr.startswith('西藏'):
        #    mh = re.search('^(西藏自治区)([^区]+[区])', addr)
        #else:
        #    pass
        #    # ! 232700 黑龙江省大兴安岭地区XYZ市区
        #    # ! 429021 湖北省神农架林区XYZ市区
        #    #mh = re.search('^([^省市区]+省)([^省市区]+自治州)', addr)
        #    #if not mh:
        #    #    mh = re.search('^([^省市区]+省)([^省市区]+市)', addr)
        #    #    if not mh:
        #    #        mh = re.search('^([^省市区]+市)([^省市区]+区)', addr)
        #    #        if not mh:
        #    #            mh = re.search('^([^省市区]+自治区)([^省市区]+市)', addr)
        #    ##mh = re.search('^([^省市]+[省市]|.+自治区)([^市区县]+[市区县]|.+自治州)', addr)
        if mh:
            print(id2, mh.groups())
        else:
            print('!', id2, addr)
    #for top in tops:
    #    if not re.search('([省市]|自治区)$', top['fullname']):
    #        print(top['id'], top['fullname'])

def f1(id=440000):
    idMax = (id /10000) * 10000 + 9999
    js = json.load(sys.stdin)
    for r in js['result']:
        for x in filter(lambda x: id<=int(x['id'])<=idMax, r):
            print(x)
def f2(id=440000):
    idMax = (id /100) * 100 + 99
    js = json.load(sys.stdin)
    for r in js['result']:
        for x in filter(lambda x: id<=int(x['id'])<=idMax, r):
            print(x)
def top():
    js = json.load(sys.stdin)
    r,_,_ = js['result']
    for x in r: #filter(lambda x: id<=int(x['id'])<=id+9999, r):
        print(x)

if __name__ == '__main__':
    fire.Fire()
# district.py f2 440300 < district.txt
# district.py f2 441300 < district.txt

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

