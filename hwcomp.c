#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#define GPLHEADER \
"Copyright (C) DaErich\n\
This program is free software; you can redistribute it and/or modify it under the\n\
terms of the GNU General Public License as published by the Free Software Foundation; version 2.\n\
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;\n\
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
See the GNU General Public License for more details.\n\
You should have received a copy of the GNU General Public License along with this program;\n\
if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.\n" 


#define CATCH(EXP,MSG)                                     \
	if(EXP == -1){		                           \
		int err = errno;                           \
		die(MSG,err);                              \
	}						   

enum MODE {
	SW,
	HW
};

static _Noreturn void die(const char* message, const int err)
{
	char * diag = NULL;
	switch(err){
		case EPERM:
		case EACCES:
			diag = "Access denied";
			break;

		case EBADF:
			diag = "Invalid file descriptor";
			break;

		case EINVAL:
			diag = "Invalid value supplied";
			break;

		case ENOTDIR:
			diag = "No such file or directory";
			break;

		default:
			diag = "";
	}
	fprintf(stderr,"%s, REASON: %s!\n", message, diag);
	exit(1);
}

static _Noreturn void usage(void)
{
	fprintf(stdout,"USAGE:\n" 
		"clockcomp [-VSHh]\n"
		"  -S:\n     Relation is SW-HW(the default)\n"
		"  -H:\n     Relation is HW-SW\n"
		"  -L:\n     HW is not in UTC(usually when used with MS/DOS)\n"
		"  -V:\n     Print version information and exit\n"
		"  -h:\n     Print this help message\n"
		"SW means the Kernel's internal software clock\n"
		"HW means your board's RTC(RealtimeClock)\n"
		);
	exit(0);
}
static _Noreturn void version(void)
{
	fprintf(stdout,GPLHEADER \
			"\nclockcomp: Version " VERSION "\n"
			"Written by DaErich and tracked"
			"on Github/Codeberg\n");
	exit(0);
}

static void rtc_time_to_tm(struct rtc_time * restrict src, struct tm * restrict dest)
{
	dest->tm_sec = src->tm_sec;
	dest->tm_min = src->tm_min;
	dest->tm_hour = src->tm_hour;
	dest->tm_mday = src->tm_mday;
	dest->tm_mon = src->tm_mon;
	dest->tm_year = src->tm_year;
}


void clockcomp(enum MODE mode, bool isLocal, bool isVerbose)
{       
	int fd = 0;
	struct rtc_time times = {0};
	struct tm time_conv = {0};
	time_t hw_epoch = 0;
	time_t sw_epoch = 0;
	time_t sw_reifd = 0;
	time_t diff = 0;
	time_t diff1 = 0;	
	
	CATCH((fd = open("/dev/rtc", O_RDONLY)), "Could'nt open file descriptor")
	CATCH(ioctl(fd, RTC_RD_TIME,&times), "Could'nt call ioctl")

	diff = time(NULL);
	rtc_time_to_tm(&times,&time_conv);	
	hw_epoch = mktime(&time_conv);
	diff1 = time(NULL);
	sw_epoch = time(NULL);
	sw_reifd = sw_epoch - (diff1 - diff);

	if(!isLocal){
		tzset();
		sw_reifd = sw_reifd + timezone;
	}
	if(isVerbose){
		fprintf(stdout,"RTC got: %ld.\nSoftware Clock got: %ld.\n", hw_epoch, sw_epoch);
		if(!isLocal){
			fprintf(stdout,"Timezone drift is %ld.\n", timezone);
		}
	}
	if(mode == SW){
		time_t diff = sw_reifd - hw_epoch;
		if(diff > 0){
			fprintf(stdout,"Software clock is ahead by %ld seconds\n", diff);
		}else if (diff == 0){
			fprintf(stdout,"Software and hardware clock are (almost) perfectly syncronized!\n");
		}else if(diff < 0 ){
			fprintf(stdout,"Software clock is behind by %ld seconds\n", -diff);
		}
	}else if(mode == HW){
		time_t diff = hw_epoch - sw_reifd;
		if(diff > 0 ){
			fprintf(stdout,"Hardware clock is ahead by %ld seconds\n", diff);
		}else if (diff == 0){
			fprintf(stdout,"Hardware and software clock are (almost) perfectly syncronized!\n");
		}else if(diff < 0){
			fprintf(stdout,"Hardware clock is behind by %ld seconds\n", -diff);
		}
	}
	close(fd);
}


int main(int argc, char** argv)
{	
			
		bool isLocal = false; /* Assume HW is UTC */
		bool isVerbose = false;
		int opt = 0;
		enum MODE mode = SW; /* SW - HW is default */
		while((opt = getopt(argc, argv, "VvSHh")) != -1){
			switch(opt){

				case 'V':
					version();
					/* Not reached */
					break;

				case 'v':
					isVerbose = true;

				case 'H':
					mode = HW;
					break;

				case 'S':
					mode = SW;
					break;

				case 'L':
					isLocal = true;
					break;

				case '?':
				case ':':
				case 'h':
					usage();
					/* Not reached */
					break;
			}
		}

		clockcomp(mode, isLocal, isVerbose);

		return 0;
}

