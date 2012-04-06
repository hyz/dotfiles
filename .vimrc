set t_Co=256
set guifont=Monaco\ 14
set guioptions=

set dict=/usr/share/dict/words
set ts=4 sw=4 expandtab ai nocp nowrap
syntax on
filetype plugin on
filetype indent on

set hlsearch
set diffopt+=iwhite

set completeopt=longest,menu

colorscheme desert256 " desert desert256 inkpot gardener tango desert elflord wombat256 

set fileencodings=cp936,gb2312,gbk,utf-8,unicode,ucs-2,big5,euc-jp,euc-kr,latin1,ucs-bom

set path=.,..,*/,*/*/,~/include,/usr/include,/usr/include/*/
" set path=.,..,../..,~/include,/usr/include,/usr/include/c++/**3;/usr/include
" gcc -v 2>&1 |grep includedir |cut -d  -f6 |cut -d= -f2 

set tags+=../tags,~/.tags
" set tags=tags,../tags,../../tags,../../../tags,~/include/tags,/usr/include/tags,~/view/boost/tags

set cscopeprg=mlcscope

"autocmd BufEnter * lcd %:p:h
autocmd BufNewFile *.cc 0r $HOME/.vim/tpl.cc

"autocmd FileType python setlocal ts=4 | setlocal sw=4
"autocmd FileType python compiler pyunit
"autocmd FileType python setlocal nocindent ai
autocmd FileType python setlocal makeprg=python

map <C-F12> :!ctags --c++-kinds=+p --fields=+aiS --extra=+q -R `pwd`<CR>

cnoremap <C-A> <Home>
cnoremap <C-F> <Right>
cnoremap <C-B> <Left>
cnoremap <Esc>b <S-Left>
cnoremap <Esc>f <S-Right>

"map <F5> Go<Esc>cc// Date: <Esc>:r!date<CR>kJ
"map <F5> :bn<CR>''
"map <S-F5> :bN<CR>''

"map K :! LANG="zh_CN.GB2312" man <cword><CR>
"
" so $HOME/.vim/a.vim

"set makeprg=bjam\ --v2\ --toolset=gcc
"set makeprg=bjam\ debug\ debug-symbols=on
"map <C-F5> :setlocal makeprg=g++\ -Wall\ -pthread\ -g\ -I$HOME/view/boost\ -DSAMPLES\ %<CR>
"ctags --languages=c,c++ --c++-kinds=+p --langmap=c++:+. --fields=+aiS --extra=+q -R `pwd`
"map Ctrl-p Ctrl-y
"

" set showmatch

let loaded_matchparen = 1

"au BufReadPost * if line("'\"") > 0 && line("'\"") <= line("$") | exe "normal g'\"" | endif

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

""" OmniCPP
" let OmniCpp_DefaultNamespaces = ["std"]
" let OmniCpp_NamespaceSearch = 1
" let OmniCpp_MayCompleteDot = 1
" let OmniCpp_MayCompleteArrow = 1
" let OmniCpp_MayCompleteScope = 1
" let OmniCpp_ShowScopeInAbbr = 1
" let OmniCpp_ShowPrototypeInAbbr = 1

"let Tlist_Sort_Type = "name" " order by
"let Tlist_Use_Right_Window = 1 " split to the right side of the screen
"let Tlist_Compart_Format = 1 " show small meny
"let Tlist_Exist_OnlyWindow = 1 " if you are the last, kill yourself
"let Tlist_File_Fold_Auto_Close = 0 " do not close tags for other files
"let Tlist_Enable_Fold_Column = 0 " do not show folding tree

"let g:SuperTabRetainCompletionType=2
"let g:SuperTabDefaultCompletionType="<C-X><C-O>"

" VimIM
let g:vimim_tab_for_one_key=1
"let g:vimim_one_key=0

