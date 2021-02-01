
### https://www.linuxquestions.org/questions/linux-software-2/how-do-you-hard-link-a-whole-directory-810187


You cannot hardlink a directory, at least when using filesystems that use traditional POSIX semantics; I bet BtrFS will let you do it with a judicious use of subvolumes sometime in the future.

Now, you can create directory trees with hardlinked files easily if using GNU coreutils and rsync. Use 

    cp -Hlr <fromdir> <targetdir>

to create the initial copy and 

    rsync -aHP --delete <fromdir> <targetdir>

for keep the target directory clean of cruft from time to time, else just fuggit about the latter. :-)

---

I just found this thread since I had a similar problem, and I figured I should post how I solved it.
My problem was that I wanted to keep file and folder structure intact in case i messed up when renaming all my music files and folders.

    cd /mnt/
    rsync -a Music/ OldMusicDir/ --link-dest=/mnt/Music/
    $ find Music/|wc -l
    47732
    $ find OldMusicDir/|wc -l
    47732
    $ du -h -s OldMusicDir/
    1003G OldMusicDir/
    $ du -h -s Music/
    1003G Music/
    $ du -h -s Music/ OldMusicDir/
    1003G Music/
    15M OldMusicDir/

Enjoy!

---

