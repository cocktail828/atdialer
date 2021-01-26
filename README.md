<!--
 * @Author: sinpo828
 * @Date: 2020-12-31 17:28:12
 * @LastEditors: sinpo828
 * @LastEditTime: 2021-01-26 18:50:06
 * @Description: file content
-->
# atdialer
a very simple tool

runtime log
```bash➜  atdialer git:(main) ✗ make
rm -rf *.o atdialer
g++ -Wall -g   -c -o at-utils.o at-utils.cpp
g++ -Wall -g   -c -o devices.o devices.cpp
g++ -Wall -g   -c -o observerIMPL.o observerIMPL.cpp
g++ -Wall -g   -c -o subjectIMPL.o subjectIMPL.cpp
g++ -o atdialer  at-utils.o  devices.o  observerIMPL.o  subjectIMPL.o -lpthread
➜  atdialer git:(main) ✗ ./atdialer -P 1234                             
current version: 0.0.1
pincode: 1234
Device ID 5e3:610, Port 5, Path /sys/bus/usb/devices/1-5
  |__ If 0, Class=9, SubClass=0, Proto=2, NODE=
Device ID 403:6001, Port 5.4, Path /sys/bus/usb/devices/1-5.4
  |__ If 0, Class=ff, SubClass=ff, Proto=ff, NODE=ttyUSB0
Device ID 5e3:612, Port 4, Path /sys/bus/usb/devices/2-4
  |__ If 0, Class=9, SubClass=0, Proto=0, NODE=
Device ID 413c:301a, Port 8, Path /sys/bus/usb/devices/1-8
  |__ If 0, Class=3, SubClass=1, Proto=2, NODE=
Device ID 2c7c:8101, Port 4.1, Path /sys/bus/usb/devices/2-4.1
  |__ If 5, Class=ff, SubClass=2, Proto=3, NODE=ttyUSB4
  |__ If 3, Class=ff, SubClass=2, Proto=14, NODE=ttyUSB2
  |__ If 1, Class=a, SubClass=0, Proto=1, NODE=
  |__ If 8, Class=ff, SubClass=2, Proto=1, NODE=ttyUSB7
  |__ If 6, Class=ff, SubClass=2, Proto=a, NODE=ttyUSB5
  |__ If 4, Class=ff, SubClass=2, Proto=13, NODE=ttyUSB3
  |__ If 2, Class=ff, SubClass=2, Proto=12, NODE=ttyUSB1
  |__ If 0, Class=2, SubClass=d, Proto=0, NODE=enp0s20f0u4u1
  |__ If 7, Class=ff, SubClass=2, Proto=6, NODE=ttyUSB6
Device ID 413c:2113, Port 6, Path /sys/bus/usb/devices/1-6
  |__ If 0, Class=3, SubClass=1, Proto=1, NODE=
  |__ If 1, Class=3, SubClass=0, Proto=0, NODE=
will use AT port: /dev/ttyUSB1
AT mode: Hisilicon
polling thread start polling, thread id 7fc858cac640
polling is ready
SEND>> AT+CPIN?
RECV<< AT+CPIN?
RECV<< +CPIN: READY
RECV<< OK
SEND>> AT+CEREG?
RECV<< AT+CEREG?
RECV<< +CEREG: 0,1
RECV<< OK
SEND>> AT+QICSGP=1,1,"","","",0
RECV<< AT+QICSGP=1,1,"","","",0
RECV<< OK
SEND>> AT+QNETDEVSTATUS=1
RECV<< AT+QNETDEVSTATUS=1
RECV<< +QNETDEVSTATUS: 3512EF0A,FCFFFFFF,3612EF0A,3612EF0A,024E68DA,028FCEDC, 85600, 85600
RECV<< OK
trigger DHCP operation
udhcpc pidfile(/tmp/udhcpc.pid) is invalid, try to start an client
fork success, child pid is 35214
udhcpc: started, v1.32.1
No resolv.conf for interface enp0s20f0u4u1.udhcpc
udhcpc: sending discover
udhcpc: sending select for 10.239.18.53
udhcpc: lease of 10.239.18.53 obtained, lease time 518400
/usr/share/udhcpc/default.script: Resetting default routes
SIOCDELRT: No such process
/usr/share/udhcpc/default.script: Adding DNS 218.104.78.2
/usr/share/udhcpc/default.script: Adding DNS 220.206.143.2

Time reached 10 seconds
SEND>> AT+QNETDEVSTATUS=1
RECV<< AT+QNETDEVSTATUS=1
RECV<< +QNETDEVSTATUS: 3512EF0A,FCFFFFFF,3612EF0A,3612EF0A,024E68DA,028FCEDC, 85600, 85600
RECV<< OK
trigger DHCP operation
udhcpc pid(217620) do renew operation
udhcpc renew operation: success
udhcpc: performing DHCP renew
udhcpc: sending renew to 10.239.18.54
Time reached 10 seconds
...
```
