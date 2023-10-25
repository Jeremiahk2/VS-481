#include "MovingPlatform.h"
#include "Character.h"
#include "CBox.h"
#include "GameWindow.h"
#include "CThread.h"
#include "ReqThread.h"
#include "SubThread.h"
//Correct version

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

#define TIC 8 //Setting this to 8 seems to produce optimal behavior. At least on my machine. 16 and 32 both work but they don't look very good. 4 usually results in 8 behavior anyway.
/**
* Run the CThread
*/
void run_cthread(CThread *fe) {
    fe->run();
}

void run_reqthread(ReqThread* fe) {
    fe->run();
}
void run_subthread(SubThread* fe) {
    fe->run();
}


int main() {

    //Setup window and add character.
    GameWindow window;

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    window.create(sf::VideoMode(800, 600, desktop.bitsPerPixel), "Window", sf::Style::Default);

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
    
    std::cout << character.toString();


    //END SETTING UP GAME OBJECTS

    //Set up timing variables
    int64_t tic = 0;
    int64_t currentTic = 0;
    float scale = 1.0;
    float ticLength;
    float jumpTime = JUMP_TIME;

    //Set up necessary thread vairables
    std::mutex mutex;
    bool upPressed = false;
    bool busy = true;
    bool stopped = false;
    std::condition_variable cv;

    //Set up Timelines
    Timeline global;
    Timeline FrameTime(&global, TIC);
    Timeline CTime(&global, TIC);
    Timeline RTime(&global, TIC);
    Timeline STime(&global, TIC);

    //Start collision detection thread
    CThread cthread(&upPressed, &window, &CTime, &stopped, &mutex, &cv, &busy);
    std::thread first(run_cthread, &cthread);

    //Start server/client req/rep
    ReqThread reqthread(&stopped, &window, &cthread, &busy, &cv, &CTime, &mutex);
    std::thread second(run_reqthread, &reqthread);

    //start server/client pub/sub
    SubThread subthread(&stopped, &window, &cthread, &busy, &cv, &CTime, &mutex);
    std::thread third(run_subthread, &subthread);

    while (window.isOpen()) {

        ticLength = FrameTime.getRealTicLength();
        currentTic = FrameTime.getTime();

        sf::Event event;
        if (currentTic > tic) {
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    stopped = true;
                    //Need to notify all so they can stop
                    cv.notify_all();
                    first.join();
                    second.join();
                    third.join();
                    window.close();

                }
                if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Q)) {
                    window.changeScaling();
                }
                //Uncomment below to enable pausing
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

            //END EVENT CHECKS
            CBox collision;
            //Need to recalculate character speed in case scale changed.
            float charSpeed = (float)character.getSpeed().x * (float)ticLength * (float)(currentTic - tic);

            //Update window visuals
            if (!stopped && !global.isPaused()) {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock);
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
                if (upPressed) {
                    upPressed = false;
                    character.setJumping(true);
                }
            }

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
        }
        tic = currentTic;
    }

    return EXIT_SUCCESS;
}