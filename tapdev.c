#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/route.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

#define BUF_MAX_ASCII    16000
#define BUF_MAX          9400

#define RXPIPE_NAME      "/tmp/rx0.pipe"
#define TXPIPE_NAME      "/tmp/tx0.pipe"


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
inline int pktout(int rxpipe_fd, const unsigned char *pkt, unsigned length)
{
	int i, olen;
	char obuf[BUF_MAX_ASCII];

	sprintf(obuf, "%02X%02X%02X%02X%02X%02X %02X%02X%02X%02X%02X%02X %02X%02X",
		pkt[0x00], pkt[0x01], pkt[0x02], pkt[0x03], pkt[0x04], pkt[0x05],   // dst mac address
		pkt[0x06], pkt[0x07], pkt[0x08], pkt[0x09], pkt[0x0a], pkt[0x0b],   // src mac address
		pkt[0x0c], pkt[0x0d]);                                              // frame type
	olen = strlen(obuf);

	for (i = 0x0e; i < length; ++i) {
		sprintf(obuf + olen + (i - 0x0e) * 3, " %02X", pkt[i]);
	}
	strcat(obuf, "\n");

	return write(rxpipe_fd, obuf, strlen(obuf));
}


/*
 * pktin
 */
inline int pktin(int tap_fd, unsigned char *buf, int cnt)
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

	return write(tap_fd, tmp_pkt, frame_len);
}


/*
 * main
 */
int main(int argc, char **argv)
{
	char dev[IFNAMSIZ];
	unsigned char buf[BUF_MAX_ASCII*2];
	int tap_fd, fd4, cnt, rxpipe_fd, txpipe_fd, maxfd, ret;
	fd_set fdset;
	struct ifreq ifr;
	struct in6_rtmsg rt;
	struct stat st;

	if (argc < 2) {
		fprintf(stderr, "Usage:%s {devicename}\n", argv[0]);
		return 1;
	}
	strcpy(dev, argv[1]);

	unlink(RXPIPE_NAME);
	unlink(TXPIPE_NAME);
	for (;;) {
		ret = stat(RXPIPE_NAME, &st);
		if (!ret) {
			break;
		}
		sleep(1);
	}

	rxpipe_fd = open(RXPIPE_NAME, O_RDWR);
	if (rxpipe_fd < 0) {
		perror("rxpipe_fd");
		return 1;
	}
	printf("Detected rxpipe file\n");

	txpipe_fd = open(TXPIPE_NAME, O_RDWR);
	if (txpipe_fd < 0) {
		perror("txpipe_fd");
		return 1;
	}
	maxfd = txpipe_fd;

	tap_fd = tap_init(dev);
	if (tap_fd < 0) {
		perror("tap_fd");
		return 1;
	}
	if (tap_fd > maxfd) {
		maxfd = tap_fd;
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
		FD_ZERO(&fdset);
		FD_SET(txpipe_fd, &fdset);
		FD_SET(tap_fd, &fdset);

		ret = select(maxfd + 1, &fdset, NULL, NULL, NULL);
		if (ret < 0) {
			perror("select");
			return 1;
		}

		// pktout
		if (FD_ISSET(tap_fd, &fdset)) {
			ret = read(tap_fd, buf, sizeof(buf));
			if (ret < 0) {
				perror("read");
			}

			ret = pktout(rxpipe_fd, buf, ret);
			if (ret < 0) {
				perror("pktout()");
			}
		}

		// pktin
		if (FD_ISSET(txpipe_fd, &fdset)) {
			cnt = read(txpipe_fd, buf, sizeof(buf));
			if (ret < 0) {
				perror("read");
			}

			ret = pktin(tap_fd, buf, cnt);
			if (ret < 0) {
				perror("pktin()");
			}
		}
	}
	close(tap_fd);
	close(rxpipe_fd);
	close(txpipe_fd);

	return 0;
}

