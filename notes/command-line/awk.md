
http://www.commandlinefu.com/commands/view/6872/exclude-a-column-with-awk
https://stackoverflow.com/questions/15361632/delete-a-column-with-awk-or-sed

    vim file
    :%!awk '{$3="";print}'

    cut -f3 --complement
