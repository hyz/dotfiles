#!/bin/bash
#
# Combines a screen and audio file created by screencast to output
# a MP4 video.
# Steven Gordon
#
# Assumes input files are named: x-audio.flac and x-screen.mp4 (or x-screen.m4v)
# Example usage:
#    screenaudio2video x date 
# optionally quality (1, 2 or 3) can be added

inaudio=${1}-audio
inscreen=${1}-screen
lecdate=${2}
quality=${3}

# Check for ffmpeg or avconv
# Once one of them is found, we will use ${vidconv} for the app
# Since ffmpeg and avconv may have different options, we set
# them in ${vidconvopt}
which ffmpeg > /dev/null
result=$?
if [ ${result} -eq 0 ];
then
        vidconv="ffmpeg"
        vidconvopt=" ";
else
        which avconv > /dev/null
        result=$?
        if [ ${result} -eq 0 ];
        then
                vidconv="avconv"
                vidconvopt=" "
        else
                echo "Error: ffmpeg or avconv must be installed. Exiting."
                exit 1;
        fi
fi

# Check for quality of audio
if [ "${quality}" = "1" ]
then
	audiorate=32;
	namequality='-Lo';
elif [ "${quality}" = "3" ]
then
	audiorate=128;
	namequality='-Hi';
# Default audio quality
else
	audiorate=64;
	namequality='';
fi

# Create name of output video
outvideo=${1}-video${namequality}-${lecdate}

# FLAC to WAV to AAC
flac -d ${inaudio}.flac
faac -b ${audiorate} ${inaudio}.wav

# MP4 or M4V accepted for screen
if [ -e ${inscreen}.mp4 ]
then
	screenfile=${inscreen}.mp4
else
	screenfile=${inscreen}.m4v
fi

# AA Audio options
audioopt=" -absf aac_adtstoasc "

# Mux the screen and audio
${vidconv} -i ${screenfile} -i ${inaudio}.aac ${audioopt} -vcodec copy -acodec copy ${outvideo}.mp4

exit
# Remove intermediate files
rm -f ${inaudio}.wav
rm -f ${inaudio}.aac
