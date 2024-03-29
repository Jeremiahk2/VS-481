#include "Timeline.h"

int64_t start_time;
int64_t elapsed_paused_time;
int64_t last_paused_time;
int64_t tic;
bool paused;
Timeline *anchor;
float scale;

std::recursive_mutex timeMutex;

Timeline::Timeline(Timeline *anchor, int64_t tic) {
    start_time = anchor->getTime();
    this->anchor = anchor;
    this->tic = tic;
    elapsed_paused_time = 0;
    last_paused_time = start_time;
    paused = false;
    scale = 1.0;

}

Timeline::Timeline() {
    start_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    anchor = NULL;
    this->tic = DEFAULT_TIC;
    elapsed_paused_time = 0;
    last_paused_time = start_time;
    paused = false;
    scale = 1.0;
}

int64_t Timeline::getTime() {
    std::lock_guard<std::recursive_mutex> lock(timeMutex);
    //If we are paused, return the last paused time. This will make it appear as though no time is passing.
    if (paused) {
        return last_paused_time;
    }
    //If we are not the anchor, refer to the anchor's start time.
    if (anchor) {
        return (anchor->getTime() - start_time) / tic;
    }
    //If we are the anchor, use the library to get the correct time.
    else {
        return (duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - start_time - elapsed_paused_time) / tic;
        //Need to subtract by "elapsed_paused_time" so that when unpausing it looks like no time has passed.
    }
}

void Timeline::pause() {
    std::lock_guard<std::recursive_mutex> lock(timeMutex);
    last_paused_time = getTime();
    paused = true;
}

void Timeline::unpause() {
    std::lock_guard<std::recursive_mutex> lock(timeMutex);
    paused = false;
    elapsed_paused_time += getTime() - last_paused_time;
}
//Not used
void Timeline::changeTic(int tic) {
    std::lock_guard<std::recursive_mutex> lock(timeMutex);
    this->tic = tic;
}

bool Timeline::isPaused() {
    std::lock_guard<std::recursive_mutex> lock(timeMutex);
    return paused;
}

float Timeline::getRealTicLength() {
    return anchor ? tic * anchor->getRealTicLength() : scale * sf::milliseconds(1).asSeconds();
}

void Timeline::changeScale(float scale) {
    this->scale = scale;
}