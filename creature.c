#include "creature.h"
#include <time.h>
#include <pthread.h>

Creature current_creature;
pthread_mutex_t creature_lock = PTHREAD_MUTEX_INITIALIZER;

void init_creature() {
    pthread_mutex_lock(&creature_lock);

    current_creature.hunger = 80;
    current_creature.happiness = 70;
    current_creature.sleep = 100;
    current_creature.health = 85;
    current_creature.growth = 25;
    current_creature.love = 50;
    current_creature.level = 0;

    time_t now = time(NULL);
    current_creature.last_update1 = now;
    current_creature.last_update2 = now;
    current_creature.last_update3 = now;
    current_creature.last_update4 = now;

    pthread_mutex_unlock(&creature_lock);
}

// Aktualizacja cykliczna — wywoływać co minutę np.
void update_creature_time() {
    pthread_mutex_lock(&creature_lock);

    time_t now = time(NULL);

    if (difftime(now, current_creature.last_update1) >= 480) { // 8 min
        if (current_creature.hunger > 0) current_creature.hunger -= 1;
        current_creature.last_update1 = now;
    }

    if (difftime(now, current_creature.last_update2) >= 900) { // 15 min
        if (current_creature.happiness > 0) current_creature.happiness -= 1;
        if (current_creature.love > 0 && current_creature.growth < 30) current_creature.love -= 1;
        current_creature.last_update2 = now;
    }

    if (difftime(now, current_creature.last_update3) >= 600) { // 10 min
        if (current_creature.sleep > 0) current_creature.sleep -= 1;
        current_creature.last_update3 = now;
    }

    if (difftime(now, current_creature.last_update4) >= 1200) { // 20 min
        if (current_creature.growth > 0) current_creature.growth -= 1;
        if (current_creature.hunger < 30 && current_creature.health > 0) current_creature.health -= 1;
        current_creature.last_update4 = now;
    }

    if (current_creature.hunger > 70 && current_creature.happiness > 85 &&
        current_creature.growth > 80 && current_creature.love > 99) {
        current_creature.level += 1;
    }

    pthread_mutex_unlock(&creature_lock);
}

// Aktualizacja na podstawie akcji od gracza
void update_creature_action(int action) {
    pthread_mutex_lock(&creature_lock);

    switch (action) {
        case 0: // Feed
            current_creature.hunger += 10;
            if (current_creature.hunger > 100) current_creature.hunger = 100;
            break;
        case 1: // Read
            current_creature.happiness += 5;
            if (current_creature.happiness > 100) current_creature.happiness = 100;
            break;
        case 2: // Sleep
            current_creature.sleep += 20;
            if (current_creature.sleep > 100) current_creature.sleep = 100;
            break;
        case 3: // Hug
            current_creature.love += 10;
            if (current_creature.love > 100) current_creature.love = 100;
            break;
        case 4: // Play
            current_creature.happiness += 15;
            if (current_creature.happiness > 100) current_creature.happiness = 100;
            current_creature.sleep -= 10;
            if (current_creature.sleep < 0) current_creature.sleep = 0;
            break;
        default:
            break;
    }

    pthread_mutex_unlock(&creature_lock);
}
