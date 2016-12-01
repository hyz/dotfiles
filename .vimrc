set nocompatible
"filetype off " filetype plugin indent on

"" set rtp+=~/.vim/bundle/Vundle.vim
"" call vundle#begin()
"" Plugin 'VundleVim/Vundle.vim'
"" Plugin 'Valloric/YouCompleteMe'
"" Plugin 'vim-scripts/a.vim'
"" Plugin 'plasticboy/vim-markdown'
"" "Plugin 'honza/vim-snippets'
"" 
"" "Plugin 'tpope/vim-fugitive'
"" " plugin from http://vim-scripts.org/vim/scripts.html
"" "Plugin 'L9'
"" " git repos on your local machine (i.e. when working on your own plugin)
"" "Plugin 'file:///home/gmarik/path/to/plugin'
"" " The sparkup vim script is in a subdirectory of this repo called vim.
"" " Pass the path to set the runtimepath properly.
"" "Plugin 'rstacruz/sparkup', {'rtp': 'vim/'}
"" " Install L9 and avoid a Naming conflict if you've already installed a
"" " different version somewhere else.
"" "Plugin 'ascenator/L9', {'name': 'newL9'}
"" 
"" " All of your Plugins must be added before the following line
"" call vundle#end()            " required

set t_Co=256
set guifont=Monaco\ 14
set guioptions=
set ts=4 sw=4 expandtab ai nocp nowrap
"
" "Brief help
" :PluginList       - lists configured plugins
" :PluginInstall    - installs plugins; append `!` to update or just :PluginUpdate
" :PluginSearch foo - searches for foo; append `!` to refresh local cache
" :PluginClean      - confirms removal of unused plugins; append `!` to auto-approve removal
"
" see :h vundle for more details or wiki for FAQ
" Put your non-Plugin stuff after this line
" =================== " ===================

colorscheme desert " desert desert256 inkpot gardener tango desert elflord wombat256 
if &diff
    colorscheme jellybeans
endif

set hlsearch " incsearch
" highlight Search ctermbg=Black ctermfg=Yellow
highlight Search ctermbg=DarkGray ctermfg=Black

set diffopt+=iwhite

set dictionary=/usr/share/dict/words
"set complete-=u
set complete-=i
set complete+=k

" set completeopt=longest,menu
" set fileencodings=utf-bom,UTF-8,gb2312,UTF-16BE,UTF-16,gb18030,big5,euc-jp,euc-kr,iso8859-1
" set fileencodings=UTF-8,latin1,UTF-16BE,UTF-16,latin1,gb2312,gb18030,big5,euc-jp,euc-kr,iso8859-1
set fileencodings=UTF-8
set fileformats=unix,dos,mac
set path=.,..,*/,*/*/,include,../include,~/include,/usr/include,/usr/include/*/
" set path=.,..,../..,~/include,/usr/include,/usr/include/c++/**3;/usr/include
" gcc -v 2>&1 |grep includedir |cut -d  -f6 |cut -d= -f2 

set tags+=../tags,../../tags,~/.tags
" set tags=tags,../tags,../../tags,../../../tags,~/include/tags,/usr/include/tags,~/view/boost/tags

" set cscopeprg=mlcscope

" set makeprg=b2\ -j5

"autocmd BufEnter * lcd %:p:h
autocmd BufNewFile *.cc 0r $HOME/.vim/tpl.cc

"autocmd FileType python setlocal ts=4 | setlocal sw=4
"autocmd FileType python compiler pyunit
"autocmd FileType python setlocal nocindent ai
autocmd FileType python setlocal makeprg=python\ %

set history=600
cnoremap <C-L> <Up>
"cnoremap <Esc>b <S-Left>
"cnoremap <Esc>w <S-Right>

"map <C-F12> :!ctags --c++-kinds=+p --fields=+aiS --extra=+q -R `pwd`<CR>

"map <F5> Go<Esc>cc// Date: <Esc>:r!date<CR>kJ
"map <F5> :bn<CR>''
"map <S-F5> :bN<CR>''

"map K :! LANG="zh_CN.GB2312" man <cword><CR>
"

"set makeprg=bjam\ --v2\ --toolset=gcc
"set makeprg=bjam\ debug\ debug-symbols=on
"map <C-F5> :setlocal makeprg=g++\ -Wall\ -pthread\ -g\ -I$HOME/view/boost\ -DSAMPLES\ %<CR>
"ctags --languages=c,c++ --c++-kinds=+p --langmap=c++:+. --fields=+aiS --extra=+q -R `pwd`
"map Ctrl-p Ctrl-y
"

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

" VimIM
let g:vimim_tab_for_one_key=1
"let g:vimim_one_key=0

let g:EclimDisabled=1

"""let loaded_matchparen = 1
"hi MatchParen cterm=none ctermbg=green ctermfg=blue
"hi MatchParen cterm=bold ctermbg=none ctermfg=magenta
"set showmatch

syntax on
filetype plugin indent on

set completeopt-=preview
let g:ycm_add_preview_to_completeopt=0

