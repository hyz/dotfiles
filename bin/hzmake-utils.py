#!/usr/bin/env python3

import sys, os
from xml.etree.ElementTree import ElementTree

def test(filename):
    def iter2(tree):
        for parent in tree.iter():
            for child in parent:
                yield parent, child

    tree = ElementTree(file=filename)
    root = tree.getroot()

    for name in root.iter('name'):
        print(1, name.text)

    for name in root.findall('name'):
        print(2, name)

    for name in tree.findall('projectDescription/name'):
        print(3, name)
    for name in root.findall('./name'):
        print(4, name.tag, name.text)

    name = root.find('./name')
    if name is not None:
        print(5, name.text)
        name.text = 'Helloworld'
        print(5, name.text)

    for node,sub in iter2(tree):
        print(9, node, sub)
        if node.tag == 'projectDescription' and sub.tag == 'name':
            break

    tree.write('/tmp/1.xml', encoding='UTF-8', xml_declaration=True) #(, short_empty_elements=False)

def replace_text(filename, path, text):
    tree = ElementTree(file=filename)
    name = tree.find(path)
    if name != None:
        name.text = text
        tree.write(filename, encoding='UTF-8', xml_declaration=True) #(, short_empty_elements=False)

def help(*a):
    print(sys.argv[0], 'replace_text <filename> <xpath> <text>')
    #test(sys.argv[1])
    #replace_text(sys.argv[1], sys.argv[2], sys.argv[3])

if __name__ == '__main__':
    import signal
    def sig_handler(signal, frame):
        global _STOP
        _STOP = 1
    signal.signal(signal.SIGINT, sig_handler)
    try:
        globals().get(os.path.basename(sys.argv[1]), help)(*sys.argv[2:])
    except Exception as e:
        print(e, file=sys.stderr)

