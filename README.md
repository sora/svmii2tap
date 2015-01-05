svMII2tap
---------

svMII2tap is a virtual network device for Systemverilog testbench for FPGA-based network devices such as NIC and Switch/Routers.


```bash
	+------------------------------+  
	|                              |  
	| ping, wireshark, httpd, etc  |  
	|        +------------+        |  
	+--------|   Socket   |--------+  
	         +----|--^----+             
	              |  |
	         +----V--|----+
	+--------|   Socket   |--------+  
	|        +------------+        |  
	|                              |  
	|          TAP device          |
	|                              |  
	|        +------------+        |  
	+--------| Named PIPE |--------+  
	         +----|--^----+          
	              |  |
	        stdin |  | stdout
	              |  |
	         +----V--|----+          
	+--------|   DPI-C    |--------+
	|        +------------+        |
	|                              |
	|    Systemverilog testbench   |
	|                              |
	|        +------------+        |
	|    +---|Ethernet MII|---+    |
	|    |   +------------+   |    |
	|    |        DUT         |    |
	|    +--------------------+    |
	+------------------------------+
```

Features
--------

QuickStart
----------

**Data flow**

```bash
# wall-clock time----->> simulated time---------------------->> wall-clock time-------------->>
Linux -> tapdev(phy0) -> testbench -> dut/hub.v -> testbench -> tapdev(phy0) -> Lnux -> tcpdump
```

**Install**

```bash
$ git clone https://github.com/sora/svmii2tap
$ cd svmii2tap
$ make
$ sudo ./tapdev phy0 &
$ sudo ifconfig phy0 inet 10.0.0.100 netmask 255.255.255.0 up
$ sudo tcpdump -i phy0 -w phy0.pcap &
$ make sim
$ gtkwave wave.vcd
```

**wave.vcd**

![gtkwave.png](https://raw.githubusercontent.com/wiki/sora/svmii2tap/images/gtkwave.png)

**phy0.pcap**

![tcpdump.png](https://raw.githubusercontent.com/wiki/sora/svmii2tap/images/tcpdump.png)

Requirements
------------

* Linux
	- TUN/TAP
	- Named PIPE
* Modelsim ASE (DPI-C)
* GTKWave

Todo
----

* XGMII (10G)
* Multiple ports
* Ethernet FCS check

License
-------

MIT

