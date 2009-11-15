set dict=/usr/share/dict/words

set nocp nowrap expandtab
set ts=4 sw=4
syntax on
filetype plugin on
filetype indent on

set completeopt=longest,menu

colorschem elflord " tango desert peachpuff darkblue elflord 

cnoremap <C-A> <Home>
cnoremap <C-F> <Right>
cnoremap <C-B> <Left>
cnoremap <Esc>b <S-Left>
cnoremap <Esc>f <S-Right>

"autocmd BufEnter * lcd %:p:h
autocmd BufNewFile *.cc 0r $HOME/.vim/template.cc

"autocmd FileType python setlocal ts=4 | setlocal sw=4
"autocmd FileType python compiler pyunit
"autocmd FileType python setlocal nocindent ai
autocmd FileType python setlocal makeprg=python

set fileencodings=utf-8,gbk,big5,euc-jp,euc-kr,latin1,ucs-bom

"map <F5> Go<Esc>cc// Date: <Esc>:r!date<CR>kJ
"map <F5> :bn<CR>''
"map <S-F5> :bN<CR>''

"map K :! LANG="zh_CN.GB2312" man <cword><CR>
"
" so $HOME/.vim/a.vim

"set makeprg=bjam\ --v2\ --toolset=gcc
"set makeprg=bjam\ debug\ debug-symbols=on
map <C-F5> :setlocal makeprg=g++\ -Wall\ -pthread\ -g\ -I$HOME/view/boost\ -DSAMPLES\ %<CR>
map <C-F12> :!ctags --c++-kinds=+p --fields=+aiS --extra=+q -R `pwd`<CR>
"ctags --languages=c,c++ --c++-kinds=+p --langmap=c++:+. --fields=+aiS --extra=+q -R `pwd`
"map Ctrl-p Ctrl-y
"

" set showmatch

let loaded_matchparen = 1

au BufReadPost * if line("'\"") > 0 && line("'\"") <= line("$") | exe "normal g'\"" | endif

"if has("cscope")
"		" set csprg=/usr/bin/cscope
"		set csto=0
"		set cst
"		set nocsverb
"		" add any database in current directory
"		if filereadable("cscope.out")
"			cs add cscope.out
"		" else add database pointed to by environment
"		elseif $CSCOPE_DB != ""
"			cs add $CSCOPE_DB
"		endif
"		set csverb
"endif

set path=.,..,../..,~/include,/usr/include,/usr/include/c++/**3;/usr/include
set tags=tags,../tags,../../tags,../../../tags,~/include/tags,/usr/include/tags,~/view/boost/tags

""" OmniCPP
" let OmniCpp_DefaultNamespaces = ["std"]
" let OmniCpp_NamespaceSearch = 1
" let OmniCpp_MayCompleteDot = 1
" let OmniCpp_MayCompleteArrow = 1
" let OmniCpp_MayCompleteScope = 1
" let OmniCpp_ShowScopeInAbbr = 1
" let OmniCpp_ShowPrototypeInAbbr = 1

let Tlist_Sort_Type = "name" " order by
let Tlist_Use_Right_Window = 1 " split to the right side of the screen
let Tlist_Compart_Format = 1 " show small meny
let Tlist_Exist_OnlyWindow = 1 " if you are the last, kill yourself
let Tlist_File_Fold_Auto_Close = 0 " do not close tags for other files
let Tlist_Enable_Fold_Column = 0 " do not show folding tree

"let g:SuperTabRetainCompletionType=2
"let g:SuperTabDefaultCompletionType="<C-X><C-O>"

