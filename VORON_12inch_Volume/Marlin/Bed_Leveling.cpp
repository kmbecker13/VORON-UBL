/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "Marlin.h"
#include "math.h"

#if ENABLED(UNIFIED_BED_LEVELING_FEATURE)
#include "Bed_Leveling.h"


// These variables used to be declared inside the bed_leveling class.  We are going to still declare
// them within the .cpp file for bed leveling.   But there is only one instance of the bed leveling
// object and we can get rid of a level of inderection by not making them 'member data'.  So, in the
// interest of speed, we do it this way.    When we move to a 32-Bit processor, they can be moved 
// back inside the bed leveling class.

float last_specified_z;
float fade_scaling_factor_for_current_height;
float z_values[MESH_NUM_X_POINTS][MESH_NUM_Y_POINTS];
float mesh_index_to_X_location[MESH_NUM_X_POINTS+1];	// +1 just because of paranoia that we might end up on the
float mesh_index_to_Y_location[MESH_NUM_Y_POINTS+1];	// the last Mesh Line and that is the start of a whole new cell



bed_leveling::bed_leveling()    {
	int i;
	for (i = 0; i<=MESH_NUM_X_POINTS; i++)	// We go one past what we expect to ever need for safety
		mesh_index_to_X_location[i] = ((double)MESH_MIN_X) + (((double)MESH_X_DIST) * ((double)i));
	for (i = 0; i<=MESH_NUM_Y_POINTS; i++)	// We go one past what we expect to ever need for safety
		mesh_index_to_Y_location[i] = ((double)MESH_MIN_Y) + (((double)MESH_Y_DIST) * ((double)i));

	this->reset();
}


void bed_leveling::store_state()    {
int k;
	k = E2END - sizeof( blm.state );
	eeprom_write_block( (void *) &blm.state , (void *) k, sizeof(blm.state) );
	return;
}

void bed_leveling::load_state()    {
int k;
	k = E2END - sizeof( blm.state );
	eeprom_read_block( (void *) &blm.state , (void *) k, sizeof(blm.state) );
	if ( this->sanity_check() != 0 ) {
	   SERIAL_PROTOCOLLNPGM("?In load_state() sanity_check() failed. \n");
	}
												// These lines can go away in a few weeks.  They are just 
												// to make sure people updating thier firmware won't be using
	if (blm.state.G29_Fade_Height_Multiplier != 1.0/blm.state.G29_Correction_Fade_Height) {	// an incomplete Bed_Leveling.state structure.   For speed 
		blm.state.G29_Fade_Height_Multiplier = 1.0 / blm.state.G29_Correction_Fade_Height;// we now multiply by the inverse of the Fade Height instead of 
		store_state();									// dividing by it.   Soon... all of the old structures will be 
	}											// updated, but until then, we try to ease the transition 
												// for our Beta testers.
	return;
}

void bed_leveling::load_mesh(int m) {
int j, k;
	k = E2END - sizeof( blm.state );
	j = (k - Unified_Bed_Leveling_EEPROM_start) / sizeof( z_values );

	if ( m == -1 ) {
		SERIAL_PROTOCOLLNPGM("?No mesh saved in EEPROM.  Zeroing mesh in memory.\n");
		this->reset();
		return;
	}

	if ( m<0 || m>=j || Unified_Bed_Leveling_EEPROM_start<=0) {
		SERIAL_PROTOCOLLNPGM("?EEPROM storage not available to load mesh.\n");
		return;
	}

	j = k-(m+1)*sizeof(z_values);	
	eeprom_read_block( (void *) &z_values , (void *) j, sizeof(z_values) );

	SERIAL_PROTOCOLPGM("Mesh loaded from slot ");
	SERIAL_PROTOCOL( m );
	SERIAL_PROTOCOLPGM("  at offset 0x");
	prt_hex_word(j);
	SERIAL_PROTOCOLPGM("\n");
}

void bed_leveling:: store_mesh(int m) {
int j, k;

	k = E2END - sizeof( state );
	j = (k - Unified_Bed_Leveling_EEPROM_start) / sizeof( z_values );

	if ( m<0 || m>=j || Unified_Bed_Leveling_EEPROM_start<=0) {
		SERIAL_PROTOCOLLNPGM("?EEPROM storage not available to load mesh.\n");
SERIAL_PROTOCOL(m);
SERIAL_PROTOCOL(" mesh slots available.\n"  );
SERIAL_PROTOCOLPAIR("\nE2END     : ", E2END  );
SERIAL_PROTOCOLPAIR("\nk         : ", k  );
SERIAL_PROTOCOLPAIR("\nj         : ", j  );
SERIAL_PROTOCOLPAIR("\nm         : ", m  );
SERIAL_PROTOCOL("\n");
		return;
	}

	j = k-(m+1)*sizeof( z_values);	
	eeprom_write_block( (const void *) &z_values , (void *) j, sizeof(z_values) );

	SERIAL_PROTOCOLPGM("Mesh saved in slot ");
	SERIAL_PROTOCOL( m );
	SERIAL_PROTOCOLPGM("  at offset 0x");
	prt_hex_word(j);
	SERIAL_PROTOCOLPGM("\n");
}

void bed_leveling::reset() {
    this->state.active = 0;
    this->state.z_offset = 0;
    for (int x=0; x<MESH_NUM_X_POINTS; x++)
	for (int y=0; y<MESH_NUM_Y_POINTS; y++)
		z_values[x][y] = 0.0;

    last_specified_z = -999.9;				// We can't pre-initialize these values in the declaration 
    fade_scaling_factor_for_current_height = 0.0;	// due to C++11 constraints  
    return;
}

void bed_leveling::invalidate() {

prt_hex_word( (unsigned int) this );
SERIAL_EOL;

    this->state.active = 0;
    this->state.z_offset = 0;
    for (int x=0; x<MESH_NUM_X_POINTS; x++)
	for (int y=0; y<MESH_NUM_Y_POINTS; y++)
		z_values[x][y] = NAN;

    return;
}

void bed_leveling::display_map(int map_type)    {
float f, ff, current_xi, current_yi;
int i, j;

	SERIAL_EOL;
	SERIAL_PROTOCOLLNPGM("Bed Topography Report:\n");

	SERIAL_ECHOPAIR("(", 0 );
	SERIAL_ECHOPAIR(",", MESH_NUM_Y_POINTS-1);
	SERIAL_ECHO(")    ");

	current_xi = blm.get_cell_index_x(current_position[X_AXIS] + MESH_X_DIST/2.0);
	current_yi = blm.get_cell_index_y(current_position[Y_AXIS] + MESH_Y_DIST/2.0);

	for(i=0; i<MESH_NUM_X_POINTS-1; i++)  {
		SERIAL_ECHO("                 ");
	}

	SERIAL_ECHOPAIR("(", MESH_NUM_X_POINTS-1 );
	SERIAL_ECHOPAIR(",", MESH_NUM_Y_POINTS-1 );
	SERIAL_ECHOLN(")");

//	if (map_type || 1) {	
		SERIAL_ECHOPAIR("(", MESH_MIN_X );
		SERIAL_ECHOPAIR(",", MESH_MAX_Y );
		SERIAL_ECHO(")");

		for(i=0; i<MESH_NUM_X_POINTS-1; i++)  {
			SERIAL_ECHO("                 ");
		}

		SERIAL_ECHOPAIR("(", MESH_MAX_X );
		SERIAL_ECHOPAIR(",", MESH_MAX_Y );
		SERIAL_ECHOLN(")");
//	}

	for(j=MESH_NUM_Y_POINTS-1; j>=0; j--)  {
		for(i=0; i<MESH_NUM_X_POINTS; i++)  {
			f = z_values[i][j];
			if (i==current_xi && j==current_yi) 		// is the nozzle here?  if so, mark the number
				SERIAL_PROTOCOL("[");
			 else 
				SERIAL_PROTOCOL(" ");

			if ( isnan(f)  )
				SERIAL_PROTOCOLPGM("      .       ");
			else {
				if ( f>= 0.0 )				// if we don't do this, the columns won't line up nicely
					SERIAL_PROTOCOLPGM(" ");
				SERIAL_PROTOCOL_F( f, 5);
			}
			if (i==current_xi && j==current_yi) 		// is the nozzle here?  if so, finish marking the number
				SERIAL_PROTOCOL("]");
			 else 
				SERIAL_PROTOCOL("  ");

			SERIAL_CHAR(' ');
		}
		SERIAL_EOL;
		if (j!=0) {	// we want the (0,0) up tight against the block of numbers
			SERIAL_CHAR(' ');
			SERIAL_EOL;
		}
	}

//	if (map_type) {
		SERIAL_ECHOPAIR("(", MESH_MIN_X );
		SERIAL_ECHOPAIR(",", MESH_MIN_Y );
		SERIAL_ECHO(")    ");

		for(i=0; i<MESH_NUM_X_POINTS-1; i++)  {
			SERIAL_ECHO("                 ");
		}

		SERIAL_ECHOPAIR("(", MESH_MAX_X );
		SERIAL_ECHOPAIR(",", MESH_MIN_Y );
		SERIAL_ECHOLN(")");
//	}

	SERIAL_ECHOPAIR("(", 0 );
	SERIAL_ECHOPAIR(",", 0 );
	SERIAL_ECHO(")       ");

	for(i=0; i<MESH_NUM_X_POINTS-1; i++)  {
		SERIAL_ECHO("                 ");
	}

	SERIAL_ECHOPAIR("(", MESH_NUM_X_POINTS-1 );
	SERIAL_ECHOPAIR(",", 0 );
	SERIAL_ECHOLN(")");

	SERIAL_CHAR(' ');
	SERIAL_EOL;
	return;
}

int bed_leveling::sanity_check() {
  int j, k, error_flag = 0;

	if (this->state.n_x !=  MESH_NUM_X_POINTS)  {
	   SERIAL_PROTOCOLLNPGM("?MESH_NUM_X_POINTS set wrong\n");
	   error_flag++;
	}

	if (this->state.n_y !=  MESH_NUM_Y_POINTS)  {
	   SERIAL_PROTOCOLLNPGM("?MESH_NUM_Y_POINTS set wrong\n");
	   error_flag++;
	}

	if (this->state.mesh_x_min !=  MESH_MIN_X)  {
	   SERIAL_PROTOCOLLNPGM("?MESH_MIN_X set wrong\n");
	   error_flag++;
	}

	if (this->state.mesh_y_min !=  MESH_MIN_Y)  {
	   SERIAL_PROTOCOLLNPGM("?MESH_MIN_Y set wrong\n");
	   error_flag++;
	}

	if (this->state.mesh_x_max !=  MESH_MAX_X)  {
	   SERIAL_PROTOCOLLNPGM("?MESH_MAX_X set wrong\n");
	   error_flag++;
	}

	if (this->state.mesh_y_max !=  MESH_MAX_Y)  {
	   SERIAL_PROTOCOLLNPGM("?MESH_MAX_Y set wrong\n");
	   error_flag++;
	}

	if (this->state.mesh_x_dist !=  MESH_X_DIST)  {
	   SERIAL_PROTOCOLLNPGM("?MESH_X_DIST set wrong\n");
	   error_flag++;
	}

	if (this->state.mesh_y_dist !=  MESH_Y_DIST)  {
	   SERIAL_PROTOCOLLNPGM("?MESH_Y_DIST set wrong\n");
	   error_flag++;
	}

	k = E2END - sizeof( blm.state );
	j = (k - Unified_Bed_Leveling_EEPROM_start) / sizeof( z_values );

	if (j<1) {
	  SERIAL_PROTOCOLLNPGM("?No EEPROM storage available for a mesh of this size.\n");
	  error_flag++;
	}

//	SERIAL_PROTOCOLPGM("?sanity_check() return value: ");
//	SERIAL_PROTOCOL( error_flag );
//	SERIAL_PROTOCOLPGM("\n");

	return error_flag;
}



#endif  // UNIFIED_BED_LEVELING_FEATURE

