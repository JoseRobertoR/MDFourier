/* 
 * MDFourier
 * A Fourier Transform analysis tool to compare different 
 * Sega Genesis/Mega Drive audio hardware revisions, and
 * other hardware in the future
 *
 * Copyright (C)2019 Artemio Urbina
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
 * Compile with: 
 *	  gcc -Wall -std=gnu99 -o mdfourier mdfourier.c -lfftw3 -lm
 */

#ifndef MDFOURIER_WINDOWS_H
#define MDFOURIER_WINDOWS_H

#include "mdfourier.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

float *hannWindow(int n);
float *flattopWindow(int n);
float *tukeyWindow(int n);
float *hammingWindow(int n);

int initWindows(windowManager *windows, int SamplesPerSec, parameters *config);
float *getWindowByLength(windowManager *windows, double length);
long int getWindowSizeByLength(windowManager *wm, double length);
void freeWindows(windowManager *windows);

#endif