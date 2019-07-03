set nocompatible
filetype off " filetype plugin indent on

set t_Co=256
set guifont=Monaco\ 14
set guioptions=
set ts=4 sw=4 expandtab ai nocp nowrap

colorscheme jellybeans " desert desert256 murphy inkpot gardener tango elflord wombat256 
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
"autocmd FileType python setlocal makeprg=python\ %

"" jump to the last position when reopening a file
if has("autocmd")
    au BufReadPost * if line("'\"") > 0 && line("'\"") <= line("$") | exe "normal! g'\"" | endif
endif

set history=1000
cnoremap <C-L> <Up>

""" https://github.com/VundleVim/Vundle.vim
""" git clone https://github.com/VundleVim/Vundle.vim.git ~/.vim/bundle/Vundle.vim
"set rtp+=~/.vim/bundle/Vundle.vim
"call vundle#begin()
"Plugin 'VundleVim/Vundle.vim'
"call vundle#end()
" vim +PluginInstall +qall
"""Brief help
" :PluginList       - lists configured plugins
" :PluginInstall    - installs plugins; append `!` to update or just :PluginUpdate
" :PluginSearch foo - searches for foo; append `!` to refresh local cache
" :PluginClean      - confirms removal of unused plugins; append `!` to auto-approve removal

""" https://github.com/junegunn/vim-plug
" curl -fLo ~/.vim/autoload/plug.vim --create-dirs https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim
" vim +PlugInstall +q

" https://github.com/euclio/vim-markdown-composer
function! BuildComposer(info)
  if a:info.status != 'unchanged' || a:info.force
    if has('nvim')
      !cargo build --release
    else
      !cargo build --release --no-default-features --features json-rpc
    endif
  endif
endfunction
function! BuildYCM(info)
  " info is a dictionary with 3 fields
  " - name:   name of the plugin
  " - status: 'installed', 'updated', or 'unchanged'
  " - force:  set on PlugInstall! or PlugUpdate!
  if a:info.status == 'installed' || a:info.force
    !./install.py
  endif
endfunction

call plug#begin('~/.vim/plugged')
Plug 'vim-scripts/fcitx.vim'
Plug 'w0rp/ale'
Plug 'jremmen/vim-ripgrep'
"Plug 'vim-scripts/a.vim'
"Plug 'rhysd/rust-doc.vim'
Plug 'rust-lang/rust.vim'
Plug 'timonv/vim-cargo'
Plug 'racer-rust/vim-racer'
"Plug 'godlygeek/tabular'
Plug 'mxw/vim-jsx'
Plug 'pangloss/vim-javascript'
"Plug 'moll/vim-node'
"Plug 'junegunn/fzf'
""" { 'dir': '~/.fzf', 'do': './install --all' }
"Plug 'junegunn/fzf.vim'
"Plug 'autozimu/LanguageClient-neovim'
""" { 'branch': 'next', 'do': 'bash install.sh', }
"Plug 'python-mode/python-mode'
"Plug 'amoffat/snake'
"Plug 'vimim/vimim'
"Plug 'vim-scripts/VimIM'
Plug 'plasticboy/vim-markdown'
""cargo install skim
Plug 'lotabout/skim.vim'
Plug 'ryym/vim-riot'

"""

"Plug 'suan/vim-instant-markdown'
Plug 'euclio/vim-markdown-composer', { 'do': function('BuildComposer') }

"Plug 'autozimu/LanguageClient-neovim', { 'branch': 'next', 'do': 'bash install.sh', }
"Plug 'Shougo/deoplete.nvim', { 'do': ':UpdateRemotePlugins' }

"junegunn/fzf
"Plug 'lotabout/skim', { 'dir': '~/.skim', 'do': './install' }
"Plug 'junegunn/fzf', { 'dir': '~/.fzf', 'do': './install --all' }

""" cd .vim/bundle/YouCompleteMe && ./install.py --rust-completer --clang-completer --system-libclang --system-boost
"Plug 'Valloric/YouCompleteMe', { 'do': function('BuildYCM') }

"Plug 'rdnetto/YCM-Generator', { 'branch': 'stable' }

"Plug 'SirVer/ultisnips' | Plug 'honza/vim-snippets'

"Plug 'junegunn/vim-easy-align'

"Plug 'fatih/vim-go', { 'tag': '*' }
"Plug 'nsf/gocode', { 'tag': 'v.20150303', 'rtp': 'vim' }

Plug 'lervag/vimtex'

call plug#end()

""" Put your non-Plugin stuff after this line === === ===

""" https://github.com/autozimu/LanguageClient-neovim
""" cd .vim/bundle/LanguageClient-neovim && bash install.sh
set runtimepath+=~/.vim/bundle/LanguageClient-neovim

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

""" VimIM - https://github.com/vim-scripts/VimIM
"let g:vimim_one_key=0
"    let g:Vimim_cloud = 'google,sogou,baidu,qq'
let g:Vimim_cloud = 'baidu,sogou,qq'
let g:Vimim_punctuation = 3
"let g:vimim_map = 'tab_as_gi'
" :let g:vimim_mode = 'dynamic'
" :let g:vimim_mycloud = 0
" :let g:vimim_plugin = 'C:/var/mobile/vim/vimfiles/plugin'
" :let g:vimim_punctuation = 2
" :let g:vimim_shuangpin = 0
" :let g:vimim_toggle = 'pinyin,google,sogou'

"""
"let g:EclimDisabled=1

"""
"let loaded_matchparen = 1
"hi MatchParen cterm=none ctermbg=green ctermfg=blue
"hi MatchParen cterm=bold ctermbg=none ctermfg=magenta
"set showmatch

""" clang_complete
"set completeopt-=preview


filetype plugin indent on
syntax on
set hidden

""" rust
""" https://github.com/ivanceras/rust-vim-setup
let g:rustfmt_autosave = 1
"let g:racer_cmd = "$HOME/.home/cargo/bin/racer"
let g:racer_experimental_completer = 1
let g:ycm_rust_src_path="$HOME/.home/rustup/toolchains/nightly-x86_64-unknown-linux-gnu/lib/rustlib/src/rust/src"

""" python
let g:pymode_python = 'python3'
"let g:ycm_python_binary_path = 'python'
if filereadable(expand("~/.vim/bundle/snake/plugin/snake.vim"))
    source ~/.vim/bundle/snake/plugin/snake.vim
endif

""autocmd BufRead *.rs :setlocal tags=./rusty-tags.vi;/
""autocmd BufWrite *.rs :silent! exec "!rusty-tags vi --quiet --start-dir=" . expand('%:p:h') . "&" <bar> redraw!
au FileType rust nmap gd <Plug>(rust-def)
au FileType rust nmap gs <Plug>(rust-def-split)
au FileType rust nmap <leader>gd <Plug>(rust-doc)
""au FileType rust nmap gx <Plug>(rust-def-vertical)

"let g:LanguageClient_serverCommands = { 'rust': ['$HOME/.cargo/bin/rustup', 'run', 'nightly', 'rls'] }
"    " , 'javascript': ['/usr/local/bin/javascript-typescript-stdio'], 'javascript.jsx': ['tcp://127.0.0.1:2089'], 'python': ['/usr/local/bin/pyls'],
"nnoremap <F5> :call LanguageClient_contextMenu()<CR>
"" Or map each action separately
"nnoremap <silent> K :call LanguageClient#textDocument_hover()<CR>
"nnoremap <silent> gd :call LanguageClient#textDocument_definition()<CR>
"nnoremap <silent> <F2> :call LanguageClient#textDocument_rename()<CR>

command W :execute ':silent w !sudo tee % > /dev/null' | :edit!

