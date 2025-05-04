#ifndef INFO_H
#define INFO_H

#include "team.h"
#include "seller.h"

#define MAX_MEMBERS 20

typedef struct {
    Chef chefs[MAX_MEMBERS];
    Baker bakers[MAX_MEMBERS];
    Seller sellers[MAX_MEMBERS];

    int chef_count;
} Info;




#endif // INFO_H