#ifndef PUBTHREAD_H
#define PUBTHREAD_H
#include <zmq.hpp>
#include <list>
#include "Timeline.h"
#include "MovingPlatform.h"
#include "CommonTypes.h"
#define MESSAGE_LIMIT 1024

class PubThread
{
private:

    Timeline* time;
    std::list<MovingPlatform*>* movings;
    std::map<int, CharStruct>* characters;


public:
    /**
    * Constructor
    */
    PubThread(Timeline* time, std::list<MovingPlatform*>* movings, std::map<int, CharStruct>* characters);

    /**
    * Not used. Possibly not needed.
    */
    bool isBusy();

    /**
    * run the program. It is responsible for the movement of a platform, which goes back and forth between two bounds.
    */
    void run();
};
#endif
