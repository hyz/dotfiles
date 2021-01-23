
## https://github.com/creationix/nvm/issues/586

NODE_PATH should not point to globally installed packages

    export NODE_PATH=$NODE_PATH:`npm root -g`

https://docs.npmjs.com/misc/faq#i-installed-something-globally-but-i-can-t-require-it

