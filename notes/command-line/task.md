
    task commands
    task calendar

    task tags
    task +ACTIVE |tail +4 |awk '{$2=$3="";print $0}' |column -t
    task +Todo add ...

    task scheduled:2018-01-01 add ...
    task due:+2days add ...
    task recur:weekday add ...

### https://taskwarrior.org/docs/
### https://taskwarrior.org/docs/best-practices.html
### https://linux.die.net/man/5/task-faq

    pacman -Sy task

    man task
    man task-sync
    vim .taskrc
    task add hello world.
    task add priority:H 手机充值
    task next
    task
    task project:android add 主机版本重新划分
    task project:android
    task project:server add '/test/echo'
    task project:server 4 done
    task project:server
    task all
    task project:server add hk server upgrade
    task project:server add test server reboot
    task
    task projects
    task project:server

    task |awk '{if($3!="Todo")print}'

### https://taskwarrior.org/docs/tags.html

virtual tags

    task 1 start
    task +ACTIVE list

    task 1 modify +work

task +ACTIVE |tail +4 |awk '{print $1" "$4" "$5}' |column -t 
