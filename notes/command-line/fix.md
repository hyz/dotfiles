
https://unix.stackexchange.com/questions/79684/fix-terminal-after-displaying-a-binary-file/79686#79686

- method #1 - reset

	(ctl-c, ctl-c, ctl-c)
	reset

- method #2 - stty sane

	stty sane
	tput rs1

- ...

	alias fix='echo -e "\033c"'
	alias fix='reset; stty sane; tput rs1; clear; echo -e "\033c"'
	alias fix='echo -e "\033c" ; stty sane; setterm -reset; reset; tput reset; clear'

	(ctl-c, ctl-c, ctl-c)
	fix

