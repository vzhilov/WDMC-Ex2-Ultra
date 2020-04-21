#include <errno.h>
#include <ctype.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

enum Commands
{
	SET_LED_ON,
	SET_LED_OFF,
	SET_LED_BLINK,
	SET_LED_BREATH,
	SYS_READY,
	SYS_SILENT,
	SYS_SHUTDOWN,
	SYS_REBOOT,
	GET_THERMAL_F,
	GET_THERMAL_C,
	SET_FAN_0,
	SET_FAN_25,
	SET_FAN_50,
	SET_FAN_75,
	SET_FAN_100,

	CMD_TOTAL
};

struct Options
{
	char *name;
	char *help;
	unsigned char command[7];
};

struct Command
{
	char start;		// always 0xFA
	char device;	// ???
	char function;	// 01=??, 03=power, 06=led, 08=thermal sensor
	char value;		// (00 = off, 01=on, 02=status/mode2)
	char data1;		// Get/Set?
	char data2;		// Get/Set?
	char stop;		// always 0xFB
};

struct Options commands[CMD_TOTAL] =
{
	{ "led_set_on",	"Set power led ON",		{ 0xfa, 0x03, 0x06, 0x01, 0x00, 0x00, 0xfb } },
	{ "led_set_off","Set power led OFF",	{ 0xfa, 0x03, 0x06, 0x00, 0x00, 0x00, 0xfb } },
	{ "led_blink",	"Set led to Blinking",	{ 0xfa, 0x03, 0x06, 0x02, 0x00, 0x00, 0xfb } },
	{ "led_breath",	"Set led to Breath",	{ 0xfa, 0x03, 0x06, 0x04, 0x00, 0x00, 0xfb } },
	{ "sys_ready",	"System ready",			{ 0xfa, 0x03, 0x01, 0x00, 0x00, 0x00, 0xfb } },
	{ "sys_silent", "Silent run",			{ 0xfa, 0x03, 0x03, 0x00, 0x00, 0x00, 0xfb } },
	{ "sys_shutdown", "Shutdown",			{ 0xfa, 0x03, 0x03, 0x01, 0x00, 0x00, 0xfb } },
	{ "sys_reboot",	"Reboot (Reset)",		{ 0xfa, 0x03, 0x03, 0x02, 0x00, 0x00, 0xfb } },
	{ "tmp_get_f",	"Get temperature (f)",	{ 0xfa, 0x03, 0x08, 0x00, 0x00, 0x00, 0xfb } },
	{ "tmp_get_c",	"Get temperature (c)",	{ 0xfa, 0x03, 0x08, 0x00, 0x00, 0x00, 0xfb } },
	{ "fan_set_0",	"Set FAN speed 0%",		{ 0xfa, 0x02, 0x00, 0x00, 0x00, 0x00, 0xfb } },
	{ "fan_set_25",	"Set FAN speed 25%",	{ 0xfa, 0x02, 0x00, 0x40, 0x00, 0x00, 0xfb } },
	{ "fan_set_50",	"Set FAN speed 50%",	{ 0xfa, 0x02, 0x00, 0x80, 0x00, 0x00, 0xfb } },
	{ "fan_set_75",	"Set FAN speed 75%",	{ 0xfa, 0x02, 0x00, 0xc0, 0x00, 0x00, 0xfb } },
	{ "fan_set_100","Set FAN speed 100%",	{ 0xfa, 0x02, 0x00, 0xff, 0x00, 0x00, 0xfb } },
};

int set_interface_attribs(int fd, int speed, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // ignore break signal
	tty.c_lflag = 0;                // no signaling chars, no echo,
									// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
									// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void set_blocking (int fd, int should_block)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		printf ("error from tggetattr \n");
		return;
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
		printf ("error setting term attributes \n");
}

void print_help(char *arguments[]) {
	//printf("Too few arguments.\n");
	//printf("Usage: %s [1] [2] [3] [4] [5] (HEX data)\n", argv[0]);
	printf(
		"Usage:\n"
		"\t%s [1] [2] [3] [4] [5] (HEX format)\n"
		"Or:\n"
		"\t%s [command]\n"
		"\n"
		"Available commands:\n",
		arguments[0],
		arguments[0]
	);

	for (int i = 0; i < CMD_TOTAL; i++)
	{
		printf("\t%s\t%s\n", commands[i].name, commands[i].help);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 6) {
		print_help(argv);
		return -1;
	}
	
	unsigned char DataToSend[] = { 0xfa, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb };
	int cmd_found = -1;

	if (argc == 2) {
		for (int i = 0; i < CMD_TOTAL; i++)
		{
			if (strcmp(argv[1], commands[i].name) == 0)
			{
				//printf("Found command: %s\n", commands[i].name);
				cmd_found = i;
				memcpy(DataToSend, commands[i].command, sizeof(DataToSend));
			}
		}
		if (cmd_found < 0)
		{
			printf("Command not found: %s\n", argv[1]);
			return -1;
		}
	}
	else if (argc == 6) {
		DataToSend[1] = strtol(argv[1], NULL, 16);
		DataToSend[2] = strtol(argv[2], NULL, 16);
		DataToSend[3] = strtol(argv[3], NULL, 16);
		DataToSend[4] = strtol(argv[4], NULL, 16);
		DataToSend[5] = strtol(argv[5], NULL, 16);
	}

#ifdef DEBUG
	// Print what we send
	printf(">>: %02x %02x %02x %02x %02x %02x %02x\n",
		DataToSend[0],
		DataToSend[1],
		DataToSend[2],
		DataToSend[3],
		DataToSend[4],
		DataToSend[5],
		DataToSend[6]);
#endif // DEBUG_ON

	// ======== Open port ========
	char *portname = "/dev/ttyS1";
	int fd;
	int dataLenght = 0;

	fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		printf("Error opening %s: %s\n", portname, strerror(errno));
		return -1;
	}

	set_interface_attribs(fd, B19200, 0);
	set_blocking (fd, 0);

	// ======== Send data ========
	int i = 0;
	do
	{
		dataLenght = write(fd, &DataToSend[i], 1);
		i++;
		usleep(100);
		if (dataLenght != 1)
		{
			printf("Error writing byte %i: %i, count: %i\n", (i - 1), DataToSend[i - 1], dataLenght);
			return -1;
		}
	} while (DataToSend[i - 1] != 0xfb);

	// ======== Read data ========

	unsigned char buf[32];
	dataLenght = 0;
	i = 0;
	do {
		dataLenght = read(fd, &buf[i], 1);
		i++;
	} while ((dataLenght == 1) && (buf[i - 1] != 0xfb));

	switch (cmd_found)
	{
	case GET_THERMAL_F:
		printf("Temperature: %d F\n", buf[5]);
		break;
	case GET_THERMAL_C:
		printf("Temperature: %d C\n", (int)(5.0f / 9.0f * (buf[5] - 32.0f)));
		break;
	case SET_FAN_0:
	case SET_FAN_25:
	case SET_FAN_50:
	case SET_FAN_75:
	case SET_FAN_100:
		if (buf[1] == 0x30)
			printf("Temperature set OK\n");
		else
			printf("ERROR: Worng answer: %d\n", buf[1]);
		break;
	default:
		printf("<<:");
		for (int byte = 0; byte < i; byte++)
		{
			printf(" %02x", buf[byte]);
		}
		printf("\n");
		break;
	}

	close(fd);
	return 0;
}