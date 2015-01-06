#include <stdio.h>
#include <stdbool.h>
#include <string.h>
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
extern void gmii_preamble(void);
extern void gmii_ifg(void);

int rxpipe_fd, txpipe_fd;

int pipe_init(void)
{
	int ret = 0;

	unlink(RXPIPE_NAME);
	mkfifo(RXPIPE_NAME, 0666);
	rxpipe_fd = open(RXPIPE_NAME, O_RDWR | O_NONBLOCK);
	if (rxpipe_fd < 0) {
		perror("open: rxpipe_fd");
		ret = -1;
	}

	unlink(TXPIPE_NAME);
	mkfifo(TXPIPE_NAME, 0666);
	txpipe_fd = open(TXPIPE_NAME, O_RDWR | O_NONBLOCK);
	if (txpipe_fd < 0) {
		perror("open: txpipe_fd");
		ret = -1;
	}
	return ret;
}

void pipe_release(void)
{
	close(rxpipe_fd);
	close(txpipe_fd);
	unlink(RXPIPE_NAME);
	unlink(TXPIPE_NAME);
}

int tap2gmii(int *ret)
{
	unsigned char buf[BUF_MAX_ASCII*3];
	unsigned char *p, *cr;
	int cnt, i, frame_len, pos;
	char tmp_pkt[BUF_MAX] = {0};

	cnt = read(rxpipe_fd, buf, sizeof(buf));
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

	/* ethernet FCS */
	frame_len += 4;    // zero padding :todo

	/* a packet data to GMII port */
	gmii_preamble();
	for (i = 0; i < frame_len; i++) {
		gmii_write(tmp_pkt[i]);
	}
	gmii_ifg();

	*ret = 1;

out:
	return 0;
}

static unsigned char rx_tmp_pkt[BUF_MAX] = {0};
static inline int gmii2pipe(unsigned int frame_len)
{
	int i, olen;
	char obuf[BUF_MAX_ASCII];

	sprintf(obuf, "%02X%02X%02X%02X%02X%02X %02X%02X%02X%02X%02X%02X %02X%02X",
		rx_tmp_pkt[0x00], rx_tmp_pkt[0x01], rx_tmp_pkt[0x02],   // dst mac address
		rx_tmp_pkt[0x03], rx_tmp_pkt[0x04], rx_tmp_pkt[0x05],
		rx_tmp_pkt[0x06], rx_tmp_pkt[0x07], rx_tmp_pkt[0x08],   // src mac address
		rx_tmp_pkt[0x09], rx_tmp_pkt[0x0a], rx_tmp_pkt[0x0b],
		rx_tmp_pkt[0x0c], rx_tmp_pkt[0x0d]);                    // frame type
	olen = strlen(obuf);

	for (i = 0x0e; i < frame_len; ++i) {
		sprintf(obuf + olen + (i - 0x0e) * 3, " %02X", rx_tmp_pkt[i]);
	}
	strcat(obuf, "\n");

	//printf("Data: %s\n", obuf);

	return write(txpipe_fd, obuf, strlen(obuf));
}

static unsigned int rx_frame_len = 0;
static enum RxState { IDLE, DATA } rx_state;
int gmii_read(const int gmii_en, const unsigned char gmii_dout)
{
	int ret;

	if (gmii_en == 0 && rx_state == IDLE) {
		goto out;
	}

	switch (rx_state) {
		case IDLE:
			if (gmii_dout == 0xD5) {    // SFD
				rx_state = DATA;
			}
			break;
		case DATA:
			if (!gmii_en) {
				// emit a packet
				ret = gmii2pipe(rx_frame_len);
				if (ret < 0) {
					perror("gmii2pipe");
				}

				// termination
				rx_state = IDLE;
				rx_frame_len = 0;
			} else {
				rx_tmp_pkt[rx_frame_len++] = gmii_dout;
			}
			break;
	}

out:
	return 0;
}

