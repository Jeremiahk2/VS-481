#ifndef CHARACTER_H
#define CHARACTER_H
#include "GameObject.h"
#include "SpawnPoint.h"
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>
#include <mutex>
#define CHAR_SPEED 100.f
#define GRAV_SPEED 160
#define CHARACTER 1

/**
* Class for Character objects, or the shape that is controlled by the player.
* A character has speed, which is the amount of movement the character moves right or left per frame.
* A character also has various states like jumping.
*/
class Character : public GameObject, public sf::Sprite {

private:
    /**
    * Movement per frame of the character
    */
    sf::Vector2f speed;

    /**
    * Mutex for blocking inner processes
    */
    std::mutex innerMutex;

    /**
    * Is the character jumping right now.
    */
    bool jumping = false;

    float jumpSpeed = 420.f;
    //Number of tics (On global timeline) to jump for.
    int jumpTime = 200;

    /**
    * How much does the character fall per second
    */
    float gravity = 0.f;

    /**
    * The ID of the character. Init at -1 for a client who has not yet connected.
    * Server should change this to 0 or a positive value to show it has connected, and return
    * a negative value if it has disconnected
    */
    int id = -1;

    /**
    * True if connecting, false if disconnecting,
    */
    int connecting;
    /**
    * The texture for the character
    */
    sf::Texture texture;

    /**
    * The spawnpoint for the character
    */
    SpawnPoint spawn;

    bool dead = false;

public:
    /**
    * Character's object type
    */
    static const int objectType = 1;

    /**
    * Empty constructor, sets appropriate game object fields
    */
    Character();
    /**
    * Sets custom GameObject fields.
    */
    Character(bool stationary, bool collidable, bool drawable);

    /**
    * True if connecting, false if disconnecting.
    */
    bool isConnecting();
    /**
    * Set the connecting status of the character
    */
    void setConnecting(int connecting);

    /**
    * Returns the speed of the character.
    * @return the speed of the character
    */
    sf::Vector2f getSpeed();

    /**
    * Teleport the character to it's respawn point.
    */
    void respawn();

    /**
    * Move the character. Protected by a lock guard.
    */
    void move(float offsetX, float offsetY);

    /**
    * Move the character. Protected by a lock guard.
    */
    void move(const sf::Vector2f offset);

    /**
    * Set the position of the character. Protected by a lock guard.
    */
    void setPosition(float x, float y);

    /**
    * Set the position of the character. Protected by a lock guard.
    */
    void setPosition(const sf::Vector2f position);

    /**
    * Return the position of the character. Protected by a lock guard
    */
    sf::Vector2f getPosition();

    /**
    * Return the float rect containing the global bounds. Protected by a lock guard.
    */
    sf::FloatRect getGlobalBounds();

    /**
    * set the speed of the character
    */
    void setSpeed(float speed);

    /**
    * check if the character is currently jumping
    */
    bool isJumping();

    bool isDead();
    
    void died();

    /**
    * set the state of the character as jumping
    */
    void setJumping(bool jump);

    /*
    * Set the gravity for the character. This should be in the form of pixels/second. Set 0 if the character doesn't have gravity.
    */
    void setGravity(float gravity);

    /**
    * Return the gravity for the character.
    */
    float getGravity();

    /**
    * Set the ID of the character. This should pretty much only be used by the server.
    */
    void setID(int id);

    /**
    * Get the ID of the character.
    */
    int getID();

    /**
    * Replace the current spawn point (if any) with a new one.
    */
    void setSpawnPoint(SpawnPoint spawn);

    /**
    * Return the Character class' ID.
    */
    int getObjectType() override;

    float getJumpSpeed();

    int getJumpTime();

    /**
    * Gets a string representation of this chracter. Matches constructSelf
    * Formats as follows: 
    * type posX posY sizeX sizeY r g b
    * int float float float float int int int
    */
    std::string toString() override;

    /**
    * Create a new Character based on a string. Matches toString().
    */
    std::shared_ptr<GameObject> constructSelf(std::string self) override;

    /**
    * Create a new empty character for GameWindow use
    */
    std::shared_ptr<GameObject> makeTemplate() override;
};
#endif