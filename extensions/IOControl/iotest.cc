/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include <chrono>
#include <thread>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <comedilib.h>
#include <getopt.h>

int subdev = 0;
int range = 0;
int aref = AREF_GROUND;
int adelay = 10 * 1000; // 10 мкс
int chan[50];
int blink_msec = 300;

static struct option longopts[] =
{
	{ "help", no_argument, 0, 'h' },
	{ "read", required_argument, 0, 'r' },
	{ "write", required_argument, 0, 'w' },
	{ "aread", required_argument, 0, 'i' },
	{ "awrite", required_argument, 0, 'o' },
	{ "subdev", required_argument, 0, 's' },
	{ "device", required_argument, 0, 'd' },
	{ "verbose", no_argument, 0, 'v' },
	{ "aref", required_argument, 0, 'z' },
	{ "adelay", required_argument, 0, 'y' },
	{ "range", required_argument, 0, 'x' },
	{ "config", required_argument, 0, 'c' },
	{ "autoconf", no_argument, 0, 'a' },
	{ "plus", required_argument, 0, 'p' },
	{ "blink", no_argument, 0, 'b' },
	{ "blink-msec", required_argument, 0, 'm' },
	{ "subdev-config", required_argument, 0, 'q' },
	{ NULL, 0, 0, 0 }
};
// --------------------------------------------------------------------------

enum Command
{
	cmdNOP,
	cmdDRead,
	cmdDWrite,
	cmdARead,
	cmdAWrite,
	cmdConfig,
	cmdSubConfig

} cmd;
// --------------------------------------------------------------------------
static void insn_config( comedi_t* card, int subdev, int channel, lsampl_t iotype, int range, int aref );
static void insn_subdev_config( comedi_t* card, int subdev, lsampl_t type );
// --------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	comedi_t* card;
	lsampl_t data = 0;
	int optindex = 0;
	int opt = 0;
	const char* dev = "/dev/comedi0";
	int val = -1;
	cmd = cmdNOP;
	int verb = 0;
	int autoconf = 0;
	int cnum = 0;
	int blink = 0;
	int exret = EXIT_SUCCESS;

	memset(chan, -1, sizeof(chan));

	while(1)
	{
		opt = getopt_long(argc, argv, "habvr:w:i:o:s:d:c:p:m:q:x:z:", longopts, &optindex);

		if( opt == -1 )
			break;

		switch (opt)
		{
			case 'h':
				printf("-h|--help         - this message\n");
				printf("[-w|--write] chan val - write to digital channel\n");
				printf("[-r|--read] chan     - read from digital channel\n");
				printf("[-o|--awrite] chan val - write to analog channel\n");
				printf("[-i|--aread] chan     - read from analog channel\n");
				printf("[-s|--subdev] sub    - use subdev number sub. (Default: 0)\n");
				printf("[-d|--device] dev    - use device dev. (Default: /dev/comedi0)\n");
				printf("[--aref] val        - AREF (Default: %d)\n", aref);
				printf("[--range] val        - RANGE (Default: %d)\n", range);
				printf("[-p|--plus] chan    - Add channel for command.\n");
				printf("[-a|--autoconf]     - autoconf channel type\n");
				printf("[-c|--config] chan type    - Set 'type' for 'chan'.\n");
				printf("    %3d  - DI\n", INSN_CONFIG_DIO_INPUT );
				printf("    %3d  - DO\n", INSN_CONFIG_DIO_OUTPUT );
				printf("    %3d  - AI\n", 100 );
				printf("    %3d  - AO\n", 101 );
				printf("[-q|--subdev-config]     - subdev type configure\n");
				printf("    1     - TBI 24/0\n");
				printf("    2     - TBI 0/24\n");
				printf("    3     - TBI 16/8\n");
				printf("aref:\n");
				printf("    0    - analog ref = analog ground\n");
				printf("    1    - analog ref = analog common\n");
				printf("    2    - analog ref = differential\n");
				printf("    3    - analog ref = other (undefined)\n");
				printf("range:\n");
				printf("    0    -  -10В - 10В\n");
				printf("    1    -  -5В - 5В\n");
				printf("    2    -  -2.5В - 2.5В\n");
				printf("    3    -  -1.25В - 1.25В\n");
				printf("[--blink]            - (blink output): ONLY FOR 'write': 0-1-0-1-0-...\n");
				printf("[--blink-msec] val    - Blink pause [msec]. Default: 300 msec\n");
				return 0;

			case 'r':
				chan[0] = atoi(optarg);
				cmd = cmdDRead;
				break;

			case 'w':
				chan[0] = atoi(optarg);
				cmd = cmdDWrite;

				if( optind < argc && (argv[optind])[0] != '-' )
					val = atoi(argv[optind]);

				break;

			case 'i':
				chan[0] = atoi(optarg);
				cmd = cmdARead;
				break;

			case 'o':
				chan[0] = atoi(optarg);
				cmd = cmdAWrite;

				if( optind < argc && (argv[optind])[0] != '-' )
					val = atoi(argv[optind]);

				break;

			case 'd':
				dev = optarg;
				break;

			case 's':
				subdev = atoi(optarg);
				break;

			case 'v':
				verb = 1;
				break;

			case 'x':
				range = atoi(optarg);
				break;

			case 'y':
				adelay = atoi(optarg);
				break;

			case 'z':
				aref = atoi(optarg);
				break;

			case 'c':
			{
				cmd = cmdConfig;
				chan[cnum++] = atoi(optarg);

				if( optind < argc && (argv[optind])[0] != '-' )
					val = atoi(argv[optind]);
				else
				{
					fprintf(stderr, "(config error: Unkown channel type. Use '-c chan type'\n");
					exit(EXIT_FAILURE);
				}
			}
			break;

			case 'q':
				cmd = cmdSubConfig;
				val = atoi(optarg);
				break;


			case 'a':
				autoconf = 1;
				break;

			case 'p':
				chan[++cnum] = atoi(optarg);
				break;

			case 'm':
				blink_msec = atoi(optarg);
				break;

			case 'b':
				blink = 1;
				break;

			case '?':
			default:
				printf("? argumnet\n");
				return 0;
		}
	}

	card = comedi_open(dev);

	if( card == NULL )
	{
		comedi_perror("comedi_open error");
		exit(EXIT_FAILURE);
	}

	if( verb )
	{
		fprintf(stdout, "init comedi:\n");
		fprintf(stdout, "dev: %s\n", dev);
		fprintf(stdout, "subdev: %d\n", subdev);
		fprintf(stdout, "channel: %d\n", chan[0]);
	}

	switch( cmd )
	{
		case cmdDRead:
		{
			if( autoconf )
			{
				for( unsigned int k = 0; chan[k] != -1; k++ )
				{
					if( comedi_dio_config(card, subdev, chan[k], INSN_CONFIG_DIO_INPUT) < 0)
					{
						comedi_perror("can't configure DI channels");
						exret = EXIT_FAILURE;
					}
				}
			}

			for( unsigned int k = 0; chan[k] != -1; k++ )
			{
				if( comedi_dio_read(card, subdev, chan[k], &data) < 0)
				{
					fprintf(stderr, "can't read from channel %d (err(%d): %s)\n", chan[k], errno, strerror(errno));
					exret = EXIT_FAILURE;
				}

				printf("Digital input %d: %d\n", chan[k], data);
			}
		}
		break;

		case cmdDWrite:
		{
			if( autoconf )
			{
				for( unsigned int k = 0; chan[k] != -1; k++ )
				{
					if( comedi_dio_config(card, subdev, chan[k], INSN_CONFIG_DIO_OUTPUT) < 0 )
					{
						fprintf(stderr, "can't configure DO channels. (%d) %s", errno, strerror(errno));
						exret = EXIT_FAILURE;
					}
				}
			}

			// реализация мигания
			while(1)
			{
				for( unsigned int k = 0; chan[k] != -1; k++ )
				{
					if( verb )
						printf( "write: ch=%d val=%d\n", chan[k], val);

					if( comedi_dio_write(card, subdev, chan[k], val) < 0)
					{
						fprintf(stderr, "can't write 1 to channel %u. (%d) %s\n", k, errno, strerror(errno));
						exret = EXIT_FAILURE;
					}
				}

				if( !blink )
					break;

				val = val ? 0 : 1;
				std::this_thread::sleep_for(std::chrono::milliseconds(blink_msec));
			}
		}
		break;

		case cmdARead:
		{
			for( unsigned int k = 0; chan[k] != -1; k++ )
			{
				if( autoconf )
					insn_config(card, subdev, chan[k], 100, range, aref);

				int ret = comedi_data_read_delayed(card, subdev, chan[k], range, aref, &data, adelay);

				if( ret < 0)
				{
					fprintf(stderr, "can't read from channel %d: (%d) %s\n", chan[k], errno, strerror(errno));
					exret = EXIT_FAILURE;
				}

				printf("Readed from channel %d value is %d\n", chan[k], data);
			}
		}
		break;

		case cmdAWrite:
		{
			for( unsigned int k = 0; chan[k] != -1; k++ )
			{
				if( autoconf )
					insn_config(card, subdev, chan[k], 101, range, aref);

				int ret = comedi_data_write(card, subdev, chan[k], range, aref, val);

				if( ret < 0)
				{
					fprintf(stderr, "can't write to channel %d: (%d) %s\n", chan[k], errno, strerror(errno));
					exret = EXIT_FAILURE;
				}
			}
		}
		break;

		case cmdConfig:
		{
			for( unsigned int k = 0; chan[k] != -1; k++ )
			{
				if( val != INSN_CONFIG_DIO_INPUT
						&& val != INSN_CONFIG_DIO_OUTPUT
						&& val != 100 /* INSN_CONFIG_AIO_INPUT */
						&& val != 101 /* INSN_CONFIG_AIO_OUTPUT */
						&& val != 200 /* INSN_CONFIG_AIO_INPUT */
						&& val != 201 /* INSN_CONFIG_AIO_OUTPUT */
				  )
				{
					fprintf(stderr, "Configure channel %d type error type='%d'. Type must be [%d,%d,%d,%d,%d,%d])\n"
							, chan[k], val, INSN_CONFIG_DIO_INPUT, INSN_CONFIG_DIO_OUTPUT, 100, 101, 200, 201);
					exret = EXIT_FAILURE;
				}
				else
					insn_config(card, subdev, chan[k], val, range, aref );
			}
		}
		break;

		case cmdSubConfig:
			insn_subdev_config(card, subdev, val);
			break;

		default:
			comedi_perror("\n...No command... Use -h for help\n");
			exit(EXIT_FAILURE);
			break;
	}

	exit(exret);
}

void insn_config( comedi_t* card, int subdev, int channel, lsampl_t iotype, int range, int aref )
{
	lsampl_t data[2];
	comedi_insn insn;
	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_CONFIG;
	insn.n = 2;
	insn.data = data;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(channel, 0, 0); // range,aref);

	data[0] = iotype;
	data[1] = iotype;

	if( comedi_do_insn(card, &insn) < 0 )
	{
		fprintf(stderr, "(insn_config): can`t configure (AIO) subdev=%d channel=%d type=%d. Err(%d): %s\n", subdev, channel, iotype, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void insn_subdev_config( comedi_t* card, int subdev, lsampl_t type )
{
	lsampl_t data[2];
	comedi_insn insn;
	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_CONFIG;
	insn.n = 2;
	insn.data = data;
	insn.subdev = subdev;
	insn.chanspec = 0;

	data[0] = 102;
	data[1] = type;

	switch(type)
	{
		case 1:
			printf("set subdev %d type: 'TBI 24/0'\n", subdev);
			break;

		case 2:
			printf("set subdev %d type: 'TBI 0/24'\n", subdev);
			break;

		case 3:
			printf("set subdev %d type: 'TBI 16/8'\n", subdev);
			break;

		default:
			printf("set subdev %d type: UNKNOWN\n", subdev);
			break;
	}

	if( comedi_do_insn(card, &insn) < 0 )
	{
		fprintf(stderr, "(insn_subdev_config): can`t configure subdev subdev=%d type=%d. Err(%d): %s\n", subdev, type, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
