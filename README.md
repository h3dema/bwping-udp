# bwping-udp

##About

**bwping-udp** is a tool for wireless performance measurement. It was
developed in the WINET laboratory (http://www.winet.dcc.ufmg.br) in
UFMG in order to collect metrics relevant to wireless links that are
not collected by classic tools (i.e. Iperf and others).


##For the impatient

Download the source files using **git clone** or download the zip file from github.

```sh
$ git clone https://github.com/h3dema/bwping-udp
```

Go to the directory of the uncompressed files (*cd ./bwping-udp*).
To compile and install bwping, simply run:

```sh
% make
% make install
```

To get help, run:

```sh
% man bwping-client
% man bwping-server
```

## See More Information
**[bwping-client](https://github.com/h3dema/bwping-udp/blob/master/docs/bwping-client.md)**

**[bwping-server](https://github.com/h3dema/bwping-udp/blob/master/docs/bwping-server.md)**


## BUGS
No known bugs.
Report bugs to <damacedo@dcc.ufmg.br>

## Features Wish List

  * Parallel streams
  * Reverse test mode – Server sends, client receives
  * Set target bandwidth to n bits/sec
  * output in JSON format
  * set CPU affinity
  * format to print bandwidth numbers in (Kbits/sec, Kbytes/sec, Mbits/sec, MBytes/sec)


##AUTHORS
Daniel Macedo ([damacedo@dcc.ufmg.br](damacedo@dcc.ufmg.br))

Erik de Britto e Silva (erikbritto@gmail.com)

Henrique Moura (henriquemoura@hotmail.com)


##Licensing and Liability


THIS SOFTWARE IS PROVIDED BY THE [UFMG/DCC/WINET](http://www.winet.dcc.ufmg.br/) AND CONTRIBUTORS ''AS
IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
