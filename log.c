/* 
 * MDFourier
 * A Fourier Transform analysis tool to compare game console audio
 * http://junkerhq.net/MDFourier/
 *
 * Copyright (C)2019-2020 Artemio Urbina
 *
 * This file is part of the 240p Test Suite
 *
 * You can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA
 *
 * Requires the FFTW library: 
 *	  http://www.fftw.org/
 * 
 */

#include "log.h"
#include "mdfourier.h"
#include "freq.h"
#include "cline.h"

#ifndef MAX_PATH
#ifdef __MINGW32__
//MAX_PATH
#include "minwindef.h"
#endif
#endif

#define	CONSOLE_ENABLED		1

int do_log = 0;
char log_file[T_BUFFER_SIZE];
FILE *logfile = NULL;

void EnableLog() { do_log = CONSOLE_ENABLED; }
void DisableLog() { do_log = 0; }
int IsLogEnabled() { return do_log; }

void initLog()
{
	do_log = 0;
	logfile = NULL;
}

void logmsg(char *fmt, ... )
{
	va_list arguments;

	va_start(arguments, fmt);
	vprintf(fmt, arguments);
	fflush(stdout);  // output to Front end ASAP
	va_end(arguments);

	if(do_log && logfile)
	{
		va_start(arguments, fmt);
		vfprintf(logfile, fmt, arguments);
		va_end(arguments);
#ifdef DEBUG
		fflush(logfile);
#endif
	}
}

void logmsgFileOnly(char *fmt, ... )
{
	if(do_log && logfile)
	{
		va_list arguments;

		va_start(arguments, fmt);
		vfprintf(logfile, fmt, arguments);
#ifdef DEBUG
		fflush(logfile);
#endif
		va_end(arguments);
	}
}

#if defined (WIN32)
void FixLogFileName(char *name)
{
	int len;

	if(!name)
		return;
	len = strlen(name);

	if(len > MAX_PATH)
	{
		name[MAX_PATH - 5] = '.';
		name[MAX_PATH - 4] = 't';
		name[MAX_PATH - 3] = 'x';
		name[MAX_PATH - 2] = 't';
		name[MAX_PATH - 1] = '\0';
	}
}
#endif

int setLogName(char *name)
{
	sprintf(log_file, "%s", name);

	if(!do_log)
		return 0;

	remove(log_file);

#if defined (WIN32)
	FixLogFileName(log_file);
#endif

	logfile = fopen(log_file, "w");
	if(!logfile)
	{
		printf("Could not create log file %s\n", log_file);
		return 0;
	}

	//printf("\tLog enabled to file: %s\n", log_file);
	return 1;
}

void endLog()
{
	if(logfile)
	{
		fclose(logfile);
		logfile = NULL;
	}
	do_log = 0;
}

int SaveWAVEChunk(char *filename, AudioSignal *Signal, char *buffer, long int block, long int loadedBlockSize, int diff, parameters *config)
{
	FILE 		*chunk = NULL;
	wav_hdr		cheader;
	char 		FName[4096];

	cheader = Signal->header;
	if(!filename)
	{
		char Name[2048];

		sprintf(Name, "%03ld_SRC_%s_%03d_%s_%s", 
			block, GetBlockName(config, block), GetBlockSubIndex(config, block), 
			basename(Signal->SourceFile), diff ? "_diff_": "");
		ComposeFileName(FName, Name, ".wav", config);
		chunk = fopen(FName, "wb");
		filename = FName;
	}
	else
		chunk = fopen(filename, "wb");
	if(!chunk)
	{
		logmsg("\tCould not open chunk file %s\n", filename);
		return 0;
	}

	cheader.riff.ChunkSize = loadedBlockSize+36;
	cheader.data.DataSize = loadedBlockSize;
	if(fwrite(&cheader, 1, sizeof(wav_hdr), chunk) != sizeof(wav_hdr))
	{
		fclose(chunk);
		logmsg("\tCould not write chunk header to file %s\n", filename);
		return(0);
	}

	if(fwrite(buffer, 1, sizeof(char)*loadedBlockSize, chunk) != sizeof(char)*loadedBlockSize)
	{
		fclose(chunk);
		logmsg("\tCould not write samples to chunk file %s\n", filename);
		return (0);
	}

	fclose(chunk);
	return 1;
}
