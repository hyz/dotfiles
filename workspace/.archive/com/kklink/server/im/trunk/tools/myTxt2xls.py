#!/usr/bin/python
def txt2xls( ifn, deli, ofn, sheet):
    with open(ifn) as fr:
        lines=fr.readlines()
        if len(lines) > 0:
            import xlwt
            xls=xlwt.Workbook(encoding='utf-8')
            tbl=xls.add_sheet(sheet, cell_overwrite_ok=True)
            l=0
            for line in lines:
                items=line.split(deli)
                h=0
                for content in items:
                    try:
                        content = int(content)
                    except:
                        pass
                    tbl.write(l,h,content)
                    h=h+1
                l=l+1
            xls.save(ofn)

if __name__=='__main__':
    #txt2xls('out-dir/_msg_and.txt','\t','test_demo.xls', 'Sheet1')
    txt2xls('/tmp/onlmsg/_msg_and.txt','\t','/tmp/onlmsg/test_demo.xls', 'Sheet1')
