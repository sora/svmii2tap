#include <stdio.h>  
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>


#define BUF_MAX_ASCII    16000
#define BUF_MAX          9400

#define TXPIPE_NAME      "/tmp/tx0.pipe"
#define RXPIPE_NAME      "/tmp/rx0.pipe"


extern void gmii_write(char);
extern void nop(void);

int pipe_fd;

int pipe_init(void)
{
	int ret = 0;

	unlink(RXPIPE_NAME);
	mkfifo(RXPIPE_NAME, 0666);

	pipe_fd = open(RXPIPE_NAME, O_RDONLY | O_NONBLOCK);
	if (pipe_fd < 0) {
		perror("open");
		ret = -1;
	}

	return ret;
}

void pipe_release(void)
{
	close(pipe_fd);
	unlink(RXPIPE_NAME);
}

int tap2gmii(int *ret)
{
	unsigned char buf[BUF_MAX_ASCII*3];
	unsigned char *p, *cr;
	int cnt, i, frame_len, pos;
	char tmp_pkt[BUF_MAX];

	cnt = read(pipe_fd, buf, sizeof(buf));
	if (cnt < 0 && errno != EAGAIN) {
		perror("read");
		*ret = -1;
		goto out;
	} else if (cnt < 1) {
		*ret = 0;
		goto out;
	}

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

	gmii_write(0);  // gmii_en
	for (i = 0; i < frame_len; i++) {
		gmii_write(tmp_pkt[i]);
	}
	nop();

	*ret = 1;

out:
	return 0;
}

#if 0
/*
 * pktout
 */
int gmii2tap(const unsigned char *pkt, unsigned length)
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

	return write(pipe_fd, obuf, strlen(obuf));
}  


/*
 * pktin
 */
int tap2gmii(unsigned char *buf, int cnt)
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
#endif

