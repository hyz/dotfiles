
    exec 1>&- # close stdout
    exec 2>&- # close stderr

    exec 1>/dev/null
    exec 2>/dev/null

    exec > >(logger -p user.info) 2> >(logger -p user.warn)

