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

"" jump to the last position when reopening a file
if has("autocmd")
    au BufReadPost * if line("'\"") > 0 && line("'\"") <= line("$") | exe "normal! g'\"" | endif
endif

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
let g:EclimDisabled=1

"""
"let loaded_matchparen = 1
"hi MatchParen cterm=none ctermbg=green ctermfg=blue
"hi MatchParen cterm=bold ctermbg=none ctermfg=magenta
"set showmatch

""" clang_complete
"set completeopt-=preview

""" YouCompleteMe
" "let g:ycm_global_ycm_extra_conf='~/.vim/.ycm_extra_conf.py'
" "let g:ycm_confirm_extra_conf = 0
" "let g:ycm_add_preview_to_completeopt=0
" "let g:ycm_seed_identifiers_with_syntax = 1  "C/C++关键字自动补全
" "let g:ycm_collect_identifiers_from_tags_files = 1
" "let g:enable_ycm_at_startup = 0
" "let g:loaded_youcompleteme = 1
" "let g:clang_library_path='/usr/lib'
" 
" let g:ycm_global_ycm_extra_conf='$HOME/.ycm_extra_conf.py'
" "let g:ycm_extra_conf_globlist=['~/.vim/*']
" let g:ycm_always_populate_location_list = 0
" let g:ycm_auto_trigger=1
" let g:ycm_enable_diagnostic_highlighting=1
" let g:ycm_enable_diagnostic_signs=1
" let g:ycm_max_diagnostics_to_display=10000
" let g:ycm_min_num_identifier_candidate_chars=0
" let g:ycm_min_num_of_chars_for_completion=2
" let g:ycm_open_loclist_on_ycm_diags=1
" let g:ycm_show_diagnostics_ui=1
" let g:ycm_collect_identifiers_from_tags_files = 1
" let g:ycm_filetype_blacklist={
"             \ 'vim' : 1,
"             \ 'tagbar' : 1,
"             \ 'qf' : 1,
"             \ 'notes' : 1,
"             \ 'markdown' : 1,
"             \ 'md' : 1,
"             \ 'unite' : 1,
"             \ 'text' : 1,
"             \ 'vimwiki' : 1,
"             \ 'pandoc' : 1,
"             \ 'infolog' : 1,
"             \ 'mail' : 1
" \}

""" pathogen
"execute pathogen#infect()

""" git clone https://github.com/VundleVim/Vundle.vim.git ~/.vim/bundle/Vundle.vim
set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()
Plugin 'VundleVim/Vundle.vim'
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
Plugin 'rhysd/rust-doc.vim'
Plugin 'rust-lang/rust.vim'
call vundle#end()            " required
"""Install Plugins:
"   Launch vim and run :PluginInstall
"   To install from command line: vim +PluginInstall +qall
"""Brief help
" :PluginList       - lists configured plugins
" :PluginInstall    - installs plugins; append `!` to update or just :PluginUpdate
" :PluginSearch foo - searches for foo; append `!` to refresh local cache
" :PluginClean      - confirms removal of unused plugins; append `!` to auto-approve removal
"
" see :h vundle for more details or wiki for FAQ
" Put your non-Plugin stuff after this line
" =================== " ===================

syntax on
filetype plugin indent on

""" rust.vim
let g:rustfmt_autosave = 1

