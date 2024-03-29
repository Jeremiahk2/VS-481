#include "MovingPlatform.h"
#include "Character.h"
#include "Platform.h"
#include "CBox.h"
#include "GameWindow.h"
#include "MovingThread.h"
#include "CThread.h"
//Correct version

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

#define TIC 8 //Setting this to 8 seems to produce optimal behavior. At least on my machine. 16 and 32 both work but they don't look very good. 4 usually results in 8 behavior anyway.
/**
* Run the CThread
*/
void run_cthread(CThread *fe) {
    fe->run();
}


int main() {

    //Setup window and add character.
    GameWindow window;

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    window.create(sf::VideoMode(800, 600, desktop.bitsPerPixel), "Window", sf::Style::Default);

    Timeline global;
    bool stopped = false;
    std::condition_variable cv;

    //Create StartPlatform and add it to the window
    Platform startPlatform;
    startPlatform.setSize(sf::Vector2f(100.f, 15.f));
    startPlatform.setFillColor(sf::Color(100, 0, 0));
    startPlatform.setPosition(sf::Vector2f(50.f, 500.f));
    window.addPlatform(&startPlatform, false);

    //Create endPlatform and add it to the window
    Platform endPlatform;
    endPlatform.setSize(sf::Vector2f(100.f, 15.f));
    endPlatform.setFillColor(sf::Color(218, 165, 32));
    endPlatform.setPosition(sf::Vector2f(500.f, 500.f));
    window.addPlatform(&endPlatform, false);

    //Create headBonk platform (for testing jump) and add it to the window
    Platform headBonk;
    headBonk.setSize(sf::Vector2f(100.f, 15.f));
    headBonk.setFillColor(sf::Color::Blue);
    headBonk.setPosition(500.f, 440.f);
    window.addPlatform(&headBonk, false);

    //Create MovingPlatform and add it to the window
    MovingPlatform moving(PLAT_SPEED, 1, 150.f, 500.f);
    moving.setSize(sf::Vector2f(100.f, 15.f));
    moving.setFillColor(sf::Color(100, 250, 50));
    window.addPlatform(&moving, true);

    MovingPlatform vertMoving(PLAT_SPEED, false, 600.f, 500.f);
    vertMoving.setSize(sf::Vector2f(50.f, 15.f));
    vertMoving.setFillColor(sf::Color::Magenta);
    window.addPlatform(&vertMoving, true);

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
    float ticLength;

    //  Prepare our context and socket
    zmq::context_t context(1);
    zmq::socket_t reqSocket(context, zmq::socket_type::req);
    zmq::socket_t subSocket(context, zmq::socket_type::sub);

    //Connect
    reqSocket.connect("tcp://localhost:5555");
    subSocket.connect("tcp://localhost:5556");

    //MPTime and CTime need to be the same tic atm. Framtime can be different (though not too low)
    Timeline FrameTime(&global, TIC);

    //Set up necessary thread vairables
    std::mutex m;

    bool upPressed = false;

    Timeline CTime(&global, TIC);

    bool busy = true;

    //Start collision detection
    CThread cthread(&upPressed, &window, &CTime, &stopped, &m, &cv, &busy);
    std::thread first(run_cthread, &cthread);

    //std::thread second(run_ethread, &ethread);

    Platform platforms[10];

    //Start main game loop

    float jumpTime = JUMP_TIME;
    float scale = 1.0;
    char status = 'c';

    subSocket.set(zmq::sockopt::subscribe, "");
    while (window.isOpen()) {

        ticLength = FrameTime.getRealTicLength();
        currentTic = FrameTime.getTime();

        sf::Event event;
        if (currentTic > tic) {

            //TODO: Make thread to handle events, so that polling doesn't block CThread and main thread.
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    stopped = true;
                    //Need to notify all so they can stop
                    cv.notify_all();
                    first.join();
                    window.close();
                    //Set the status as disconnecting
                    status = 'd';
                }
                if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Q)) {
                    window.changeScaling();
                }
                /*if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::P)) {
                    if (global.isPaused()) {
                        global.unpause();
                    }
                    else {
                        global.pause();
                    }
                }*/
                if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Left)) {
                    if (scale != .5) {
                        scale *= .5;
                        global.changeScale(scale);
                    }
                }
                if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Right)) {
                    if (scale != 2.0) {
                        scale *= 2.0;
                        global.changeScale(scale);
                    }
                }
                if (event.type == sf::Event::Resized)
                {
                    window.handleResize(event);
                }
            }

            //Send updated character information to server or disconnect if status is 'd'
            char characterString[MESSAGE_LIMIT];
            sprintf_s(characterString, "%d %f %f %c", character.getID(), character.getPosition().x, character.getPosition().y, status);
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
            if (newID > 9) {
                exit(1);
            }
            if (status != 'd') {
                //Receive updated platforms
                zmq::message_t newPlatforms;
                subSocket.recv(newPlatforms, zmq::recv_flags::none);

                char* platformsString = (char*)newPlatforms.data();
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
            }
            

            CBox collision;

            //Need to recalculate character speed in case scale changed.
            float charSpeed = (float)character.getSpeed().x * (float)ticLength * (float)(currentTic - tic);

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
            float frameJump = JUMP_SPEED * (float)ticLength * (float)(currentTic - tic);
            
            if (character.isJumping()) {
                character.move(0, -1 * frameJump);
                jumpTime -= (float)ticLength * (float)(currentTic - tic);
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
    return EXIT_SUCCESS;
}