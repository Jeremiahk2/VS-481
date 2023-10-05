#include "MovingPlatform.h"
#include "Character.h"
#include "Platform.h"
#include "CBox.h"
#include "GameWindow.h"
#include "MovingThread.h"
#include "CThread.h"
#include "EventThread.h"

#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <list>
#include <thread>
#include <zmq.hpp>
#include <string>

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>

#define sleep(n)	Sleep(n)
#endif
using namespace std;

/**
* KNOWN BUGS
* 1. Changing the scale to a lower scale at the precise moment that the 
* platform hits it's bounds will cause it to stop in place. This is probably because
* it's new speed is too slow to make it back through the bounded area.
* This can probably be fixed by updating the platform's position to the bound when it hits it.
* That way a speed change won't affect it.
* Moderately difficult to replicate
* 2. Very rare circumstance where if the player is on a vertical moving platform for a long time, they will fall through the platform eventually
* Very difficult to replicate (Random occurrence)
*/

//Test settings. Feel free to change these.
#define CHAR_SPEED 100.f

// #define GRAV_SPEED 160.f
#define GRAV_SPEED 160

//Speed is calculated in pixels per second.
#define PLAT_SPEED 100.f

#define PLAT_TIC 16

#define JUMP_SPEED 420.f

#define JUMP_TIME .5

#define MESSAGE_LIMIT 1024

//TESTS:
// 9/4/2023 4:26 PM
// Left alone for 30 minutes on 16 tic (at 1.0 scale) and nothing broke. My character remained on the horizontal moving platform.
// Left player alone for 24 minutes on 16 tic (at 1.0 scale) on the vertical platform and the player fell off at some point near the end of this period.
#define TIC 16 //Setting this to 4 or lower causes problems. CThread can't keep up and causes undefined behavior. Need to use mutexes or some equivalent to fix this. Though it's technically on the client.

//TODO: Combine these?
/**
* Run the CThread
*/
void run_cthread(CThread *fe) {
    fe->run();
}


void run_ethread(EventThread* fe) {
    fe->run();
}
int main() {

    //Setup window and add character.
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    GameWindow window;
    window.create(sf::VideoMode(800, 600, desktop.bitsPerPixel), "Window", sf::Style::Default);

    //Create StartPlatform and add it to the window
    Platform startPlatform;
    startPlatform.setSize(sf::Vector2f(100.f, 15.f));
    startPlatform.setFillColor(sf::Color(100, 0, 0));
    startPlatform.setPosition(sf::Vector2f(150.f - startPlatform.getSize().x, 500.f));
    window.addPlatform(&startPlatform, false);

    //Create MovingPlatform and add it to the window
    MovingPlatform moving(PLAT_SPEED, 1, startPlatform.getGlobalBounds().left + startPlatform.getGlobalBounds().width, 500.f);
    moving.setSize(sf::Vector2f(100.f, 15.f));
    moving.setFillColor(sf::Color(100, 250, 50));
    window.addPlatform(&moving, true);

    //Create endPlatform and add it to the window
    Platform endPlatform;
    endPlatform.setSize(sf::Vector2f(100.f, 15.f));
    endPlatform.setFillColor(sf::Color(218, 165, 32));
    endPlatform.setPosition(sf::Vector2f(400.f + endPlatform.getSize().x, 500.f));
    window.addPlatform(&endPlatform, false);

    MovingPlatform vertMoving(PLAT_SPEED, false, endPlatform.getPosition().x + endPlatform.getSize().x, 500.f);
    vertMoving.setSize(sf::Vector2f(50.f, 15.f));
    vertMoving.setFillColor(sf::Color::Magenta);
    window.addPlatform(&vertMoving, true);

    //Create headBonk platform (for testing jump) and add it to the window
    Platform headBonk;
    headBonk.setSize(sf::Vector2f(100.f, 15.f));
    headBonk.setFillColor(sf::Color::Blue);
    headBonk.setPosition(endPlatform.getPosition().x, endPlatform.getPosition().y - 60);
    window.addPlatform(&headBonk, false);

    //Create playable character and add it to the window as the playable object
    Character character;
    character.setSize(sf::Vector2f(30.f, 30.f));
    character.setFillColor(sf::Color::White);
    character.setOrigin(0.f, 30.f);
    character.setPosition(100.f, 100.f);
    character.setSpeed(CHAR_SPEED);

    /**
    ART FOR SANTA PROVIDED BY Marco Giorgini THROUGH OPENGAMEART.ORG
    License: CC0 (Public Domain) @ 12/28/21
    https://opengameart.org/content/santas-rats-issue-c64-sprites
    */
    sf::Texture charTexture;
    if (!charTexture.loadFromFile("Santa_standing.png")) {
        std::cout << "Failed";
    }
    character.setTexture(&charTexture);
    character.setGravity(GRAV_SPEED);
    window.addCharacter(&character);

    //Add Moving Platforms to the list.
    list<MovingPlatform*> movings;
    movings.push_front(&moving);
    movings.push_front(&vertMoving);



    //Setup timing stuff
    int64_t tic = 0;
    int64_t currentTic = 0;
    double ticLength;

    //  Prepare our context and socket
    zmq::context_t context(1);
    zmq::socket_t reqSocket(context, zmq::socket_type::req);
    zmq::socket_t subSocket(context, zmq::socket_type::sub);

    //Connect
    reqSocket.connect("tcp://localhost:5555");
    subSocket.connect("tcp://localhost:5556");

    //This should allow altering all timelines through global (pausing).
    Timeline global;
    //MPTime and CTime need to be the same tic atm. Framtime can be different (though not too low)
    Timeline FrameTime(&global, TIC);
    
    //Bool for if the threads should stop
    bool stopped = false;

    //Set up necessary thread vairables
    std::mutex m;
    std::condition_variable cv;

    bool upPressed = false;

    Timeline CTime(&global, TIC);

    bool busy = true;

    //Start collision detection
    CThread cthread(&upPressed, &window, &CTime, &stopped, &m, &cv, &busy);
    std::thread first(run_cthread, &cthread);

    EventThread ethread(&window, &global, &stopped, &cv);
    std::thread second(run_ethread, &ethread);

    Platform platforms[10];

    //Start main game loop

    double jumpTime = JUMP_TIME;

    subSocket.set(zmq::sockopt::subscribe, "");
    while (!stopped) {

        //

        ticLength = FrameTime.getRealTicLength();
        currentTic = FrameTime.getTime();

        if (currentTic > tic) {

            //Send updated character information to server
            char characterString[MESSAGE_LIMIT];
            sprintf_s(characterString, "%d %f %f", character.getID(), character.getPosition().x, character.getPosition().y);
            zmq::message_t request(strlen(characterString) + 1);
            memcpy(request.data(), &characterString, strlen(characterString) + 1);
            reqSocket.send(request, zmq::send_flags::none);

            //Receive confirmation
            zmq::message_t reply;
            reqSocket.recv(reply, zmq::recv_flags::none);
            char* replyString = (char *)reply.data();
            int newID;
            int matches = sscanf_s(replyString, "%d", &newID);
            character.setID(newID);
        

            //Receive updated platforms
            zmq::message_t newPlatforms;
            subSocket.recv(newPlatforms, zmq::recv_flags::none);

            char* platformsString = (char *)newPlatforms.data();
            int pos = 0;
            for (MovingPlatform* i : movings) {
                float x = 0;
                float y = 0;
                int matches = sscanf_s(platformsString + pos, "%f %f %n", &x, &y, &pos);

                i->move(x - i->getPosition().x, y - i->getPosition().y);
            }
            while (cthread.isBusy())
            {
                cv.notify_all();
            }
            busy = true;

            //Receive updated characters
            zmq::message_t newCharacters;
            subSocket.recv(newCharacters, zmq::recv_flags::none);

            char* newCharString = (char*)newCharacters.data();
            //Update the characters in the window with new ones
            window.updateCharacters(newCharString);

            CBox collision;

            //Need to recalculate character speed in case scale changed.
            double charSpeed = (double)character.getSpeed().x * (double)ticLength * (double)(currentTic - tic);

            //Update window visuals
            if (!stopped && !global.isPaused()) {
                window.update();
            }
            //Handle left input
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && window.hasFocus())
            {
                //Move left
                character.move(-1 * charSpeed, 0.f);

                //Check for collisions
                if (window.checkCollisions(&collision)) {
                    //If the collided platform is not moving, just correct the position of Character back.
                    if (!collision.isMoving()) {
                        character.move(charSpeed, 0.f);
                    }
                    //if the collided platform is moving, move the character back AND move them along with the platform.
                    else {
                        MovingPlatform *temp = (MovingPlatform *)collision.getPlatform();
                        if (temp->getType()) {
                            character.move(charSpeed + temp->getLastMove().x, 0);
                        }
                        else {
                            character.move(charSpeed, 0);
                        }
                    }
                }
            }

            //Handle right input
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && window.hasFocus())
            {
                // Move Right
                character.move(charSpeed, 0.f);

                //Check collisions.
                if (window.checkCollisions(&collision)) {
                    //If the collided object is not moving, just correct the position of the character back.
                    if (!collision.isMoving()) {
                        character.move(-1 * charSpeed, 0.f);
                    }
                    //If it was moving, the move it back AND along with the platform's speed.
                    else {
                        MovingPlatform *temp = (MovingPlatform *)collision.getPlatform();
                        if (temp->getType()) {
                            character.move(-1 * charSpeed + temp->getLastMove().x, 0);
                        }
                        else {
                            character.move(-1 * charSpeed, 0);
                        }
                    }
                }
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && window.hasFocus())
            {
                /*std::unique_lock<std::mutex> cv_lock(m);
                cv.wait(cv_lock);
                busy = false;*/
                if (upPressed) {
                    upPressed = false;
                    character.setJumping(true);
                }
            }
            /*busy = false;*/

            //If the character is currently jumping, move them up and check for collisions.
            double frameJump = JUMP_SPEED * (double)ticLength * (double)(currentTic - tic);
            
            if (character.isJumping()) {
                character.move(0, -1 * frameJump);
                jumpTime -= (double)ticLength * (double)(currentTic - tic);
                bool jumpCollides = window.checkCollisions(&collision);
                //Exit jumping if there are no more jump frames or if we collided with something.
                if (jumpTime <= 0 || jumpCollides) {
                    if (jumpCollides) {
                        character.move(0, frameJump);
                    }
                    character.setJumping(false);
                    jumpTime = JUMP_TIME;
                }
            }
            //Update the window's visuals.
        }
        tic = currentTic;
    }
    first.join();
    second.join();
    return EXIT_SUCCESS;
}