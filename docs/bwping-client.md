bwping-client
=========

About
-------
**bwping-client** - The bwping client side tool for measuring maximum UDP bandwidth

DESCRIPTION
-------
**bwping-client** is part of a client-server tool for measuring bandwidth and response times between two hosts using UDP request/reply mechanism. It requires a client and a server program.

USAGE
-------
**Usage**: bwping-client -a &lt;IP address\> [-p &lt;port>] [-l &lt;length>] [-t &lt;interval_us>] [-d &lt;duration>] [-c] [-h] [-v] [-s wlan-if] [-r]

| Parameter | Description |
---| ---|
-6 | connect using IPv6 - default connect with IPv4 |
-a ip   | server IPv4 address (required) |
-c | output in CSV (Comma Separated Values ) format  |
-d duration | total execution time in seconds - Number of collected samples = duration/interval |
-i interval | time between periodic bandwidth reports in seconds - default 1 second - minimum 0.01 seconds (=10ms). This value should be entered as a float in seconds, e.g enter 0.5 to get a report each 0.5 seconds (500ms) |
-l length  | payload byte length of each packet  (see RFC 2544 - 64, 128, 256, 512, 1024, 1280 and 1518 bytes) |
-o outfile | record output to file name outfile - overwrite |
-p port | server UDP port in use , default = 5001 |
-s wlan-if | print received signal and noise power in dBm of interface wlan-if
-r | print CPU idle and non-idle times
-t interval | time between **bwping-udp** requests in microseconds - default 10 miliseconds. This value should be entered as an integer in microseconds, e.g to make a request each 20 ms, you should enter 20000. |
Miscellaneous |
-h | print this help message and quit |
-v | print version information and quit |

**Notes:**

*bwping-udp* prints some information during its execution. If you use "-c" parameter, bwping-udp uses a "Comma Separated Values" (CSV) format. It prints the following fields in each line, one line per sample:

  YYYYMMDDHHMMSS, tv_sec.tv_usec, bw, delay, delay_s, jitter, jitter_s, loss, packets_sent, packets_recv, total_sent, total_rcvd, total_diff
  [, cpuidle_t_local, cpu_nonidle_t_local, cpuidle_t_remote, cpu_nonidle_t_remote]
  [, link_quality_local, signal_local, noise_local, link_quality_remote, signal_remote, noise_remote]

  * *delay* and *jitter* are shown in seconds, with a resolution of microseconds.
  
  * *delay_s* and *jitter_s* are the standard deviation of the delay and jitter, respectiveely. They area also shown in seconds, with a resolution of microseconds.

  * *loss* is shown in percentage.

  * *bw* (bandwidth) is shown in bytes per second (Bps). Bwping-udp uses a long long int variable to compute the bandwidh, so it is capable of containing at least the [−9,223,372,036,854,775,807 , +9,223,372,036,854,775,807] range.

  * *packets_sent* and *packets_recv* are the number of packets sent and received in the sample period

  * *total_sent* and *total_rcvd* are the total number of packets sent and received since the begin of the test

  * *total_diff* = *total_sent* - *total_rcvd*

  * link_quality_local and link_quality_remote are values between 0 and 100, provided by the driver provider

  * signal_local, noise_local, signal_remote, noise_remote are in dBm

  * **bwping-client** with no -d duration runs forever!

SEE ALSO
-------
**[bwping-server](https://github.com/h3dema/bwping-udp/blob/master/docs/bwping-server.md)**

BUGS
-------
No known bugs.
Report bugs to <damacedo@dcc.ufmg.br>

AUTHORS
-------
Daniel Macedo ([damacedo@dcc.ufmg.br](damacedo@dcc.ufmg.br))

Erik de Britto e Silva (erikbritto@gmail.com)

Henrique Moura (henriquemoura@hotmail.com)
