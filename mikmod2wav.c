#include <mikmod.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define MIKMOD2WAV_VERSION "1.0.1"

static void usage(void)
{
	fprintf(stderr,
			"mikmod2wav %s\n"
			"\n"
			"Usage: mikmod2wav <input.mod> <output.wav>\n"
			"      --limitSeconds <n>  Stop the song after <n> seconds.\n"
			"  -h, --help              Output this help and exit\n"
			"  -V, --version           Output version and exit\n"
			"\n", MIKMOD2WAV_VERSION);
	exit(EXIT_FAILURE);
}

uint32_t maxSeconds=0;
char * inputFilePath=0;
char * outputFilePath=0;

static void parse_options(int argc, char **argv)
{
	int i;

	for(i=1;i<argc;i++)
	{
		int lastarg = i==argc-1;

		if(!strcmp(argv[i],"-h") || !strcmp(argv[i], "--help"))
		{
			usage();
		}
		else if(!strcmp(argv[i],"-V") || !strcmp(argv[i], "--version"))
		{
			printf("mikmod2wav %s\n", MIKMOD2WAV_VERSION);
			exit(EXIT_SUCCESS);
		}
		else if(!strcmp(argv[i], "--limitSeconds") && !lastarg)
		{
			maxSeconds = atol(argv[++i])*1024;
		}
		else
		{
			break;
		}
	}

	argc -= i;
	argv += i;

	if(argc<2)
		usage();

	inputFilePath = argv[0];
	outputFilePath = argv[1];
}
void segfault_sigaction()
{
	fprintf(stderr, "libmikmod caused a seg fault. Sadly, this happens.\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char ** argv)
{
	MODULE * module;
	char wavDriverOptions[1024];
	struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sigaction;
    sigaction(SIGSEGV, &sa, NULL);

	parse_options(argc, argv);

	// Regsiter WAV Driver
	MikMod_RegisterDriver(&drv_wav);
	MikMod_RegisterAllLoaders();

	md_device = 1;
	md_mode = DMODE_INTERP | DMODE_16BITS | DMODE_HQMIXER | DMODE_STEREO | DMODE_SIMDMIXER;
	sprintf(wavDriverOptions, "file=%s", outputFilePath);
	if(MikMod_Init(wavDriverOptions)!=0)
	{
		fprintf(stderr, "MikMod_Init call failed: %s\n", MikMod_strerror(MikMod_errno));
		exit(EXIT_FAILURE);
	}

	module = Player_Load(inputFilePath, 256, false);
	module->loop = 0;
	if(!module)
	{
		fprintf(stderr, "Player_Load call failed: %s\n", MikMod_strerror(MikMod_errno));
		MikMod_Exit();
		exit(EXIT_FAILURE);
	}
	
	Player_Start(module);
	while(Player_Active())
	{
		MikMod_Update();
		if(maxSeconds>0 && module->sngtime>=maxSeconds)
			break;
	}
	Player_Stop();
	Player_Free(module);

	MikMod_Exit();
}
