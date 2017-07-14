#!/usr/bin/env python
""" Usage: call with <filename>
"""
### > clang -Xclang -ast-dump -fsyntax-only foo.cc
### http://blog.glehmann.net/2014/12/29/Playing-with-libclang/
### https://github.com/llvm-mirror/clang/blob/master/bindings/python/clang/cindex.py
### http://eli.thegreenplace.net/2011/07/03/parsing-c-in-python-with-clang
### https://manu343726.github.io/2017/02/11/writing-ast-matchers-for-libclang.html

import sys
import clang.cindex

function_calls = []             # List of AST node objects that are function calls
function_declarations = []      # List of AST node objects that are fucntion declarations

def traverse(node, lv):
    if node.kind == clang.cindex.CursorKind.CALL_EXPR:
        function_calls.append(node)

    if node.kind == clang.cindex.CursorKind.FUNCTION_DECL:
        function_declarations.append(node)

    print('  '*lv, 'Node:', node.kind, node.displayname, f'{node.location.line}:{node.location.column}')
    #print('Found %s [line=%s, col=%s]' % (node.displayname, node.location.line, node.location.column))

    for child in node.get_children():
        traverse(child, lv+1)

# Tell clang.cindex where libclang.dylib is
#clang.cindex.Config.set_library_path("/Users/tomgong/Desktop/build/lib")
index = clang.cindex.Index.create()

# Generate AST from filepath passed in the command line
tu = index.parse(sys.argv[1])

root = tu.cursor        # Get the root of the AST
traverse(root, 0)

# Print the contents of function_calls and function_declarations
#print(function_calls)
#print(function_declarations)

