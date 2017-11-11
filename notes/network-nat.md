

### http://www.think-like-a-computer.com/2011/09/16/types-of-nat/

Full Cone NAT (Static NAT)
Restricted Cone NAT (Dynamic NAT)
Port Restricted Cone NAT (Dynamic NAT)
Symmetric NAT (Dynamic NAT)

### https://www.vmware.com/support/ws3/doc/ws32_network21.html

Understanding NAT 

    apt-get install coturn stun
    server:
        $ turnserver --stun-only
    client:
        $ stun x.x.x.x
        STUN client version 0.96
        Primary: Independent Mapping, Independent Filter, random port, will hairpin
        Return value is 0x000002


### https://github.com/hanpfei/stund 

### http://www.cnblogs.com/my_life/articles/1908552.html
### http://blog.chinaunix.net/uid-23227798-id-2485820.html

Notes: NAT Addressing and Port Mapping and Filter Behavior - RFC4787

NAT Addressing and Port Mapping
Endpoint-Independent Mapping
    The NAT reuses the port mapping for subsequent packets sent from the same internal IP address and port (X:x) to any external IP address and port. Specifically, X1':x1' equals X2':x2' for all values of Y2:y2.
Address-Dependent Mapping
    The NAT reuses the port mapping for subsequent packets sent from the same internal IP address and port (X:x) to the same external IP address, regardless of the external port. Specifically, X1':x1' equals X2':x2' if and only if, Y2 equals Y1.
Address and Port-Dependent Mapping
    The NAT reuses the port mapping for subsequent packets sent from the same internal IP address and port (X:x) to the same external IP address and port while the mapping is still active. Specially, X1':x1' equals X2:x2' if and only if Y2:y2 equals Y1:y1. 
             
REQ-1: A NAT MUST have an "Endpoint-Independent Mapping" behavior.

NAT Filtering Behavior
    The key behavior to describe is what criteria are used by the NAT to filter packets originating from specific external endpoints.
Endpoint-Independent Filtering
    The NAT filters out only packets not destined to the internal address and port X:x, regardless of the external IP address and port source (Z:z). The NAT forwards any packets destined to X:x. In other words, sending packets from the internal side of the NAT to any external IP address is sufficient to allow any packets back to the internal endpoint.
Address-Dependent Filtering
    The NAT filters out packets not destined to the internal address X:x. Additionally, the NAT will filter out packets from Y:y destined for the internal endpoint X:x if X:x has not sent packets to Y previously(independently of the port used by Y). In other words, for receiving packets from a specific external endpoint, it is necessary for the internal endpoint to send packets first to that specific external endpoint's IP address.
Address and Port-Dependent Filtering
    This is similar to the previours behavior, except that the external port is also relevant. The NAT filters out packets not destined for the internal address X:x. Additionally, the NAT will filter out packets from Y:y destined for the internal endpoint X:x if X:x has not sent packets to Y:y previously. In other words, for receiving packets from a specific external endpoint, it is necessary for the internal endpoint to send packets first to that external endpoint's IP address and port. 

REQ-8: If application transparency is most important, it is RECOMMENDED that a NAT have an "Endpoint-Independent Filtering" behavior. If a more stringent filtering behavior is most import, it is RECOMMENDED that a NAT have an "Address-Dependent Filtering" behavior.
    a) The filtering behavior MAY be an option configurable by the administrator of the NAT.


### https://en.wikipedia.org/wiki/Network_address_translation
### http://www.cnblogs.com/shangdawei/p/3680034.html

### https://github.com/jselbie/stunserver

    > make BOOST_INCLUDE='-I/BOOST_ROOT'
    > ./stunclient --mode full stun.xten.com
    Binding test: success
    Local address: 192.168.2.115:44403
    Mapped address: 119.137.2.248:1446
    Behavior test: success
    Nat behavior: Endpoint Independent Mapping
    Filtering test: success
    Nat filtering: Address and Port Dependent Filtering

    > ./stunclient --mode full stun.b2b2c.ca
    Binding test: success
    Local address: 192.168.2.115:46361
    Mapped address: 119.137.2.248:1949
    Behavior test: success
    Nat behavior: Endpoint Independent Mapping
    Filtering test: success
    Nat filtering: Address and Port Dependent Filtering

### https://github.com/laike9m/PyPunchP2P

### https://en.wikipedia.org/wiki/STUN
### http://www.stunprotocol.org/
### http://olegh.ftp.sh/public-stun.txt

    # 264 STUN-servers. Tested 2017-08-08
    # Author: Oleg Khovayko (olegarch)
    # Distributed under Creative Common License:
    # https://en.wikipedia.org/wiki/Creative_Commons_license

    iphone-stun.strato-iphone.de:3478
    numb.viagenie.ca:3478
    sip1.lakedestiny.cordiaip.com:3478
    stun.12connect.com:3478
    stun.12voip.com:3478
    stun.1cbit.ru:3478
    stun.1und1.de:3478
    stun.2talk.co.nz:3478
    stun.2talk.com:3478
    stun.3clogic.com:3478
    stun.3cx.com:3478
    stun.726.com:3478
    stun.a-mm.tv:3478
    stun.aa.net.uk:3478
    stun.aceweb.com:3478
    stun.acrobits.cz:3478
    stun.acronis.com:3478
    stun.actionvoip.com:3478
    stun.advfn.com:3478
    stun.aeta-audio.com:3478
    stun.aeta.com:3478
    stun.allflac.com:3478
    stun.anlx.net:3478
    stun.antisip.com:3478
    stun.avigora.com:3478
    stun.avigora.fr:3478
    stun.b2b2c.ca:3478
    stun.bahnhof.net:3478
    stun.barracuda.com:3478
    stun.beam.pro:3478
    stun.bitburger.de:3478
    stun.bluesip.net:3478
    stun.bomgar.com:3478
    stun.botonakis.com:3478
    stun.budgetphone.nl:3478
    stun.budgetsip.com:3478
    stun.cablenet-as.net:3478
    stun.callromania.ro:3478
    stun.callwithus.com:3478
    stun.cheapvoip.com:3478
    stun.cloopen.com:3478
    stun.cognitoys.com:3478
    stun.comfi.com:3478
    stun.commpeak.com:3478
    stun.communigate.com:3478
    stun.comrex.com:3478
    stun.comtube.com:3478
    stun.comtube.ru:3478
    stun.connecteddata.com:3478
    stun.cope.es:3478
    stun.counterpath.com:3478
    stun.counterpath.net:3478
    stun.crimeastar.net:3478
    stun.dcalling.de:3478
    stun.demos.ru:3478
    stun.demos.su:3478
    stun.dls.net:3478
    stun.dokom.net:3478
    stun.dowlatow.ru:3478
    stun.duocom.es:3478
    stun.dus.net:3478
    stun.e-fon.ch:3478
    stun.easemob.com:3478
    stun.easycall.pl:3478
    stun.easyvoip.com:3478
    stun.eibach.de:3478
    stun.ekiga.net:3478
    stun.ekir.de:3478
    stun.elitetele.com:3478
    stun.emu.ee:3478
    stun.engineeredarts.co.uk:3478
    stun.eoni.com:3478
    stun.epygi.com:3478
    stun.faktortel.com.au:3478
    stun.fbsbx.com:3478
    stun.fh-stralsund.de:3478
    stun.fmbaros.ru:3478
    stun.fmo.de:3478
    stun.freecall.com:3478
    stun.freeswitch.org:3478
    stun.freevoipdeal.com:3478
    stun.genymotion.com:3478
    stun.gmx.de:3478
    stun.gmx.net:3478
    stun.gnunet.org:3478
    stun.gradwell.com:3478
    stun.halonet.pl:3478
    stun.highfidelity.io:3478
    stun.hoiio.com:3478
    stun.hosteurope.de:3478
    stun.i-stroy.ru:3478
    stun.ideasip.com:3478
    stun.imweb.io:3478
    stun.infra.net:3478
    stun.innovaphone.com:3478
    stun.instantteleseminar.com:3478
    stun.internetcalls.com:3478
    stun.intervoip.com:3478
    stun.ipcomms.net:3478
    stun.ipfire.org:3478
    stun.ippi.com:3478
    stun.ippi.fr:3478
    stun.it1.hr:3478
    stun.ivao.aero:3478
    stun.jabbim.cz:3478
    stun.jumblo.com:3478
    stun.justvoip.com:3478
    stun.kaospilot.dk:3478
    stun.kaseya.com:3478
    stun.kaznpu.kz:3478
    stun.kiwilink.co.nz:3478
    stun.kuaibo.com:3478
    stun.l.google.com:19302
    stun.lamobo.org:3478
    stun.levigo.de:3478
    stun.lindab.com:3478
    stun.linphone.org:3478
    stun.linx.net:3478
    stun.liveo.fr:3478
    stun.lowratevoip.com:3478
    stun.lundimatin.fr:3478
    stun.maestroconference.com:3478
    stun.mangotele.com:3478
    stun.mgn.ru:3478
    stun.mit.de:3478
    stun.miwifi.com:3478
    stun.mixer.com:3478
    stun.modulus.gr:3478
    stun.mrmondialisation.org:3478
    stun.myfreecams.com:3478
    stun.myvoiptraffic.com:3478
    stun.mywatson.it:3478
    stun.nacsworld.com:3478
    stun.nas.net:3478
    stun.nautile.nc:3478
    stun.netappel.com:3478
    stun.nextcloud.com:3478
    stun.nfon.net:3478
    stun.ngine.de:3478
    stun.node4.co.uk:3478
    stun.nonoh.net:3478
    stun.nottingham.ac.uk:3478
    stun.nova.is:3478
    stun.onesuite.com:3478
    stun.onthenet.com.au:3478
    stun.ooma.com:3478
    stun.oovoo.com:3478
    stun.ozekiphone.com:3478
    stun.personal-voip.de:3478
    stun.petcube.com:3478
    stun.pexip.com:3478
    stun.phone.com:3478
    stun.pidgin.im:3478
    stun.pjsip.org:3478
    stun.planete.net:3478
    stun.poivy.com:3478
    stun.powervoip.com:3478
    stun.ppdi.com:3478
    stun.rackco.com:3478
    stun.redworks.nl:3478
    stun.ringostat.com:3478
    stun.rmf.pl:3478
    stun.rockenstein.de:3478
    stun.rolmail.net:3478
    stun.rudtp.ru:3478
    stun.russian-club.net:3478
    stun.rynga.com:3478
    stun.sainf.ru:3478
    stun.schlund.de:3478
    stun.sigmavoip.com:3478
    stun.sip.us:3478
    stun.sipdiscount.com:3478
    stun.sipgate.net:10000
    stun.sipgate.net:3478
    stun.siplogin.de:3478
    stun.sipnet.net:3478
    stun.sipnet.ru:3478
    stun.siportal.it:3478
    stun.sippeer.dk:3478
    stun.siptraffic.com:3478
    stun.sma.de:3478
    stun.smartvoip.com:3478
    stun.smsdiscount.com:3478
    stun.snafu.de:3478
    stun.solcon.nl:3478
    stun.solnet.ch:3478
    stun.sonetel.com:3478
    stun.sonetel.net:3478
    stun.sovtest.ru:3478
    stun.speedy.com.ar:3478
    stun.spoiltheprincess.com:3478
    stun.srce.hr:3478
    stun.ssl7.net:3478
    stun.stunprotocol.org:3478
    stun.swissquote.com:3478
    stun.t-online.de:3478
    stun.talks.by:3478
    stun.tel.lu:3478
    stun.telbo.com:3478
    stun.telefacil.com:3478
    stun.threema.ch:3478
    stun.tng.de:3478
    stun.trueconf.ru:3478
    stun.twt.it:3478
    stun.ucallweconn.net:3478
    stun.ucsb.edu:3478
    stun.ucw.cz:3478
    stun.uiscom.ru:3478
    stun.uls.co.za:3478
    stun.unseen.is:3478
    stun.up.edu.ph:3478
    stun.usfamily.net:3478
    stun.uucall.com:3478
    stun.veoh.com:3478
    stun.vipgroup.net:3478
    stun.viva.gr:3478
    stun.vivox.com:3478
    stun.vline.com:3478
    stun.vmi.se:3478
    stun.vo.lu:3478
    stun.vodafone.ro:3478
    stun.voicetrading.com:3478
    stun.voip.aebc.com:3478
    stun.voip.blackberry.com:3478
    stun.voip.eutelia.it:3478
    stun.voiparound.com:3478
    stun.voipblast.com:3478
    stun.voipbuster.com:3478
    stun.voipbusterpro.com:3478
    stun.voipcheap.co.uk:3478
    stun.voipcheap.com:3478
    stun.voipdiscount.com:3478
    stun.voipfibre.com:3478
    stun.voipgain.com:3478
    stun.voipgate.com:3478
    stun.voipinfocenter.com:3478
    stun.voipplanet.nl:3478
    stun.voippro.com:3478
    stun.voipraider.com:3478
    stun.voipstunt.com:3478
    stun.voipwise.com:3478
    stun.voipzoom.com:3478
    stun.voxgratia.org:3478
    stun.voxox.com:3478
    stun.voztele.com:3478
    stun.wcoil.com:3478
    stun.webcalldirect.com:3478
    stun.whc.net:3478
    stun.whoi.edu:3478
    stun.wifirst.net:3478
    stun.wwdl.net:3478
    stun.xn----8sbcoa5btidn9i.xn--p1ai:3478
    stun.xten.com:3478
    stun.xtratelecom.es:3478
    stun.yy.com:3478
    stun.zadarma.com:3478
    stun.zepter.ru:3478
    stun.zoiper.com:3478
    stun1.faktortel.com.au:3478
    stun1.l.google.com:19302
    stun2.l.google.com:19302
    stun3.l.google.com:19302
    stun4.l.google.com:19302
    stun.zoiper.com:3478
