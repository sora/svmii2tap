#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>
#include <unistd.h>  
#include <string.h>  
#include <sys/ioctl.h>  
#include <sys/socket.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
  
#include <netinet/in.h>  
#include <netinet/ip6.h>  
#include <arpa/inet.h>
#include <netdb.h>
#include <net/route.h>
  
#include <linux/if.h>  
#include <linux/if_tun.h>  
#include <linux/if_ether.h>

#define BUF_MAX_ASCII    160000
#define BUF_MAX          9400

/*
 * tap_init
 */
int tap_init(char *dev)  
{  
	struct ifreq ifr;  
	int fd, err;  
  
	fd = open("/dev/net/tun", O_RDWR);
	if (fd < 0) {
		perror("open");  
		return -1;  
	}  
  
	memset(&ifr, 0, sizeof(ifr));  
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;  
	if (*dev) { 
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);  
	}
	  
	err = ioctl(fd, TUNSETIFF, (void *)&ifr);
	if (err < 0) {
		perror("TUNSETIFF");  
		close(fd);  
		return err;  
	}  
	strcpy(dev, ifr.ifr_name);  

	return fd;  
}  

  
/*
 * pktout
 */
inline int pktout(const unsigned char *pkt, unsigned length)  
{  
	int i, olen;  
	unsigned char obuf[BUF_MAX_ASCII];
	  
	sprintf(obuf, "%02X%02X%02X%02X%02X%02X %02X%02X%02X%02X%02X%02X %02X%02X",
		pkt[0x00], pkt[0x01], pkt[0x02], pkt[0x03], pkt[0x04], pkt[0x05],   // dst mac address
		pkt[0x06], pkt[0x07], pkt[0x08], pkt[0x09], pkt[0x0a], pkt[0x0b],   // src mac address
		pkt[0x0c], pkt[0x0d]);                                              // frame type
	olen = strlen(obuf);

	for (i = 0x0e; i < length; ++i) {
		sprintf(obuf + olen + (i - 0x0e) * 3, " %02X", pkt[i]);
	}
	strcat(obuf, "\n");

	return write(1, obuf, strlen(obuf));
}  


/*
 * pktin
 */
inline int pktin(int fd, int cnt, unsigned char *buf)
{
	unsigned char *p, *cr;
	int i, frame_len, pos;
	unsigned char tmp_pkt[BUF_MAX];

	fprintf(stderr, "cnt=%d\n", cnt);

	for (i = 0, cr = buf; *cr != '\n' && i < cnt; ++cr, ++i);

	frame_len = 0;
	pos = 0;
	for (p = buf; p < cr && frame_len < BUF_MAX; ++p) {
		// skip space
		if (*p == ' ') {
			   continue;
		}

		// conver to upper char
		if (*p >= 'a' && *p <= 'z') {
			   *p -= 0x20;
		}

		// is hexdigit?
		if (*p >= '0' && *p <= '9') {
			if (pos == 0) {
				tmp_pkt[frame_len] = (*p - '0') << 4;
				pos = 1;
			} else {
				tmp_pkt[frame_len] |= (*p - '0');
				++frame_len;
				pos = 0;
			}
		} else if (*p >= 'A' && *p <= 'Z') {
			if (pos == 0) {
				tmp_pkt[frame_len] = (*p - 'A' + 10) << 4;
				pos = 1;
			} else {
				tmp_pkt[frame_len] |= (*p - 'A' + 10);
				++frame_len;
				pos = 0;
			}
		}
	}

	fprintf(stderr, "frame_len=%d\n", frame_len);
	if (frame_len == 0)
		exit(1);

	return write(fd, tmp_pkt, frame_len);  
}
  

/*
 * main
 */
int main(int argc, char **argv)  
{  
	char dev[IFNAMSIZ];  
	unsigned char buf[BUF_MAX_ASCII*2];
	int fd, fd4, cnt;
	fd_set fdset;  
	struct ifreq ifr;  
	struct in6_rtmsg rt;  
  
	if (argc < 2) {  
		fprintf(stderr, "Usage:%s {devicename}\n", argv[0]);  
		return 1;  
	}  
	strcpy(dev, argv[1]);  

	fd = tap_init(dev);  
	if (fd < 0) {  
		return 1;  
	}  

	/* ifup */  
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);  
	fd4 = socket(PF_INET, SOCK_DGRAM, 0);  
	if (fd4 < 0) {  
		return 1;  
	}  

	if (ioctl(fd4, SIOCGIFFLAGS, &ifr) != 0) {  
		perror("SIOCGIFFLAGS");  
		return 1;  
	}  

	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;  
	if (ioctl(fd4, SIOCSIFFLAGS, &ifr) != 0) {  
		perror("SIOCSIFFLAGS");  
		return 1;  
	}  
	  
	memset(&rt, 0, sizeof(rt));  

	/* get ifindex */  
	memset(&ifr, 0, sizeof(ifr));  
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);  
	if (ioctl(fd4, SIOGIFINDEX, &ifr) <0) {  
		perror("SIOGIFINDEX");  
		return 1;  
	}  
		  
	while(1) {  
		int ret;  
		unsigned char hdr_offset;

		FD_ZERO(&fdset);  
		FD_SET(STDIN_FILENO, &fdset);  
		FD_SET(fd, &fdset);  

		ret = select(fd + 1, &fdset, NULL, NULL, NULL);  
		if (ret < 0) {  
			perror("select");  
			return 1;  
		}  

		// pktout
		if (FD_ISSET(fd, &fdset)) {
			ret = read(fd, buf, sizeof(buf));  
			if (ret < 0) {
				perror("read");  
			}

			ret = pktout(buf, ret);  
			if (ret < 0) {
				perror("pktout()");
			}
		}  

		// pktin
		if (FD_ISSET(STDIN_FILENO, &fdset)) {  
			cnt = read(STDIN_FILENO, buf, sizeof(buf));  
			if (ret < 0) {
				perror("read");  
			}

			ret = pktin(fd, cnt, buf);
			if (ret < 0) {
				perror("pktin()");
			}
		}  
	}  
	close(fd); 

	return 0;  
}

