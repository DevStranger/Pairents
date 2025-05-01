#include "creature.h"

void init_creature(Creature *c) {
	c->hunger = 80;
	c->happiness = 70;
	c->sleep = 100;
	c->health = 85;
	c->growth = 25;
	c->love = 50;
	c->level = 0;
}

void update_creature(Creature *c) {
	if (c->hunger > 0) c->hunger -= 1;
	if (c->sleep > 0) c->sleep -= 1;
	if (c->happiness > 0) c->happiness -= 1;
	if (c->growth > 0) c->growth -= 1;

	if (c->growth < 30 && c->love > 0) c->love -= 1;
	if (c->hunger < 30 && c->health > 0) c->health -= 1;
	
	if (c->hunger > 70 && c->happiness > 85 && c->growth > 80 && c->love > 99) {
		c->level += 1;
	}
}
