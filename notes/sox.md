
### http://www.zoharbabin.com/how-to-do-noise-reduction-using-ffmpeg-and-sox/

如何利用ffmpeg和sox进行降噪

    ffmpeg -i input.mp4 -vcodec copy -an tmpvid.mp4
    ffmpeg -i input.mp4 -acodec pcm_s16le -ar 128k -vn tmpaud.wav

    ffmpeg -i input.mp4 -acodec pcm_s16le -ar 128k -vn -ss 00:00:00.0 -t 00:00:00.5 noiseaud.wav

    sox noiseaud.wav -n noiseprof noise.prof
    sox tmpaud.wav tmpaud-clean.wav noisered noise.prof 0.21

    ffmpeg -i tmpvid.mp4 -i tmpaud-clean.wav -map 0:v -map 1:a -c:v copy -c:a aac -b:a 128k out.mp4

### https://www.jianshu.com/p/be8977de4a6b

