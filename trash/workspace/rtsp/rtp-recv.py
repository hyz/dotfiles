#!/usr/bin/env python2

"""
A demo python code that ..

1) Connects to an IP cam with RTSP
2) Draws RTP/NAL/H264 packets from the camera
3) Writes them to a file that can be read with any stock video player (say, mplayer, vlc & other ffmpeg based video-players)

Done for educative/demonstrative purposes, not for efficiency..!

written 2015 by Sampsa Riikonen.
http://stackoverflow.com/questions/28022432/receiving-rtp-packets-after-rtsp-setup?rq=1
"""

import socket, sys
import re
import bitstring # if you don't have this from your linux distro, install with "pip install bitstring"

# ************************ FOR QUICK-TESTING EDIT THIS AREA *********************************************************
ip="127.0.0.1" # IP address of your cam
adr='rtsp://127.0.0.1:7654/rtp1' #"rtsp://admin:12345@192.168.1.74" # username, passwd, etc.
clientports=[1234,1235] # the client ports we are going to use for receiving video
#clientports=[60784,60785] # the client ports we are going to use for receiving video
fname="rtp-recv.py.dump" # filename for dumping the stream
rn=5000 # receive this many packets
# After running this program, you can try your file defined in fname with "vlc fname" or "mplayer fname" from the command line
# *******************************************************************************************************************

_describe="DESCRIBE "+adr+" RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: python\r\nAccept: application/sdp\r\n\r\n"
_setup="SETUP "+adr+"/streamid=0 RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: python\r\nTransport: RTP/AVP;unicast;client_port="+str(clientports[0])+"-"+str(clientports[1])+"\r\n\r\n"
#SETUP rtsp://127.0.0.1:7654/test1-rtsp/streamid=1 TRANSPORT RTP/AVP;unicast;client_port=1234-1235
_play="PLAY "+adr+" RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: python\r\nSession: SESID\r\nRange: npt=0.000-\r\n\r\n"

# File organized as follows:
# 1) Strings manipulation routines
# 2) RTP stream handling routine
# 3) Main program



# *** (1) First, some string searching/manipulation for handling the rtsp strings ***

def getPorts(searchst,st):
  """ Searching port numbers from rtsp strings using regular expressions
  """
  pat=re.compile(searchst+"=\d*-\d*")
  pat2=re.compile('\d+')
  mstring=pat.findall(st)[0] # matched string .. "client_port=1000-1001"
  nums=pat2.findall(mstring)
  numas=[]
  for num in nums:
    numas.append(int(num))
  return numas


def getLength(st):
  """ Searching "content-length" from rtsp strings using regular expressions
  """
  pat=re.compile("Content-Length: \d*")
  pat2=re.compile('\d+')
  mstring=pat.findall(st)[0] # matched string.. "Content-Length: 614"
  num=int(pat2.findall(mstring)[0])
  return num


def printrec(recst):
  """ Pretty-printing rtsp strings
  """
  recs=recst.split('\r\n')
  for rec in recs:
    print rec


def sessionid(recst):
  """ Search session id from rtsp strings
  """
  recs=recst.split('\r\n')
  for rec in recs:
    ss=rec.split()
    # print ">",ss
    if (ss[0].strip()=="Session:"):
      return ss[1].split(";")[0].strip()


def setsesid(recst,idn):
  """ Sets session id in an rtsp string
  """
  return recst.replace("SESID", idn)



# ********* (2) The routine for handling the RTP stream ***********

startbytes="\x00\x00\x00\x01" # this is the sequence of four bytes that identifies a NAL packet.. must be in front of every NAL packet.

def digestpacket(rtp_packet):
  """ This routine takes a UDP packet, i.e. a string of bytes and ..
  (a) strips off the RTP header
  (b) adds NAL "stamps" to the packets, so that they are recognized as NAL's
  (c) Concantenates frames
  (d) Returns a packet that can be written to disk as such and that is recognized by stock media players as h264 stream
  """

  bt=bitstring.BitArray(bytes=rtp_packet) # turn the whole string-of-bytes packet into a string of bits.  Very unefficient, but hey, this is only for demoing.  lc=12 # bytecounter
  bc=12*8 # bitcounter

  version=bt[0:2].uint # version
  p=bt[2] # P
  x=bt[3] # X
  cc=bt[4:7].uint # CC
  m=bt[8] # M
  pt=bt[9:16].uint # PT
  sn=bt[16:32].uint # sequence number
  timestamp=bt[32:64].uint # timestamp
  ssrc=bt[64:96].uint # ssrc identifier
  # The header format can be found from:
  # https://en.wikipedia.org/wiki/Real-time_Transport_Protocol

  lc=12 # so, we have red twelve bytes
  bc=12*8 # .. and that many bits

  print "version, p, x, cc, m, pt:",version,p,x,cc,m,pt
  print "sequence number, timestamp",sn,timestamp
  print "sync. source identifier",ssrc

  # rtp_packet=f.read(4*cc) # csrc identifiers, 32 bits (4 bytes) each
  cids=[]
  for i in range(cc):
    cids.append(bt[bc:bc+32].uint)
    bc+=32; lc+=4;
  print "csrc identifiers:",cids

  if (x):
    # this section haven't been tested.. might fail
    hid=bt[bc:bc+16].uint
    bc+=16; lc+=2;

    hlen=bt[bc:bc+16].uint
    bc+=16; lc+=2;

    print "ext. header id, header len",hid,hlen

    hst=bt[bc:bc+32*hlen]
    bc+=32*hlen; lc+=4*hlen;
  print 'rtp-header-size', lc

  # OK, now we enter the NAL packet, as described here:
  # 
  # https://tools.ietf.org/html/rfc6184#section-1.3
  #
  # Some quotes from that document:
  #
  """
  5.3. NAL Unit Header Usage


  The structure and semantics of the NAL unit header were introduced in
  Section 1.3.  For convenience, the format of the NAL unit header is
  reprinted below:

      +---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |F|NRI|  Type   |
      +---------------+

  This section specifies the semantics of F and NRI according to this
  specification.

  """
  """
  Table 3.  Summary of allowed NAL unit types for each packetization
                mode (yes = allowed, no = disallowed, ig = ignore)

      Payload Packet    Single NAL    Non-Interleaved    Interleaved
      Type    Type      Unit Mode           Mode             Mode
      -------------------------------------------------------------
      0      reserved      ig               ig               ig
      1-23   NAL unit     yes              yes               no
      24     STAP-A        no              yes               no
      25     STAP-B        no               no              yes
      26     MTAP16        no               no              yes
      27     MTAP24        no               no              yes
      28     FU-A          no              yes              yes
      29     FU-B          no               no              yes
      30-31  reserved      ig               ig               ig
  """
  # This was also very usefull:
  # http://stackoverflow.com/questions/7665217/how-to-process-raw-udp-packets-so-that-they-can-be-decoded-by-a-decoder-filter-i
  # A quote from that:
  """
  First byte:  [ 3 NAL UNIT BITS | 5 FRAGMENT TYPE BITS] 
  Second byte: [ START BIT | RESERVED BIT | END BIT | 5 NAL UNIT BITS] 
  Other bytes: [... VIDEO FRAGMENT DATA...]
  """

  fb=int(bt[bc]) # i.e. "F"
  nri=bt[bc+1:bc+3].uint # "NRI"
  nlu0=bt[bc:bc+3] # "3 NAL UNIT BITS" (i.e. [F | NRI])
  typ=bt[bc+3:bc+8].uint # "Type"
  print "F, NRI, Type :", fb, nri, typ, 'size', len(rtp_packet)-lc
  print "first three bits together :", nlu0 #bt[bc:bc+3]

  if (typ==7 or typ==8):
    # this means we have either an SPS or a PPS packet
    # they have the meta-info about resolution, etc.
    # more reading for example here:
    # http://www.cardinalpeak.com/blog/the-h-264-sequence-parameter-set/
    if (typ==7):
      print ">>>>> SPS packet"
    else:
      print ">>>>> PPS packet"
    return startbytes+rtp_packet[lc:]
    # .. notice here that we include the NAL starting sequence "startbytes" and the "First byte"
  elif typ == 24:
    bc+=8; lc+=1;
    ret = ''
    while lc+2 < len(rtp_packet):
      siz=bt[bc:bc+16].uint
      bc+=16; lc+=2;
      if lc+siz > len(rtp_packet):
          return ret
      fb=int(bt[bc]) # i.e. "F"
      nri=bt[bc+1:bc+3].uint # "NRI"
      # nlu0=bt[bc:bc+3] # "3 NAL UNIT BITS" (i.e. [F | NRI])
      typ=bt[bc+3:bc+8].uint # "Type"
      print "F, NRI, Type :", fb, nri, typ, 'size', siz, 'STAP-A'
      ret += startbytes + rtp_packet[lc:lc+siz]
      bc += 8*(siz); lc += siz;
    return ret
  elif typ != 28:
    print >>sys.stderr, "unknown nal_unit_type(%d)" % typ
    return None

  bc+=8; lc+=1; # let's go to "Second byte"
  # ********* WE ARE AT THE "Second byte" ************
  # The "Type" here is most likely 28, i.e. "FU-A"
  start=bt[bc] # start bit
  end=bt[bc+1] # end bit
  nlu1=bt[bc+3:bc+8] # 5 nal unit bits

  if (start): # OK, this is a first fragment in a movie frame
    print ">>> first fragment found"
    nlu=nlu0+nlu1 # Create "[3 NAL UNIT BITS | 5 NAL UNIT BITS]"
    head=startbytes+nlu.bytes # .. add the NAL starting sequence
    lc+=1 # We skip the "Second byte"
  if (start==False and end==False): # intermediate fragment in a sequence, just dump "VIDEO FRAGMENT DATA"
    head=""
    lc+=1 # We skip the "Second byte"
  elif (end==True): # last fragment in a sequence, just dump "VIDEO FRAGMENT DATA"
    head=""
    print "<<<< last fragment found"
    lc+=1 # We skip the "Second byte"

  if (typ==28): # This code only handles "Type" = 28, i.e. "FU-A"
    return head+rtp_packet[lc:]
  else:
    print >>sys.stderr, "unknown frame type for this piece of s(%d)" % typ
    return None #raise(Exception,"unknown frame type for this piece of s")

# *********** (3) THE MAIN PROGRAM STARTS HERE ****************

# Create an TCP socket for RTSP communication
# further reading: 
# https://docs.python.org/2.7/howto/sockets.html
sock_stream=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock_stream.connect((ip,7654)) # RTSP should peek out from port 554

print
print "*** SENDING DESCRIBE ***"
print
sock_stream.send(_describe)
recst=sock_stream.recv(4096)
print
print "*** GOT ****"
print
printrec(recst)

print
print "*** SENDING SETUP ***"
print _setup.partition('\r')[0]
sock_stream.send(_setup)
recst=sock_stream.recv(4096)
print
print "*** GOT ****"
print
printrec(recst)
idn=sessionid(recst)

serverports=getPorts("server_port",recst)
clientports=getPorts("client_port",recst)
print "****"
print "ip,serverports",ip,serverports,"clientports",clientports
print "****"

sock_dgram=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_dgram.bind(("", clientports[0])) # we open a port that is visible to the whole internet (the empty string "" takes care of that)
sock_dgram.settimeout(5) # if the socket is dead for 5 s., its thrown into trash
# further reading:
# https://wiki.python.org/moin/UdpCommunication

# Now our port is open for receiving shitloads of videodata.  Give the camera the PLAY command..
print
print "*** SENDING PLAY ***"
print
_play=setsesid(_play,idn)
sock_stream.send(_play)
recst=sock_stream.recv(4096)
print
print "*** GOT ****"
print
printrec(recst)
print
print

print "** STRIPPING RTP INFO AND DUMPING INTO FILE **"
f=open(fname,'w')
for i in range(rn):
  print
  print
  recst=sock_dgram.recv(4096)
  print "read",len(recst),"bytes"
  st=digestpacket(recst)
  if st:
      print "dumping",len(st),"bytes"
      f.write(st)
f.write(startbytes)
f.close()

# Before closing the sockets, we should give the "TEARDOWN" command via RTSP, but I am feeling lazy today (after googling, wireshark-analyzing, among other-things).
sock_stream.close()
sock_dgram.close()

