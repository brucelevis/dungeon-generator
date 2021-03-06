#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include "dungeon.h"

/*
 * "Private" methods
 */
bool room_has_door(s_dungeon *dungeon, int room, int direction);
int get_neighbour_room_index(s_dungeon *dungeon, int current_room, int direction);
int get_opposite_direction_bit(int direction);
unsigned int get_random_int(unsigned int min, unsigned int max);
void generate_room(s_dungeon *d, unsigned int cell_index_queue, int* cells_queue, unsigned int* queue_size);

// Bits for the possible doors in the current cell
int g_neighbours = BIT_DOOR_NORTH | BIT_DOOR_EAST | BIT_DOOR_SOUTH | BIT_DOOR_WEST;

/*
 * Implementations
 */

void init_dungeon(s_dungeon *d, const int width, const int height) {
	d->width = width;
	d->height = height;
	d->grid = malloc(width * height);
}

/**
 * Generate a dungeon from an empty dungeon and its size.
 * The number of rooms is defined from the given size
 *
 * For that, an array of rooms is generated.
 * The first item of the array is the first room, the entrance.
 * Then for each room (entrance included), its doors are defined:
 * - there is a door if the neighbour room has a room leading to the current
 * 		room
 * - if the neighbour room has not yet been visited, a room presence is randomly
 * 		defined
 */
void generate_dungeon(s_dungeon *d)
{
	unsigned int i, entrance, generated_cells_number;

	// Dungeon cells area number
	int dungeon_area = d->width * d->height;
	// Collection of generated cells, array of dungeon.grid indexes
	int* generated_cells = (int*) calloc(dungeon_area, sizeof(int));
	generated_cells_number = 0;

	for (i = 0 ; generated_cells_number < dungeon_area && (i == 0 || i < generated_cells_number); i++) {
		// if the cell is the first, let's define the dungeon entrance.
		if (i == 0 && generated_cells_number == 0) {
			entrance = get_random_int(0, dungeon_area);
			generated_cells[0] = entrance;
			d->grid[entrance] = BIT_ENTRANCE | BIT_USED_ROOM;
			d->entrance = entrance;
			generated_cells_number = 1;
		}

		generate_room(d, i, generated_cells, &generated_cells_number);

		if (!(d->grid[generated_cells[i]] & BIT_USED_ROOM)) {
			// The room is processed, flag is as used
			d->grid[generated_cells[i]] |= BIT_USED_ROOM;
		}

		if (i == generated_cells_number - 1 && generated_cells_number < dungeon_area * .75) {
			i = -1;
		}
	}
	free(generated_cells);
}

/**
 * Generate a room for the dungeon
 */
void generate_room(s_dungeon *d, unsigned int cell_index_queue, int* cells_queue, unsigned int* queue_size)
{
	int potential_doors = 0;
	potential_doors = get_random_int(0, g_neighbours);
	unsigned int cell_index = cells_queue[cell_index_queue];

	// Check the room's neighbours
	int door, opposite_door;
	for (door = 1; door <= g_neighbours ; door <<= 1) {
		// The bit match a door bit, ignore the others
		// or a door is already defined here
		if (
			(door & g_neighbours) != door
			|| (d->grid[cell_index] & door)
		) {
			continue;
		}

		int neighbour_room = get_neighbour_room_index(
			d,					// the dungeon
			cell_index,	// the current room index
			door				// the direction to use
		);

		// if there is no neighbour here (eg. room on the dungeon edge)
		// or the neighbour room is already used (no room defined here)
		if (!~neighbour_room || (d->grid[neighbour_room] & BIT_USED_ROOM)) {
			continue;
		}

		opposite_door = get_opposite_direction_bit(door);

		// define the doors between room and neighbour
		if ((door & potential_doors) == door) {
			d->grid[cell_index] |= door;
			d->grid[neighbour_room] |= opposite_door;

		}

		// First time neighbour room is met, stack it to be processed later
		if (d->grid[neighbour_room] == opposite_door) {
			cells_queue[*queue_size] = neighbour_room;
			(*queue_size) += 1;
		}
	}
}

/**
 * Returns a randomly generated int.
 * Uses /dev/urandom
 * @TODO Has to be improved to work on non unix system
 */
unsigned int get_random_int(unsigned int min, unsigned int max)
{
	int randomData = open("/dev/urandom", O_RDONLY);
	unsigned int rInt;
	read(randomData, &rInt, sizeof rInt);

	rInt = rInt % (max - min) + min;
	close(randomData);

	return rInt;
}

/**
 * Take a room index, a direction to know if:
 * - the room has already been visited (BIT_USED_ROOM bit set)
 * - the room has a door leading to the given direction
 *
 * Returns false if
 * - there is no possible room (current room on an edge)
 * - or there is no room
 * - or if there is a room but with no door to the current room.
 * Else returns true
 */
bool room_has_door(s_dungeon *dungeon, int room, int direction)
{
	// if the room is used and has a door to the current room
	int needed_bit = direction;
	return ((*dungeon).grid[room] & needed_bit) == needed_bit;
}

/**
 * Takes a cardinal direction in argument and returns its oposite.
 */
int get_opposite_direction_bit(int direction)
{
	int opposite_direction = -1;
	switch (direction) {
		case BIT_DOOR_NORTH:
			opposite_direction = BIT_DOOR_SOUTH;
			break;
		case BIT_DOOR_EAST:
			opposite_direction = BIT_DOOR_WEST;
			break;
		case BIT_DOOR_SOUTH:
			opposite_direction = BIT_DOOR_NORTH;
			break;
		case BIT_DOOR_WEST:
			opposite_direction = BIT_DOOR_EAST;
			break;
	}
	return opposite_direction;
}

/**
 * Takes a room index and a direction and returns the neighbour room.
 * If no neighbour room exists (eg. current_room is on an edge of the dungeon),
 * -1 is returned.
 */
int get_neighbour_room_index(s_dungeon *dungeon, int current_room, int direction)
{
	int neighbour_room, width, height;

	width = (*dungeon).width;
	height = (*dungeon).height;

	switch (direction) {
		case BIT_DOOR_NORTH:
			neighbour_room = current_room - width;
			break;
		case BIT_DOOR_EAST:
			neighbour_room = current_room + 1;
			break;
		case BIT_DOOR_SOUTH:
			neighbour_room = current_room + width;
			break;
		case BIT_DOOR_WEST:
			neighbour_room = current_room - 1;
			break;
	}

	if ((direction == BIT_DOOR_NORTH && neighbour_room >= 0)
		|| (direction == BIT_DOOR_SOUTH && neighbour_room < width * height)
		|| (direction == BIT_DOOR_EAST && neighbour_room % width > 0)
		|| (direction == BIT_DOOR_WEST && neighbour_room % width < width - 1)) {
		return neighbour_room;
	}

	return -1;
}

/**
 * Function to display the dungeon.
 * Will probably be used for debug purpose only.
 */
void display_dungeon(s_dungeon *d, int options)
{
	int i, size, rank;
	size = d->width*d->height;
	rank = 0;
	for (i = 0; i < size; i++) {
		if (!(options & VISUAL_DISPLAY_MODE)) {
			printf("%d\n", d->grid[i]);
		}
		else {
			if (d->grid[i] == 0) {
				printf("   ");
			}
			else {
				if (rank == 0) {
					if ((d->grid[i] & BIT_DOOR_NORTH) == BIT_DOOR_NORTH) {
						printf("# #");
					}
					else {
						printf("###");
					}
				}
				else if (rank == 1) {
					if ((d->grid[i] & BIT_DOOR_WEST) == BIT_DOOR_WEST) {
						printf(" ");
					}
					else {
						printf("#");
					}
					if ((d->grid[i] & BIT_ENTRANCE) == BIT_ENTRANCE) {
						printf("E");
					}
					else {
						printf(" ");
					}
					if ((d->grid[i] & BIT_DOOR_EAST) == BIT_DOOR_EAST) {
						printf(" ");
					}
					else {
						printf("#");
					}
				}
				else if (rank == 2) {
					if ((d->grid[i] & BIT_DOOR_SOUTH) == BIT_DOOR_SOUTH) {
						printf("# #");
					}
					else {
						printf("###");
					}
				}
			}
			if (i % d->width == d->width - 1) {
				printf("\n");

				rank = (rank + 1) %3;
				if (rank != 0) {
					i -= d->width;
				}
			}
		}
	}
}

void free_dungeon(s_dungeon *d) {
	free(d->grid);
}
