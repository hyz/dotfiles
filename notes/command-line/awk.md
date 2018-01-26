
http://www.commandlinefu.com/commands/view/6872/exclude-a-column-with-awk
https://stackoverflow.com/questions/15361632/delete-a-column-with-awk-or-sed

    vim file
    :%!awk '{$3="";print}'

    cut -f3 --complement

https://stackoverflow.com/questions/2961635/using-awk-to-print-all-columns-from-the-nth-to-the-last

    awk +ACTIVE |tail +4 |awk '{$2=$3="";print $0}' |column -t

