
    https://webrtc.github.io/samples/src/content/peerconnection/trickle-ice/

    curl -LO http://olegh.ftp.sh/public-stun.txt

### http://www.cnblogs.com/my_life/articles/1908552.html

NAT的四种类型及类型检测【很好】
Posted on 2010-12-16 19:30 bw_0927 阅读(66956) 评论(2) 编辑 收藏

http://www.h3c.com.cn/MiniSite/Technology_Circle/Net_Reptile/The_Five/Home/Catalog/201206/747042_97665_0.htm

考虑到UDP的无状态特性，目前针对其的NAT实现大致可分为Full Cone、Restricted Cone、Port Restricted Cone和Symmetric NAT四种。值得指出的是，对于TCP协议而言，一般来说，目前NAT中针对TCP的实现基本上是一致的，其间并不存在太大差异，这是因为TCP协议本身 便是面向连接的，因此无需考虑网络连接无状态所带来复杂性。

用语定义

1.内部Tuple：指内部主机的私有地址和端口号所构成的二元组，即内部主机所发送报文的源地址、端口所构成的二元组
2.外部Tuple：指内部Tuple经过NAT的源地址/端口转换之后，所获得的外部地址、端口所构成的二元组，即外部主机收到经NAT转换之后的报文时，它所看到的该报文的源地址（通常是NAT设备的地址）和源端口
3.目标Tuple：指外部主机的地址、端口所构成的二元组，即内部主机所发送报文的目标地址、端口所构成的二元组

详细释义

1. Full Cone NAT：所有来自同一 个内部Tuple X的请求均被NAT转换至同一个外部Tuple Y，而不管这些请求是不是属于同一个应用或者是多个应用的。除此之外，当X-Y的转换关系建立之后，任意外部主机均可随时将Y中的地址和端口作为目标地址 和目标端口，向内部主机发送UDP报文，由于对外部请求的来源无任何限制，因此这种方式虽然足够简单，但却不那么安全

2. Restricted Cone NAT： 它是Full Cone的受限版本：所有来自同一个内部Tuple X的请求均被NAT转换至同一个外部Tuple Y，这与Full Cone相同，但不同的是，只有当内部主机曾经发送过报文给外部主机（假设其IP地址为Z）后，外部主机才能以Y中的信息作为目标地址和目标端口，向内部 主机发送UDP请求报文，这意味着，NAT设备只向内转发（目标地址/端口转换）那些来自于当前已知的外部主机的UDP报文，从而保障了外部请求来源的安 全性

3. Port Restricted Cone NAT：它是Restricted Cone NAT的进一步受限版。只有当内部主机曾经发送过报文给外部主机（假设其IP地址为Z且端口为P）之后，外部主机才能以Y中的信息作为目标地址和目标端 口，向内部主机发送UDP报文，同时，其请求报文的源端口必须为P，这一要求进一步强化了对外部报文请求来源的限制，从而较Restrictd Cone更具安全性

4. Symmetric NAT：这是一种比所有Cone NAT都要更为灵活的转换方式：在Cone NAT中，内部主机的内部Tuple与外部Tuple的转换映射关系是独立于内部主机所发出的UDP报文中的目标地址及端口的，即与目标Tuple无关；

在Symmetric NAT中，目标Tuple则成为了NAT设备建立转换关系的一个重要考量：只有来自于同一个内部Tuple 、且针对同一目标Tuple的请求才被NAT转换至同一个外部Tuple，否则的话，NAT将为之分配一个新的外部Tuple；

打个比方，当内部主机以相 同的内部Tuple对2个不同的目标Tuple发送UDP报文时，此时NAT将会为内部主机分配两个不同的外部Tuple，并且建立起两个不同的内、外部 Tuple转换关系。与此同时，只有接收到了内部主机所发送的数据包的外部主机才能向内部主机返回UDP报文，这里对外部返回报文来源的限制是与Port Restricted Cone一致的。不难看出，如果说Full Cone是要求最宽松NAT UDP转换方式，那么，Symmetric NAT则是要求最严格的NAT方式，其不仅体现在转换关系的建立上，而且还体现在对外部报文来源的限制方面。


P2P的NAT研究
第一部分：NAT介绍
第二部分：NAT类型检测

第一部分： NAT介绍
各种不同类型的NAT(according to RFC)
Full Cone NAT:
   内网主机建立一个UDP socket(LocalIP:LocalPort) 第一次使用这个socket给外部主机发送数据时NAT会给其分配一个公网(PublicIP:PublicPort),以后用这个socket向外面任何主机发送数据都将使用这对(PublicIP:PublicPort)。此外，任何外部主机只要知道这个(PublicIP:PublicPort)就可以发送数据给(PublicIP:PublicPort)，内网的主机就能收到这个数据包
Restricted Cone NAT:
   内网主机建立一个UDP socket(LocalIP:LocalPort) 第一次使用这个socket给外部主机发送数据时NAT会给其分配一个公网(PublicIP:PublicPort),以后用这个socket向外面任何主机发送数据都将使用这对(PublicIP:PublicPort)。此外，如果任何外部主机想要发送数据给这个内网主机，只要知道这个(PublicIP:PublicPort)并且内网主机之前用这个socket曾向这个外部主机IP发送过数据。只要满足这两个条件，这个外部主机就可以用自己的(IP,任何端口)发送数据给(PublicIP:PublicPort)，内网的主机就能收到这个数据包
Port Restricted Cone NAT:
    内网主机建立一个UDP socket(LocalIP:LocalPort) 第一次使用这个socket给外部主机发送数据时NAT会给其分配一个公网(PublicIP:PublicPort),以后用这个socket向外面任何主机发送数据都将使用这对(PublicIP:PublicPort)。此外，如果任何外部主机想要发送数据给这个内网主机，只要知道这个(PublicIP:PublicPort)并且内网主机之前用这个socket曾向这个外部主机(IP,Port)发送过数据。只要满足这两个条件，这个外部主机就可以用自己的(IP,Port)发送数据给(PublicIP:PublicPort)，内网的主机就能收到这个数据包
Symmetric NAT:
    内网主机建立一个UDP socket(LocalIP,LocalPort),当用这个socket第一次发数据给外部主机1时,NAT为其映射一个(PublicIP-1,Port-1),以后内网主机发送给外部主机1的所有数据都是用这个(PublicIP-1,Port-1)； 如果内网主机同时用这个socket给外部主机2发送数据，第一次发送时，NAT会为其分配一个(PublicIP-2,Port-2), 以后内网主机发送给外部主机2的所有数据都是用这个(PublicIP-2,Port-2).

如果NAT有多于一个公网IP，则PublicIP-1和PublicIP-2可能不同，如果NAT只有一个公网IP,则Port-1和Port-2肯定不同，也就是说一定不能是PublicIP-1等于 PublicIP-2且Port-1等于Port-2。

此外，如果任何外部主机想要发送数据给这个内网主机，那么它首先应该收到内网主机发给他的数据，然后才能往回发送，否则即使他知道内网主机的一个(PublicIP,Port)也不能发送数据给内网主机，这种NAT无法实现UDP-P2P通信。

 

 

第二部：NAT类型检测

前提条件:有一个公网的Server并且绑定了两个公网IP(IP-1,IP-2)。这个Server做UDP监听(IP-1,Port-1),(IP-2,Port-2)并根据客户端的要求进行应答。

第一步：检测客户端是否有能力进行UDP通信以及客户端是否位于NAT后？

客户端建立UDP socket然后用这个socket向服务器的(IP-1,Port-1)发送数据包要求服务器返回客户端的IP和Port, 客户端发送请求后立即开始接受数据包，要设定socket Timeout（300ms），防止无限堵塞. 重复这个过程若干次。如果每次都超时，无法接受到服务器的回应，则说明客户端无法进行UDP通信，可能是防火墙或NAT阻止UDP通信，这样的客户端也就 不能P2P了（检测停止）。
当客户端能够接收到服务器的回应时，需要把服务器返回的客户端（IP,Port）和这个客户端socket的 （LocalIP，LocalPort）比较。如果完全相同则客户端不在NAT后，这样的客户端具有公网IP可以直接监听UDP端口接收数据进行通信（检 测停止）。否则客户端在NAT后要做进一步的NAT类型检测(继续)。

第二步：检测客户端NAT是否是Full Cone NAT？

客户端建立UDP socket然后用这个socket向服务器的(IP-1,Port-1)发送数据包要求服务器用另一对(IP-2,Port-2)响应客户端的请求往回 发一个数据包,客户端发送请求后立即开始接受数据包，要设定socket Timeout（300ms），防止无限堵塞. 重复这个过程若干次。如果每次都超时，无法接受到服务器的回应，则说明客户端的NAT不是一个Full Cone NAT，具体类型有待下一步检测(继续)。如果能够接受到服务器从(IP-2,Port-2)返回的应答UDP包，则说明客户端是一个Full Cone NAT，这样的客户端能够进行UDP-P2P通信（检测停止）。

第三步：检测客户端NAT是否是Symmetric NAT？

客户端建立UDP socket然后用这个socket向服务器的(IP-1,Port-1)发送数据包要求服务器返回客户端的IP和Port, 客户端发送请求后立即开始接受数据包，要设定socket Timeout（300ms），防止无限堵塞. 重复这个过程直到收到回应（一定能够收到，因为第一步保证了这个客户端可以进行UDP通信）。
用同样的方法用一个socket向服务器的(IP-2,Port-2)发送数据包要求服务器返回客户端的IP和Port。
比 较上面两个过程从服务器返回的客户端(IP,Port),如果两个过程返回的(IP,Port)有一对不同则说明客户端为Symmetric NAT，这样的客户端无法进行UDP-P2P通信（检测停止）。否则是Restricted Cone NAT，是否为Port Restricted Cone NAT有待检测(继续)。

第四步：检测客户端NAT是否是Restricted Cone NAT还是Port Restricted Cone NAT？

客户端建立UDP socket然后用这个socket向服务器的(IP-1,Port-1)发送数据包要求服务器用IP-1和一个不同于Port-1的端口发送一个UDP 数据包响应客户端, 客户端发送请求后立即开始接受数据包，要设定socket Timeout（300ms），防止无限堵塞. 重复这个过程若干次。如果每次都超时，无法接受到服务器的回应，则说明客户端是一个Port Restricted Cone NAT，如果能够收到服务器的响应则说明客户端是一个Restricted Cone NAT。以上两种NAT都可以进行UDP-P2P通信。

注：以上检测过程中只说明了可否进行UDP-P2P的打洞通信，具体怎么通信一般要借助于Rendezvous Server。另外对于Symmetric NAT不是说完全不能进行UDP-P2P达洞通信，可以进行端口预测打洞，不过不能保证成功。

 

=====================

https://zhuanlan.zhihu.com/p/26796476
1. 术语

防火墙（Firewall）： 防火墙主要限制内网和公网的通讯，通常丢弃未经许可的数据包。防火墙会检测(但是不修改)试图进入内网数据包的IP地址和TCP/UDP端口信息。

网络地址转换器（NAT）： NAT不止检查进入数据包的头部，而且对其进行修改，从而实现同一内网中不同主机共用更少的公网IP（通常是一个）。

基本NAT（Basic NAT）： 基本NAT会将内网主机的IP地址映射为一个公网IP，不改变其TCP/UDP端口号。基本NAT通常只有在当NAT有公网IP池的时候才有用。

网络地址-端口转换器（NAPT）： 到目前为止最常见的即为NAPT，其检测并修改出入数据包的IP地址和端口号，从而允许多个内网主机同时共享一个公网IP地址。

锥形NAT（Cone NAT）： 在建立了一对（公网IP，公网端口）和（内网IP，内网端口）二元组的绑定之后，Cone NAT会重用这组绑定用于接下来该应用程序的所有会话（同一内网IP和端口），只要还有一个会话还是激活的。

例如，假设客户端A建立了两个连续的对外会话，从相同的内部端点（10.0.0.1:1234）到两个不同的外部服务端S1和S2。

Cone NAT只为两个会话映射了一个公网端点（155.99.25.11:62000）， 确保客户端端口的“身份”在地址转换的时候保持不变。由于基本NAT和防火墙都不改变数据包的端口号，因此这些类型的中间件也可以看作是退化的Cone NAT。

 

    Server S1                                     Server S2
18.181.0.31:1235                              138.76.29.7:1235
       |                                             |
       |                                             |
       +----------------------+----------------------+
                              |
  ^  Session 1 (A-S1)  ^      |      ^  Session 2 (A-S2)  ^
  |  18.181.0.31:1235  |      |      |  138.76.29.7:1235  |
  v 155.99.25.11:62000 v      |      v 155.99.25.11:62000 v
                              |
                           Cone NAT
                         155.99.25.11
                              |
  ^  Session 1 (A-S1)  ^      |      ^  Session 2 (A-S2)  ^
  |  18.181.0.31:1235  |      |      |  138.76.29.7:1235  |
  v   10.0.0.1:1234    v      |      v   10.0.0.1:1234    v
                              |
                           Client A
                        10.0.0.1:1234

其中Cone NAT根据NAT如何接收已经建立的（公网IP，公网端口）对的输入数据还可以细分为以下三类：

    1) 全锥形NAT（Full Cone NAT） 在一个新会话建立了公网/内网端口绑定之后，全锥形NAT接下来会接受对应公网端口的所有数据，无论是来自哪个（公网）终端。 全锥NAT有时候也被称为“混杂”NAT（promiscuous NAT）。

    2) 受限锥形NAT（Restricted Cone NAT） 受限锥形NAT只会转发符合某个条件的输入数据包。条件为：外部（源）IP地址匹配内网主机之前发送一个或多个数据包的结点的IP地址。 AT通过限制输入数据包为一组“已知的”外部IP地址，有效地精简了防火墙的规则。

    3) 端口受限锥形NAT（Port-Restricted Cone NAT） 端口受限锥形NAT也类似，只当外部数据包的IP地址和端口号都匹配内网主机发送过的地址和端口号时才进行转发。 端口受限锥形NAT为内部结点提供了和对称NAT相同等级的保护，以隔离未关联的数据。

 

上面3种类型，统称为Cone NAT，有一个共同点：只要是从同一个内部地址和端口出来的包，NAT都将它转换成同一个外部地址和端口。

但是Symmetric有点不同，具体表现在： 只要是从同一个内部地址和端口出来，且到同一个外部目标地址和端口，则NAT也都将它转换成同一个外部地址和端口。

但如果从同一个内部地址和端口出来，是 到另一个外部目标地址和端口，则NAT将使用不同的映射，转换成不同的端口（外部地址只有一个，故不变）。

而且和Port Restricted Cone一样，只有曾经收到过内部地址发来包的外部地址，才能通过NAT映射后的地址向该内部地址发包。

对称NAT（Symmetric NAT）: 对称NAT正好相反，不在所有公网-内网对的会话中维持一个固定的端口绑定。其为每个新的会话开辟一个新的端口。如下图所示：

 

   Server S1                                     Server S2
18.181.0.31:1235                              138.76.29.7:1235
       |                                             |
       |                                             |
       +----------------------+----------------------+
                              |
  ^  Session 1 (A-S1)  ^      |      ^  Session 2 (A-S2)  ^
  |  18.181.0.31:1235  |      |      |  138.76.29.7:1235  |
  v 155.99.25.11:62000 v      |      v 155.99.25.11:62001 v
                              |
                         Symmetric NAT
                         155.99.25.11
                              |
  ^  Session 1 (A-S1)  ^      |      ^  Session 2 (A-S2)  ^
  |  18.181.0.31:1235  |      |      |  138.76.29.7:1235  |
  v   10.0.0.1:1234    v      |      v   10.0.0.1:1234    v
                              |
                           Client A
                        10.0.0.1:1234

 
 
2. P2P通信

根据客户端的不同，客户端之间进行P2P传输的方法也略有不同，这里介绍了现有的穿越中间件进行P2P通信的几种技术。
2.1 中继（Relaying）

这是最可靠但也是最低效的一种P2P通信实现。其原理是通过一个有公网IP的服务器中间人对两个内网客户端的通信数据进行中继和转发。如下图所示：

 

                       Server S
                          |
                          |
   +----------------------+----------------------+
   |                                             |
 NAT A                                         NAT B
   |                                             |
   |                                             |
Client A                                      Client B

客户端A和客户端B不直接通信，而是先都与服务端S建立链接，然后再通过S和对方建立的通路来中继传递的数据。这钟方法的缺陷很明显， 当链接的客户端变多之后，会显著增加服务器的负担，完全没体现出P2P的优势。但这种方法的好处是能保证成功，因此在实践中也常作为一种备选方案。
2.2 逆向链接（Connection reversal）

第二种方法在当两个端点中有一个不存在中间件的时候有效。例如，客户端A在NAT之后而客户端B拥有全局IP地址，如下图：

 

                            Server S
                        18.181.0.31:1235
                               |
                               |
        +----------------------+----------------------+
        |                                             |
      NAT A                                           |
155.99.25.11:62000                                    |
        |                                             |
        |                                             |
     Client A                                      Client B
  10.0.0.1:1234                               138.76.29.7:1234　

客户端A内网地址为10.0.0.1，且应用程序正在使用TCP端口1234。A和服务器S建立了一个链接，服务器的IP地址为18.181.0.31，监听1235端口。NAT A给客户端A分配了TCP端口62000，地址为NAT的公网IP地址155.99.25.11， 作为客户端A对外当前会话的临时IP和端口。因此S认为客户端A就是155.99.25.11:62000。而B由于有公网地址，所以对S来说B就是138.76.29.7:1234。

当客户端B想要发起一个对客户端A的P2P链接时，要么链接A的外网地址155.99.25.11:62000，要么链接A的内网地址10.0.0.1:1234，然而两种方式链接都会失败。

链接10.0.0.1:1234失败自不用说，为什么链接155.99.25.11:62000也会失败呢？来自B的TCP SYN握手请求到达NAT A的时候会被拒绝，因为对NAT A来说只有外出的链接才是允许的。

在直接链接A失败之后，B可以通过S向A中继一个链接请求，从而从A方向“逆向“地建立起A-B之间的点对点链接。

很多当前的P2P系统都实现了这种技术，但其局限性也是很明显的，只有当其中一方有公网IP时链接才能建立。越来越多的情况下， 通信的双方都在NAT之后，因此就要用到我们下面介绍的第三种技术了。

 
2.3 UDP打洞（UDP hole punching）

第三种P2P通信技术，被广泛采用的，名为“P2P打洞“。P2P打洞技术依赖于通常防火墙和cone NAT允许正当的P2P应用程序在中间件中打洞且与对方建立直接链接的特性。 以下主要考虑两种常见的场景，以及应用程序如何设计去完美地处理这些情况。第一种场景代表了大多数情况，即两个需要直接链接的客户端处在两个不同的NAT 之后；第二种场景是两个客户端在同一个NAT之后，但客户端自己并不需要知道。

2.3.1. 端点在不同的NAT之后

假设客户端A和客户端B的地址都是内网地址，且在不同的NAT后面。A、B上运行的P2P应用程序和服务器S都使用了UDP端口1234，A和B分别初始化了 与Server的UDP通信，地址映射如图所示:

 

                            Server S
                        18.181.0.31:1234
                               |
                               |
        +----------------------+----------------------+
        |                                             |
      NAT A                                         NAT B
155.99.25.11:62000                            138.76.29.7:31000
        |                                             |
        |                                             |
     Client A                                      Client B
  10.0.0.1:1234                                 10.1.1.3:1234

现在假设客户端A打算与客户端B直接建立一个UDP通信会话。如果A直接给B的公网地址138.76.29.7:31000发送UDP数据，NAT B将很可能会无视进入的 数据（除非是Full Cone NAT），因为源地址和端口与S不匹配，而最初只与S建立过会话。B往A直接发信息也类似。

假设A开始给B的公网地址发送UDP数据的同时，给服务器S发送一个中继请求，要求B开始给A的公网地址发送UDP信息。A往B的输出信息会导致NAT A打开 一个A的内网地址与与B的外网地址之间的新通讯会话，B往A亦然。一旦新的UDP会话在两个方向都打开之后，客户端A和客户端B就能直接通讯， 而无须再通过引导服务器S了。

UDP打洞技术有许多有用的性质。一旦一个的P2P链接建立，链接的双方都能反过来作为“引导服务器”来帮助其他中间件后的客户端进行打洞， 极大减少了服务器的负载。应用程序不需要知道中间件具体是什么（如果有的话），因为以上的过程在没有中间件或者有多个中间件的情况下 也一样能建立通信链路。

2.3.2. 端点在相同的NAT之后

现在考虑这样一种情景，两个客户端A和B正好在同一个NAT之后（而且可能他们自己并不知道），因此在同一个内网网段之内。 客户端A和服务器S建立了一个UDP会话，NAT为此分配了公网端口62000，B同样和S建立会话，分配到了端口62001，如下图：

 

                          Server S
                      18.181.0.31:1234
                             |
                             |
                            NAT
                   A-S 155.99.25.11:62000
                   B-S 155.99.25.11:62001
                             |
      +----------------------+----------------------+
      |                                             |
   Client A                                      Client B
10.0.0.1:1234                                 10.1.1.3:1234

假设A和B使用了上节介绍的UDP打洞技术来建立P2P通路，那么会发生什么呢？首先A和B会得到由S观测到的对方的公网IP和端口号，然后给对方的地址发送信息。 两个客户端只有在NAT允许内网主机对内网其他主机发起UDP会话的时候才能正常通信，我们把这种情况称之为”回环传输“(lookback translation)，因为从内部 到达NAT的数据会被“回送”到内网中而不是转发到外网。例如，当A发送一个UDP数据包给B的公网地址时，数据包最初有源IP地址和端口地址10.0.0.1:1234和 目的地址155.99.25.11:62001，NAT收到包后，将其转换为源155.99.25.11:62000（A的公网地址）和目的10.1.1.3:1234，然后再转发给B。即便NAT支持 回环传输，这种转换和转发在此情况下也是没必要的，且有可能会增加A与B的对话延时和加重NAT的负担。

对于这个情况，优化方案是很直观的。当A和B最初通过S交换地址信息时，他们应该包含自身的IP地址和端口号（从自己看），同时也包含从服务器看的自己的 地址和端口号。然后客户端同时开始从对方已知的两个的地址中同时开始互相发送数据，并使用第一个成功通信的地址作为对方地址。如果两个客户端在同一个 NAT后，发送到对方内网地址的数据最有可能先到达，从而可以建立一条不经过NAT的通信链路；如果两个客户端在不同的NAT之后，发送给对方内网地址的数据包 根本就到达不了对方，但仍然可以通过公网地址来建立通路。值得一提的是，虽然这些数据包通过某种方式验证，但是在不同NAT的情况下完全有可能会导致A往B 发送的信息发送到其他A内网网段中无关的结点上去的。

2.3.3. 端点在多级NAT之后

在一些拓朴结构中，可能会存在多级NAT设备，在这种情况下，如果没有关于拓朴的具体信息， 两个Peer要建立“最优”的P2P链接是不可能的，下面来说为什么。以下图为例：

 

                            Server S
                        18.181.0.31:1234
                               |
                               |
                             NAT X
                     A-S 155.99.25.11:62000
                     B-S 155.99.25.11:62001
                               |
                               |
        +----------------------+----------------------+
        |                                             |
      NAT A                                         NAT B
192.168.1.1:30000                             192.168.1.2:31000
        |                                             |
        |                                             |
     Client A                                      Client B
  10.0.0.1:1234                                 10.1.1.3:1234

假设NAT X是一个网络提供商ISP部署的工业级NAT，其下子网共用一个公网地址155.99.25.11，NAT A和NAT B分别是其下不同用户的网关部署的NAT。只有服务器S 和NAT X有全局的路由地址。Client A在NAT A的子网中，同时Client B在NAT B的子网中，每经过一级NAT都要进行一次网络地址转换。

现在假设A和B打算建立直接P2P链接，用一般的方法（通过Server S来打洞）自然是没问题的，那能不能优化呢？一种想当然的优化办法是A直接把信息发送给NAT B的 内网地址192.168.1.2:31000，且B通过NAT B把信息发送给A的路由地址192.168.1.1:30000，不幸的是，A和B都没有办法得知这两个目的地址，因为S只看见了客户端 ‵全局‵地址155.99.25.11。退一步说，即便A和B通过某种方法得知了那些地址，我们也无法保证他们是可用的。因为ISP分配的子网地址可能和NAT A B分配的子网地址 域相冲突。因此客户端没有其他选择，只能使用S来进行打洞并进行回环传输。

2.3.4. 固定端口绑定

UDP打洞技术有一个主要的条件：只有当两个NAT都是Cone NAT（或者非NAT的防火墙）时才能工作。因为其维持了一个给定的（内网IP，内网UDP）二元组 和（公网IP， 公网UDP）二元组固定的端口绑定，只要该UDP端口还在使用中，就不会变化。如果像对称NAT一样，给每个新会话分配一个新的公网端口，就 会导致UDP应用程序无法使用跟外部端点已经打通了的通信链路。由于Cone NAT是当今最广泛使用的，尽管有一小部分的对称NAT是不支持打洞的，UDP打洞 技术也还是被广泛采纳应用。
3.具体实现

一般的网络编程，都是客户端比服务端要难，因为要处理与服务器的通信同时还要处理来自用户的事件；对于P2P客户端来说更是如此，因为P2P客户端不止作 为客户端，同时也作为对等连接的服务器端。这里的大体思路是，输入命令传输给服务器之后，接收来自服务器的反馈，并执行相应代码。例如A想要与B建立 通信链路，先给服务器发送punch命令以及给B发送数据，服务器接到命令后给B发送punch_requst信息以及A的端点信息，B收到之后向A发送数据打通通路，然 后A与B就可以进行P2P通信了。经测试，打通通路后即便把服务器关闭，A与B也能正常通信。

一个UDP打洞的例子见P2P-Over-MiddleBoxes-Demo
UPDATE 2016-04-06

关于TCP打洞，有一点需要提的是，因为TCP是基于连接的，所以任何未经连接而发送的数据都会被丢弃，这导致在recv的时候是无法直接从peer端读取数据。 其实这对UDP也一样，如果对UDP的socket进行了connect，其也会忽略连接之外的数据，详见connect(2)。

所以，如果我们要进行TCP打洞，通常需要重用本地的endpoint来发起新的TCP连接，这样才能将已经打开的NAT利用起来。具体来说，则是要设置socket的 SO_REUSEADDR或SO_REUSEPORT属性，根据系统不同，其实现也不尽一致。一般来说，TCP打洞的步骤如下：

    A 发送 SYN 到 B （出口地址，下同），从而创建NAT A的一组映射
    B 发送 SYN 到 A， 创建NAT B的一组映射
    根据时序不同，两个SYN中有一个会被对方的NAT丢弃，另一个成功通过NAT
    通过NAT的SYN报文被其中一方收到，即返回SYNACK， 完成握手
    至此，TCP的打洞成功，获得一个不依赖于服务器的链接

博客地址:

Pans Labyrinthhttp://pppan.net/有价值炮灰-博客园

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

### http://www.stunprotocol.org/
### https://en.wikipedia.org/wiki/STUN
### http://olegh.ftp.sh/public-stun.txt

    # 264 STUN-servers. Tested 2017-08-08
    # Author: Oleg Khovayko (olegarch)
    # Distributed under Creative Common License:
    # https://en.wikipedia.org/wiki/Creative_Commons_license

    iphone-stun.strato-iphone.de:3478
    ...
    stun1.l.google.com:19302
    stun2.l.google.com:19302
    stun3.l.google.com:19302
    stun4.l.google.com:19302
    stun.zoiper.com:3478

### https://www.cnblogs.com/dyufei/p/7466924.html

CONE NAT 和 Symmetric NAT

1. NAT 的划分

    RFC3489 中将 NAT 的实现分为四大类：

    Full Cone NAT 完全锥形 NAT
    Restricted Cone NAT 限制锥形 NAT （可以理解为 IP 限制）
    Port Restricted Cone NAT 端口限制锥形 NAT （ IP+Port 限制）
    Symmetric NAT 对称 NAT

其中完全锥形的穿透性最好，而对称形的安全性最高

1.1 锥形NAT与对称NAT的区别

所谓锥形NAT 是指：只要是从同一个内部地址和端口出来的包，无论目的地址是否相同，NAT 都将它转换成同一个外部地址和端口。

“同一个外部地址和端口”与“无论目的地址是否相同”形成了一个类似锥形的网络结构，也是这一名称的由来。反过来，不满足这一条件的即为对称NAT 。

1.2 举例说明

假设：

- NAT 内的主机 A ： IP 记为 A ，使用端口 1000
- NAT 网关 ： IP 记为 NAT ，用于 NAT 的端口池假设为（ 5001-5999 ）
- 公网上的主机 B ： IP 记为B ，开放端口 2000
- 公网上的主机 C ： IP 记为C ，开放端口 3000
- 假设主机 A 先后访问主机 B 和 C

1 ）如果是锥形 NAT ：

那么成功连接后，状态必然如下：

    A （ 1000 ） —— > NAT （ 5001 ）—— > B （ 2000 ）
    A （ 1000 ） —— > NAT （ 5001 ）—— > C （ 3000 ）

也就是说，只要是从 A 主机的 1000 端口发出的包，经过地址转换后的源端口一定相同。

2 ）如果是对称形 NAT ：

连接后，状态有可能（注意是可能，不是一定）如下：

    A （ 1000 ） —— > NAT （ 5001 ）—— > B （ 2000 ）
    A （ 1000 ） —— > NAT （ 5002 ）—— > C （ 3000 ）

两者的区别显而易见。

1.3 三种CONE NAT之间的区别

仍然以上面的网络环境为例, 假设 A 先与 B 建立了连接:

    A （ 1000 ） —— > NAT （ 5001 ）——— > B （ 2000 ）

1） Port Restricted Cone NAT:

只有 B （ 2000 ）发往 NAT （ 5001 ）的数据包可以到达 A （ 1000 ）

    B （ 2000 ） —— >  NAT （ 5001 ） ——— >   A （ 1000 ）
    B （ 3000 ） —— >  NAT （ 5001 ） — X — >   A （ 1000 ）
    C （ 2000 ） —— >  NAT （ 5001 ） — X — >   A （ 1000 ）

2） Restricted Cone NAT

只要是从 B 主机发往 NAT （ 5001 ）的数据包都可以到达 A （ 1000 ）

    B （ 2000 ） —— >  NAT （ 5001 ） ——— >   A （ 1000 ）
    B （ 3000 ） —— >  NAT （ 5001 ） ——— >   A （ 1000 ）
    C （ 2000 ） —— >  NAT （ 5001 ） — X — >   A （ 1000 ）

3） Full Cone NAT

任意地址发往 NAT （ 5001 ）的数据包都可以到达 A （ 1000 ）

    B （ 2000 ） —— >  NAT （ 5001 ） ——— >   A （ 1000 ）
    B （ 3000 ） —— >  NAT （ 5001 ） ——— >   A （ 1000 ）
    C （ 3000 ） —— >  NAT （ 5001 ） ——— >   A （ 1000 ）

2. Linux的NAT

Linux的NAT“MASQUERADE”属于对称形NAT。说明这一点只需要否定 MASQUERADE 为锥形 NAT 即可。

linux 在进行地址转换时，会遵循两个原则：

    尽量不去修改源端口，也就是说，ip 伪装后的源端口尽可能保持不变。
    更为重要的是，ip 伪装后必须 保证伪装后的源地址/ 端口与目标地址/ 端口（即所谓的socket ）唯一。

假设如下的情况（ 内网有主机 A 和 D ，公网有主机 B 和 C ）：
先后 建立如下三条连接：

    A （ 1000 ） —— >  NAT （ 1000 ）—— >  B （ 2000 ）
    D （ 1000 ） —— >  NAT （ 1000 ）—— >  C （ 2000 ）
    A （ 1000 ） —— >  NAT （ 1001 ）—— >  C （ 2000 ）

可以看到，前两条连接遵循了原则 1 ，并且不违背原则 2 而第三条连接为了避免与第二条产生相同的 socket 而改变了源端口比较第一和第三条连接，同样来自 A(1000) 的数据包在经过 NAT 后源端口分别变为了 1000 和1001 。说明 Linux 的 NAT 是对称 NAT 。
3. 对协议的支持

CONENAT 要求原始源地址端口相同的数据包经过地址转换后，新源地址和端口也相同，换句话说，原始源地址端口不同的数据包，转换后的源地址和端口也一定不同。

那么，是不是 Full Cone NAT 的可穿透性一定比 Symmetric NAT 要好呢，或者说，通过 Symmetric NAT 可以建立的连接，如果换成 Full Cone NAT 是不是也一定能成功呢？

假设如下的情况：

（内网有主机Ａ和Ｄ，公网有主机Ｂ和Ｃ，某 UDP 协议服务端口为 2000 ，并且要求客户端的源端口一定为 1000 。 ）
１）如果Ａ使用该协议访问Ｂ：

    A （ 1000 ） —— > NAT （ 1000 ）——— > B （ 2000 ）

由于 Linux 有尽量不改变源端口的规则，因此在 1000 端口未被占用时，连接是可以正常建立的如果此时Ｄ也需要访问Ｂ：

    D （ 1000 ） —— > NAT （ 1001 ）—Ｘ— > B （ 2000 ）

端口必须要改变了，否则将出现两个相同的 socket ，后续由 B(2000) 发往ＮＡＴ（ 1000 ）的包将不知道是转发给Ａ还是Ｄ。于是Ｂ将因为客户端的源端口错误而拒绝连接。在这种情况下， MASQUERADE 与 CONENAT 的表现相同。

####２）如果Ａ连接Ｂ后，Ｄ也像Ｃ发起连接，而在此之后，Ａ又向Ｃ发起连接

    ① A （ 1000 ） —— > NAT （ 1000 ）——— > B （ 2000 ）

如果是 MASQUERADE ：

    ② D （ 1000 ） —— > NAT （ 1000 ）——— > C （ 2000 ）
    ③ A （ 1000 ） —— > NAT （ 1001 ）—Ｘ— > C （ 2000 ）

如果是 CONENAT ：

    ② D （ 1000 ） —— > NAT （ 1001 ）—Ｘ— > C （ 2000 ）
    ③ A （ 1000 ） —— > NAT （ 1000 ）——— > C （ 2000 ）

对于 MASQUERADE 来说，只要在没有重复的 socket 的情况下，总是坚持尽量不改变源端口的原则，因此第二条连接仍然采用源端口 1000 ，而第三条连接为了避免重复的 socket 而改变了端口。

对于 CONENAT ，为了保证所有来自 A(1000) 的数据包均被转换为 NAT(1000) ，因此 D 在向 C 发起连接时，即使不会产生重复的 socket ，但因为 NAT 的 1000 端口已经被 A(1000) “占用”了，只好使用新的端口。

可以看出，不同的 target 产生不同的结果。我们也不能绝对的说，在任何时候，全锥形 NAT 的可穿透性都比对称 NAT 要好，比如上面的例子，如果只存在连接①和②，显然是对称形 NAT 要更适用。因此，选择哪种 NAT ，除了对网络安全和普遍的可穿透性的考虑外，有时还需要根据具体应用来决定。

原文出处：http://blog.csdn.net/ojhsky/article/details/6011232
Nat的类型——Cone Nat、Symmetic Nat

Nat共分为四种类型：

    1.Full Cone Nat
    2.Restriced Cone Nat
    3.Port Restriced Cone Nat
    4.Symmetric Nat

Symmetric Nat 与 Cone Nat的区别

    1.三种Cone Nat同一主机，同一端口会被映射为相同的公网IP和端口
    2.Symmetric Nat只有来自同一主机，同一端口发送到同一目的主机、端口，映射的公网IP和端口才会一致

一、Full Cone Nat

该nat 将内网中一台主机的IP和端口映射到公网IP和一个指定端口，外网的任何主机都可以通过映射后的IP和端口发送消息

例如：主机A（192.168.0.123：4000）访问主机B，A的IP将会被映射为（222.123.12.23:50000);

当主机A使用4000端口访问主机C时，同样会被映射为（222.123.12.23:50000);而且此时任何主机C 、D·····（包含主机A未访问过的主机）都可以使用（222.123.12.23:50000)访问到主机A（192.168.0.123：4000）。
二、Restriced Cone Nat

该nat 将内网中一台主机的IP和端口映射到公网IP和一个指定端口，只有访问过的IP可以通过映射后的IP和端口连接主机A

例如：主机A（192.168.0.123：4000）访问主机B（223.124.34.23：9000），A的IP将会被映射为（222.123.12.23:50000);

此时只有Ip为（223.124.34.23）才能通过（222.123.12.23:50000)连接主机A。
三、Port Restriced Cone Nat

该nat 将内网中一台主机的IP和端口映射到公网IP和一个指定端口，只有访问过的IP和端口可以通过映射后的IP和端口连接主机A

例如：主机A（192.168.0.123：4000）访问主机B（223.124.34.23：9000），A的IP将会被映射为（222.123.12.23:50000);

此时只有Ip为（223.124.34.23：9000）才能通过（222.123.12.23:50000)连接主机A。
四、Symmetric Nat

当主机A（192.168.0.123：4000）访问主机B（223.124.34.23：9000），A的IP被映射为（222.123.12.23:50000)后，并将这三个IP、端口进行绑定;

等到主机A（192.168.0.123：4000）访问主机C时，可能（注意是可能，也有可能会不变）会被映射为（222.123.12.23:60010)，然后又会将这三个IP、端口绑定;

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

