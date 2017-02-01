/*
 * bwping-udp, Copyright (c) 2014-2017, UFMG/DCC/WINET
 * All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact damacedo@dcc.ufmg.br.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 4. Neither the name of The UFMG/DCC/WINET nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UFMG/DCC/WINET AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

 /*
	BWPing is a tool to measure bandwidth and response times
	between two hosts using UDP request/reply mechanism.
	It requires a client and a server program.

    This is the server side.

  First developement by professor Daniel Macedo.
  Adapted by Henrique Moura and Erik de Britto e Silva
 */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h> // strncpy
#include <stdlib.h>
#include <unistd.h> // getopt, daemon
#include <stdbool.h>

#include "ping.h"
#include "cpu_info.h"
#include "wireless.h"


void version(char ** argv) {
  printf("%s version %s\n", argv[0], BWPING_VERSION);
}

void usage(char ** argv) {
  version(argv);
  printf("Usage: %s [-l <length>] [-p<port>] [-6] [-d] [-q] [-h] [-v] [-s wlan-if]\n", argv[0]);
  printf("\t-l length  : payload byte length of each packet\n");
  printf("\t-p port    : server UDP port to listen to, default = 5001\n");
  printf("\t-d         : run the server as a daemon\n");
  printf("\t-6         : connect using IPv6\n");
  printf("\t-s wlan-if : print received signal and noise power in dBm of interface wlan-if\n");
  printf("\nMiscellaneous:\n");
  printf("\t-q: quiet\n");
  printf("\t-h: print this help message and quit\n");
  printf("\t-v: print version information and quit\n");
}

void run(bool daemon_mode, int port, int length, bool quiet, bool ipv6, bool rssi, char * wlan_if) {
   int sockfd;
   struct sockaddr_storage servaddr, cliaddr;
   socklen_t addr_size;

   char recvBuffer[length];

   int domain;
   if (ipv6) {
     domain = AF_INET6;
   } else {
     domain = AF_INET;
   }
   if ((sockfd=socket(domain, SOCK_DGRAM, 0)) >= 0) {
     // valid socket
     if (!quiet) {
         printf("Starting server %s @ %d - payload size: %d\n", (ipv6) ? "using IPv6 " : "using IPv4", port, length);
         if (daemon_mode) printf("Entering daemon mode\n");
     }

     bzero(&servaddr, sizeof(servaddr));
     if (ipv6) {
       struct sockaddr_in6 * addr = (struct sockaddr_in6 *)&servaddr;
       addr->sin6_family  = AF_INET6;
       addr->sin6_addr    = in6addr_any;
       addr->sin6_port    = htons(port);
       addr_size = sizeof(*addr);
     } else {
       struct sockaddr_in * addr = (struct sockaddr_in *)&servaddr;
       addr->sin_family     = AF_INET;
       addr->sin_addr.s_addr= htonl(INADDR_ANY);
       addr->sin_port       = htons(port);
       addr_size = sizeof(*addr);
     }
     bind(sockfd, (struct sockaddr *)&servaddr, addr_size);
     for (;;) { // loop forever
        recvfrom(sockfd, recvBuffer, sizeof(char)*length, 0,
                  (struct sockaddr *)&cliaddr, &addr_size);
        header * recv = (header *)&recvBuffer;
        // CPU info
        cpu_info info;
        get_cpu_info(&info);
        recv->idle_time_perc = info.idle_time_perc;
        recv->non_idle_time_perc = info.non_idle_time_perc;

        // RSSI info
        if (rssi) {
          strncpy((char *)&recv->intfname, wlan_if, MAX_INTF_NAME);
          get_rssi(&recv->link, &recv->level, &recv->noise);
        }

        if (!quiet) {
          char str[INET6_ADDRSTRLEN];
          if (ipv6) {
            struct sockaddr_in6 * addr = (struct sockaddr_in6 *)&cliaddr;
            inet_ntop(AF_INET6, &addr->sin6_addr, str, INET6_ADDRSTRLEN);
          } else {
            struct sockaddr_in * addr = (struct sockaddr_in *)&cliaddr;
            inet_ntop(AF_INET, &addr->sin_addr, str, INET_ADDRSTRLEN);
          }
          printf("[%s] Received packet number %d\n", str, recv->seqnum);
        }
        sendto(sockfd, recv, sizeof(header), 0, (struct sockaddr *)&cliaddr, addr_size);
     }
   } else {
     printf("Error creating socket.\n");
     exit(1);
   }
}

int main(int argc, char**argv) {

   int port = 5001;
   int length = DEFAULT_MINIMUM_PAYLOAD_SIZE;
   int c = 0;
   bool daemon_mode = false;
   bool quiet = false;
   bool ipv6 = false;
   bool rssi = false;

   char wlan_if[MAX_INTF_NAME];
   wlan_if[0] = '\0';

   while ((c = getopt(argc, argv, "p:l:s:6qdhv")) != -1) {
       switch(c){
          case 'p':
            port = atoi(optarg);
            break;
          case 'l':
            length = atoi(optarg);
            if (length < DEFAULT_MINIMUM_PAYLOAD_SIZE) length = DEFAULT_MINIMUM_PAYLOAD_SIZE;
            break;
          case 'v':
            version(argv);
            exit(1);
          case 'h':
            usage(argv);
            exit(1);
          case 'd':
            daemon_mode = true;
            break;
          case '6':
            ipv6 = true;
            break;
          case 's':
            rssi = true;
            int err;
            strncpy((char *)&wlan_if, optarg, MAX_INTF_NAME);
            if ((err = open_wireless_info((char *)&wlan_if)) < 0) {
              switch (err) {
                case -1:
                  fprintf(stderr,"Info about wireless interface not found! Please check right wlan interface name.\n");
                  break;
                case -2:
                case -3:
                  fprintf(stderr,"Error reading wireless interface data!\n");
                  break;
              }
              exit(1); // error
            }
            break;
          case 'q':
            quiet = true;
            break;
          default:
            usage(argv);
            exit(1);
       }
   }
   if (length < sizeof(header)) {
    length = sizeof(header);
   }

   if ((!daemon_mode) || (daemon(1, 1)==0))  {
     // daemon: don't change directory, don't redirect output
     run(daemon_mode, port, length, quiet, ipv6, rssi, (char *)&wlan_if);
   }
}
