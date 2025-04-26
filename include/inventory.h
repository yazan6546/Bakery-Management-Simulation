//
// Created by yazan on 4/26/2025.
//

#ifndef INVENTORY_H
#define INVENTORY_H

typedef struct {
    int bread;
    int sandwich;
    int cake;
    int sweet;
    int savory;
} Inventory;

void init_inventory(Inventory *inventory);
#endif //INVENTORY_H