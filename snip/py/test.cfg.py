# -*- coding: utf-8 -*-

import ConfigParser, os

config = ConfigParser.ConfigParser()
#config.readfp(open('defaults.cfg'))
config.read('moon.conf')

print config.options('database')

print config.get('database','host')
print config.get('database','port')
print config.get('database','user')
print config.get('database','password')
print config.get('database','db')

