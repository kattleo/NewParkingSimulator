#ifndef VEHICLE_LIST_H
#define VEHICLE_LIST_H

#include <stddef.h>
#include "vehicle.h"

typedef struct VehicleNode
{
    Vehicle vehicle;
    struct VehicleNode *next;
} VehicleNode;

typedef struct
{
    VehicleNode *head;
    VehicleNode *tail;
    size_t size;
} VehicleList;

// Initialize empty list
void vehicle_list_init(VehicleList *list);

// Append a vehicle, returns pointer to the stored Vehicle in the list
Vehicle *vehicle_list_push_back(VehicleList *list, const Vehicle *src);

// Remove all vehicles and free all nodes
void vehicle_list_clear(VehicleList *list);

void vehicles_update_all(VehicleList *list, Map *map);

#endif
