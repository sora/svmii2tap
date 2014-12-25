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

Install
-------

```bash
$ git clone https://github.com/sora/svmii2tap
$ cd svmii2tap
$ make
$ ./svmii2tap phy0
```

Requirements
------------

* TUN/TAP
* Named PIPE
* Modelsim ASE (DPI-C)
