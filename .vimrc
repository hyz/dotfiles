set nocompatible
filetype off " filetype plugin indent on

set t_Co=256
set guifont=Monaco\ 14
set guioptions=
set ts=4 sw=4 expandtab ai nocp nowrap

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
"set path=.,..,*/,*/*/,include,../include,~/include,/usr/include,/usr/include/*/
set path=.,..
" set path=.,..,../..,~/include,/usr/include,/usr/include/c++/**3;/usr/include
" gcc -v 2>&1 |grep includedir |cut -d  -f6 |cut -d= -f2 

"set tags+=../tags,../../tags,~/.tags
"set tags=tags,../tags,../../tags,../../../tags,~/include/tags,/usr/include/tags,~/view/boost/tags

" set cscopeprg=mlcscope

" set makeprg=b2\ -j5

"autocmd BufEnter * lcd %:p:h
"autocmd BufNewFile *.cc 0r $HOME/.vim/tpl.cc

"autocmd FileType python setlocal ts=4 | setlocal sw=4
"autocmd FileType python compiler pyunit
"autocmd FileType python setlocal nocindent ai
autocmd FileType python setlocal makeprg=python\ %

"" jump to the last position when reopening a file
if has("autocmd")
    au BufReadPost * if line("'\"") > 0 && line("'\"") <= line("$") | exe "normal! g'\"" | endif
endif

set history=1000
cnoremap <C-L> <Up>

"" #pathogen https://github.com/tpope/vim-pathogen
""  mkdir -p ~/.vim/autoload ~/.vim/bundle && curl -LSso ~/.vim/autoload/pathogen.vim https://tpo.pe/pathogen.vim
"execute pathogen#infect()
"" #vim-go https://github.com/fatih/vim-go
""  git clone https://github.com/fatih/vim-go.git ~/.vim/bundle/vim-go
""  :GoInstallBinaries

"" #Vundle https://github.com/VundleVim/Vundle.vim
""  git clone https://github.com/VundleVim/Vundle.vim.git ~/.vim/bundle/Vundle.vim
set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()
Plugin 'VundleVim/Vundle.vim'
Plugin 'Valloric/YouCompleteMe'
""" https://github.com/Valloric/YouCompleteMe
"""     ./install.py --rust-completer
"""     ./install.py --clang-completer --system-libclang --system-boost
"Plugin 'vim-scripts/a.vim'
"Plugin 'plasticboy/vim-markdown'
Plugin 'rhysd/rust-doc.vim'
Plugin 'rust-lang/rust.vim'
Plugin 'racer-rust/vim-racer'
"" https://github.com/plasticboy/vim-markdown
Plugin 'godlygeek/tabular'
Plugin 'plasticboy/vim-markdown'
call vundle#end()
" vim +PluginInstall +qall
"""Brief help
" :PluginList       - lists configured plugins
" :PluginInstall    - installs plugins; append `!` to update or just :PluginUpdate
" :PluginSearch foo - searches for foo; append `!` to refresh local cache
" :PluginClean      - confirms removal of unused plugins; append `!` to auto-approve removal
"
"""https://github.com/junegunn/vim-plug
" curl -fLo ~/.vim/autoload/plug.vim --create-dirs https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim
" vim +PlugInstall +q
call plug#begin('~/.vim/plugged')
Plug 'fatih/vim-go'
Plug 'junegunn/fzf', { 'dir': '~/.fzf', 'do': './install --all' }
Plug 'junegunn/fzf.vim'
Plug 'junegunn/goyo.vim'
"Plug 'autozimu/LanguageClient-neovim', {'tag': 'binary-*-x86_64-linux-musl' }
call plug#end()

""" Put your non-Plugin stuff after this line === === ===

"cnoremap <Esc>b <S-Left>
"cnoremap <Esc>w <S-Right>

"map <C-F12> :!ctags --c++-kinds=+p --fields=+aiS --extra=+q -R `pwd`<CR>
"map <F5> Go<Esc>cc// Date: <Esc>:r!date<CR>kJ
"map <F5> :bn<CR>''
"map <S-F5> :bN<CR>''

"map K :! LANG="zh_CN.GB2312" man <cword><CR>

"set makeprg=bjam\ --v2\ --toolset=gcc
"set makeprg=bjam\ debug\ debug-symbols=on
"map <C-F5> :setlocal makeprg=g++\ -Wall\ -pthread\ -g\ -I$HOME/view/boost\ -DSAMPLES\ %<CR>
"ctags --languages=c,c++ --c++-kinds=+p --langmap=c++:+. --fields=+aiS --extra=+q -R `pwd`
"map Ctrl-p Ctrl-y
"

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

""" VimIM
let g:vimim_tab_for_one_key=1
"let g:vimim_one_key=0

"""
"let g:EclimDisabled=1

"""
"let loaded_matchparen = 1
"hi MatchParen cterm=none ctermbg=green ctermfg=blue
"hi MatchParen cterm=bold ctermbg=none ctermfg=magenta
"set showmatch

""" clang_complete
"set completeopt-=preview


syntax on
filetype plugin indent on

"" rust
let g:rustfmt_autosave = 1
set hidden
let g:racer_cmd = "$HOME/.cargo/bin/racer"
let g:racer_experimental_completer = 1
let g:ycm_rust_src_path="$HOME/rs/rust-master/src"
"autocmd BufRead *.rs :setlocal tags=./rusty-tags.vi;/
"autocmd BufWrite *.rs :silent! exec "!rusty-tags vi --quiet --start-dir=" . expand('%:p:h') . "&" <bar> redraw!
au FileType rust nmap gd <Plug>(rust-def)
au FileType rust nmap gs <Plug>(rust-def-split)
"au FileType rust nmap gx <Plug>(rust-def-vertical)
au FileType rust nmap <leader>gd <Plug>(rust-doc)

