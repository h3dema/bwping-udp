bwping-server
=======

About
-------
**bwping-server** - The bwping server side tool for measuring maximum UDP bandwidth

DESCRIPTION
-------
**bwping-server** is part of a client-server tool for measuring bandwidth and response times between two hosts using UDP request/reply mechanism. It requires a client and a server program.

USAGE
-------
**Usage**: bwping-server [-l &lt;length>] [-p &lt;port>] [-6] [-d] [-q] [-h] [-v] [-s wlan-if] 

| Parameter | Description |
---| ---|
-6 | connect using IPv6 - default connect with IPv4 |
-l length | payload byte length of each packet (see RFC 2544 - 64, 128, 256, 512, 1024, 1280 and 1518 bytes)
-p port | server UDP port to listen to, default = 5001 (UDP LISTENING on -p port)
-d | run server as a daemon 
-s wlan-if | collect received signal and noise power in dBm of interface wlan-if
Miscellaneous | |
    -q | quiet
    -h | print this help message and quit
    -v | print version information and quit

SEE ALSO
-------
**[bwping-client](https://github.com/h3dema/bwping-udp/blob/master/docs/bwping-client.md)**

BUGS
-------
No known bugs.
Report bugs to <damacedo@dcc.ufmg.br>

AUTHORS
-------
Daniel Macedo ([damacedo@dcc.ufmg.br](damacedo@dcc.ufmg.br))

Erik de Britto e Silva (erikbritto@gmail.com)

Henrique Moura (henriquemoura@hotmail.com)

