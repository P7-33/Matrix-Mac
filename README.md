# XMRig
XMRig is high performance Monero (XMR) CPU miner, with the official full Windows support.
Based on cpuminer-multi with heavy optimizations/rewrites and removing a lot of legacy code.

<img src="http://i.imgur.com/GdRDnAu.png" width="596" >

#### Table of contents
* [Features](#features)
* [Download](#download)
* [Usage](#usage)
* [Algorithm variations](#algorithm-variations)
* [Build](#build)
* [Common Issues](#common-issues)
* [Other information](#other-information)
* [Donations](#donations)
* [Contacts](#contacts)

## Features
* High performance (290+ H/s on i7 6700).
* Official Windows support.
* Small Windows executable, only 535 KB without dependencies.
* Support for backup (failover) mining server.
* keepalived support.
* Command line options compatible with cpuminer.
* CryptoNight-Lite support for AEON.
* Smart automatic [CPU configuration](https://github.com/xmrig/xmrig/wiki/Threads).
* Nicehash support
* It's open source software.

## Download
* Binary releases: https://github.com/xmrig/xmrig/releases
* Git tree: https://github.com/xmrig/xmrig.git
  * Clone with `git clone https://github.com/xmrig/xmrig.git`

## Usage
### Basic example
```
xmrig.exe -o xmr-eu.dwarfpool.com:8005 -u YOUR_WALLET -p x -k
```

### Options
```
  -a, --algo=ALGO       cryptonight (default) or cryptonight-lite
  -o, --url=URL         URL of mining server
  -b, --backup-url=URL  URL of backup mining server
  -O, --userpass=U:P    username:password pair for mining server
  -u, --user=USERNAME   username for mining server
  -p, --pass=PASSWORD   password for mining server
  -t, --threads=N       number of miner threads
  -v, --av=N            algorithm variation, 0 auto select
  -k, --keepalive       send keepalived for prevent timeout (need pool support)
  -r, --retries=N       number of times to retry before switch to backup server (default: 5)
  -R, --retry-pause=N   time to pause between retries (default: 5)
      --cpu-affinity    set process affinity to cpu core(s), mask 0x3 for cores 0 and 1
      --no-color        disable colored output
      --donate-level=N  donate level, default 5% (5 minutes in 100 minutes)
  -B, --background      run the miner in the background
  -c, --config=FILE     load a JSON-format configuration file
      --max-cpu-usage=N maximum cpu usage for automatic threads mode (default 75)
      --safe            safe adjust threads and av settings for current cpu
      --nicehash        enable nicehash support
  -h, --help            display this help and exit
  -V, --version         output version information and exit
```

## Algorithm variations
Since version 0.8.0.
* `--av=1` For CPUs with hardware AES.
* `--av=2` Lower power mode (double hash) of `1`.
* `--av=3` Software AES implementation.
* `--av=4` Lower power mode (double hash) of `3`.

## Build
### Ubuntu (Debian-based distros)
```
sudo apt-get install git build-essential cmake libcurl4-openssl-dev
git clone https://github.com/xmrig/xmrig.git
cd xmrig
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### Windows
It's complicated, you need [MSYS2](http://www.msys2.org/), custom libcurl build, and of course CMake too.

Necessary MSYS2 packages:
```
pacman -Sy
pacman -S mingw-w64-x86_64-gcc
pacman -S make
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-pkg-config
```
Configure options for libcurl:
```
./configure --disable-shared --enable-optimize --enable-threaded-resolver --disable-libcurl-option --disable-ares --disable-rt --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-manual --disable-ipv6 --disable-sspi --disable-crypto-auth --disable-ntlm-wb --disable-tls-srp --disable-unix-sockets --without-zlib --without-winssl --without-ssl --without-libssh2 --without-nghttp2 --disable-cookies --without-ca-bundle --without-librtmp
```
CMake options:
```
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCURL_INCLUDE_DIR="c:\xmrig-deps\gcc\x64\include" -DCURL_LIBRARY="c:\xmrig-deps\gcc\x64\lib\libcurl.a"
```

### Optional features
`-DWITH_LIBCPUID=OFF` Disable libcpuid. Auto configuration of CPU after this will be very limited.
`-DWITH_AEON=OFF` Disable CryptoNight-Lite support.

## Common Issues
### HUGE PAGES unavailable
* Run XMRig as Administrator.
* Since version 0.8.0 XMRig automatically enable SeLockMemoryPrivilege for current user, but reboot or sign out still required. [Manual instruction](https://msdn.microsoft.com/en-gb/library/ms190730.aspx).

## Other information
* No HTTP support, only stratum protocol support.
* No TLS support.
* Default donation 5% (5 minutes in 100 minutes) can be reduced to 1% via command line option `--donate-level`.


### CPU mining performance
* **i7-6700** - 290+ H/s (4 threads, cpu affinity 0xAA)
* **Dual E5620** - 377 H/s (12 threads, cpu affinity 0xEEEE)

Please note performance is highly dependent on system load. The numbers above are obtained on an idle system. Tasks heavily using a processor cache, such as video playback, can greatly degrade hashrate. Optimal number of threads depends on the size of the L3 cache of a processor, 1 thread requires 2 MB of cache.

### Maximum performance checklist
* Idle operating system.
* Do not exceed optimal thread count.
* Use modern CPUs with AES-NI instructuon set.
* Try setup optimal cpu affinity.
* Enable fast memory (Large/Huge pages).

## Donations
* XMR: `48edfHu7V9Z84YzzMa6fUueoELZ9ZRXq9VetWzYGzKt52XU5xvqgzYnDK9URnRoJMk1j8nLwEVsaSWJ4fhdUyZijBGUicoD`
* BTC: `1P7ujsXeX7GxQwHNnJsRMgAdNkFZmNVqJT`

## Contacts
* support@xmrig.com
* [reddit](https://www.reddit.com/user/XMRig/)
