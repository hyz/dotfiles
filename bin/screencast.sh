#!/bin/bash
# 
# Record screen and microphone audio
# Steven Gordon
#
# Example usage:
#    screencast lecture intro-to-datacomms
# The audio and screen will be recorded. Press 'z' to stop.
# Two files will be created: intro-to-datacomms-audio.flac, intro-to-datacomms-screen.mp4

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

# Add a 5 second delay before starting. This gives us time
# to move the mouse between screens/apps before the screencast
# starts.
echo -n "Starting in 5"
sleep 1
echo -n " 4"
sleep 1
echo -n " 3"
sleep 1
echo -n " 2"
sleep 1
echo  " 1"
sleep 1
echo "Press z to finish..."


# The computer recording: screen sizes differ, e.g. lenovo, samsung
# See the list below
computer=$1
# Basename of file. Separate extensions for audio and screen are added
# E.g. internet-lecture
outfile=$2

# Name and default options for audio output
audioFile=${outfile}-audio.flac
audioRate=44100
audioBits=16
audioChannels=1

# Name and default options for screen output
screenFile=${outfile}-screen.mp4
screenRate=10
screenOffset='+0,0'

# Set the screen size depending on the computer chosen
# You may add your own names/entries depending on your
# screen resolution
if [ ${computer} = "lenovo" ]
then
	# Lenovo laptop, full screen record
	screenSize=1366x768
elif [ ${computer} = "lenovo2" ]
then
	# Lenovo laptop, record all but left menu bar in Ubuntu
	screenSize=1024x768
	screenOffset='+171,0'
	screenRate=10;
elif [ ${computer} = "home" ]
then
	# Home monitor, full screen
	screenSize=1920x1080
elif [ ${computer} = "dell" ]
then
	# Office Dell monitor, full screen
	screenSize=1920x1200
elif [ ${computer} = "samsung" ]
then
	# Samsung netbook, full screen
	screenSize=1024x600
elif [ ${computer} = "netlab" ]
then
	# Network lab, full screen
	screenSize=1600x900
elif [ ${computer} = "office" ]
then
	# Testing in office, laptop plus monitor
	screenSize=1280x1024
	screenOffset='+1366,0'
	screenRate=10;
elif [ ${computer} = "lecture" ]
then
	# SIIT lecture
	# Lenovo laptop + projector
	# Record only projector
	screenSize=1024x768
	screenOffset='+1366,0'
	screenRate=10;
elif [ ${computer} = "both" ]
then
	# Both laptop and project are same resolution
	# Record just projector
	screenSize=1024x768
	screenOffset='+1024,0'
	screenRate=10;
elif [ ${computer} = "mirror" ]
then
	# Mirror displays on laptop and projector
	# Record the entire screen
	    screenSize=1024x768
	    screenOffset=' '
	    screenRate=10;
else
	# Default if none other chosen
	screenSize=800x600
fi

# Video codec, H.264, and options for fast recording
screenCodec=libx264
screenOptions=" -preset ultrafast -crf 0 "

# Record the microphone audio using SoX, putting process in background
rec -q -r ${audioRate} -b ${audioBits} -c ${audioChannels} ${audioFile} &
# Get the process ID (so we can kill it later)
audioPID=$!

# Record the screen using ffmpeg, putting the process in background
${vidconv} -v 0 -f x11grab -r ${screenRate} -s ${screenSize} -i :0.0${screenOffset} ${vidconvopt} -vcodec ${screenCodec} ${screenOptions} ${screenFile} &
# Get the process ID (so we can kill it later)
screenPID=$!

# Enter loop reading input from terminal, stop when 'z' key pressed
op="s"
while [ ${op} != "z" ]
do
	# read captures input key presses from the terminal
	read -n1 op
done

# Kill the SoX and ffmpeg processes, which causes them to stop recording
kill ${audioPID} ${screenPID}

exit
