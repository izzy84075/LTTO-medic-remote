/*
*   LTTO Medic Remote
*   Copyright (C) 2014  Ryan L. "Izzy84075" Bales
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License along
*    with this program; if not, write to the Free Software Foundation, Inc.,
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef _MAIN_H_
#define _MAIN_H_

void init_cpu(void);
void interrupt isr_tmr0(void);
void queue_IR(unsigned char ms, unsigned char state);
void set_IR_state(unsigned char state);
void main(void);

#endif