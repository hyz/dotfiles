
### https://docs.microsoft.com/en-us/windows/wsl/wsl2-mount-disk

    PS C:\> wmic diskdrive list brief

    wsl --mount <DiskPath> --bare
    wsl --mount \\.\PHYSICALDRIVE2 --partition 1
    cd /mnt/wsl/PHYSICALDRIVE2p1

    wsl.exe  --shutdown
    wsl.exe  --list --running
    wsl.exe  --list --all

