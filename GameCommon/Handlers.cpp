#include "Handlers.h"

void CollisionHandler::onEvent(Event e)
{
    GameObject* collision;
    Character* character;
    bool* upPressed;
    bool* doGravity;
    float ticLength;
    int differential;
    try {
        collision = e.parameters.at(std::string("collision")).m_asGameObject;
        character = (Character*)e.parameters.at(std::string("character")).m_asGameObject;
        upPressed = e.parameters.at(std::string("upPressed")).m_asBoolP;
        doGravity = e.parameters.at(std::string("doGravity")).m_asBoolP;
        ticLength = e.parameters.at(std::string("ticLength")).m_asFloat;
        differential = e.parameters.at(std::string("differential")).m_asInt;
    }
    catch (std::out_of_range) {
        std::cout << "Parameters for CollisionHandler are wrong";
        exit(3);
    }

    float oneHalfTicGrav = (character->getGravity() * ticLength) / 2;
    //If the collided object is not moving, correct the position by moving up one.
                    //NOTE: This should be impossible unless a platform was generated at the player's location (Since it's not a moving platform).
    if (collision->getObjectType() == Platform::objectType) {
        Platform* temp = (Platform*)collision;
        character->setPosition(character->getPosition().x, temp->getGlobalBounds().top - character->getGlobalBounds().height - oneHalfTicGrav);
        //Enable jumping. TODO: Rename variable to better fit. canJump? canJump(bool)?
        *upPressed = true;
        //We just teleported up to the top of a stationary platform, no need for gravity.
        *doGravity = false;
    }
    else if (collision->getObjectType() == MovingPlatform::objectType) {
        MovingPlatform* temp = (MovingPlatform*)collision;
        float platSpeed = (float)temp->getSpeedValue() * (float)ticLength * (float)(differential);
        float oneHalfTicPlat = ((float)temp->getSpeedValue() * (float)ticLength) / 2;

        //If the platform is moving horizontally.
        if (temp->getMovementType()) {
            //Since gravity hasn't happened yet, this must be a collision where it hit us from the x-axis, so put the character to the side of it.
            if (platSpeed < 0.f) {
                //If the platform is moving left, set us to the left of it.
                character->setPosition(temp->getGlobalBounds().left - character->getGlobalBounds().width - abs(oneHalfTicPlat), character->getPosition().y);
            }
            else {
                //If the platform is moving right, set us to the right of it
                character->setPosition(temp->getGlobalBounds().left + temp->getGlobalBounds().width + abs(oneHalfTicPlat), character->getPosition().y);
            }
        }
        //If the platform is moving vertically
        else {
            //If the platform is currently moving upwards
            if (platSpeed < 0.f) {
                //At this point, we know that we have been hit by a platform moving upwards, so correct our position upwards.
                character->setPosition(character->getPosition().x, temp->getGlobalBounds().top - character->getGlobalBounds().height - abs(oneHalfTicPlat));
                //We are above a platform, we can jump.
                *upPressed = true;
                //We just got placed above a platform, no need to do gravity.
                *doGravity = false;
            }
            //If the platform is moving downwards
            else {
                //Gravity will (probably) take care of it, but just in case, correct our movement to the bottom of the platform.
                character->setPosition(character->getPosition().x, temp->getGlobalBounds().top + temp->getGlobalBounds().height + oneHalfTicPlat);
            }
        }
    }
    else if (collision->getObjectType() == SideBound::objectType) {
        SideBound* sb = (SideBound*)collision;
        sb->onCollision();
    }
}

void MovementHandler::onEvent(Event e)
{
    std::cout << "Ran" << std::endl;
    Character *character;
    try {
        character = (Character*)e.parameters.at(std::string("character")).m_asGameObject;

        //Unless left is specified
        if (e.parameters.at(std::string("direction")).m_asInt == MovementHandler::LEFT) {
            character->setSpeed(sf::Vector2f(-CHAR_SPEED, 0));
        }
        else if (e.parameters.at(std::string("direction")).m_asInt == MovementHandler::RIGHT) {
            character->setSpeed(sf::Vector2f(CHAR_SPEED, 0));
        }
        else if (e.parameters.at(std::string("direction")).m_asInt == MovementHandler::UP) {
            character->setSpeed(sf::Vector2f(0, -CHAR_SPEED));
        }
        else if (e.parameters.at(std::string("direction")).m_asInt == MovementHandler::DOWN) {
            character->setSpeed(sf::Vector2f(0, CHAR_SPEED));
        }
    } 
    catch (std::out_of_range) {
        std::cout << "Parameters for MovementHandler messed up";
        exit(3);
    }
}

GravityHandler::GravityHandler(EventManager *em)
{
    this->em = em;
}

void GravityHandler::onEvent(Event e)
{
    //Get event parameters
    GameWindow* window;
    bool* upPressed;
    float ticLength;
    int differential;
    float nonScalableTicLength;
    bool* doGravity;
    try {
        window = e.parameters.at(std::string("window")).m_asGameWindow;
        upPressed = e.parameters.at(std::string("upPressed")).m_asBoolP;
        ticLength = e.parameters.at(std::string("ticLength")).m_asFloat;
        differential = e.parameters.at(std::string("differential")).m_asInt;
        nonScalableTicLength = e.parameters.at(std::string("nonScalableTicLength")).m_asFloat;
        doGravity = e.parameters.at(std::string("doGravity")).m_asBoolP;
    }
    catch (std::out_of_range) {
        std::cout << "Parameters incorrect in GravityHandler";
        exit(3);
    }

    //Set up our own variables.
    Character* character = (Character*)window->getPlayableObject();
    float oneHalfTicGrav = (character->getGravity() * ticLength) / 2;
    float gravity = character->getGravity() * ticLength * (differential);

    GameObject* collision;
    if (doGravity) {
        character->move(character->getSpeed());
        //Check collisions after gravity movement.
        if (window->checkCollisions(&collision)) {
            if (!character->isDead()) {
                Event death;
                character->died();
                Event::variant characterVariant;
                characterVariant.m_Type = Event::variant::TYPE_GAMEOBJECT;
                characterVariant.m_asGameObject = character;
                death.parameters.insert({ "character", characterVariant });
                death.type = "death";
                death.time = e.time;
                death.order = e.order + 1;
                em->raise(death);
            }
        }
    }
}

void DisconnectHandler::onEvent(Event e)
{
}

void SpawnHandler::onEvent(Event e)
{
    Character* character;
    try {
        character = (Character*)e.parameters.at("character").m_asGameObject;
    }
    catch (std::out_of_range) {
        std::cout << "Invalid arguments SpawnHandler" << std::endl;
        exit(3);
    }
    character->setSpeed(sf::Vector2f(CHAR_SPEED, 0));
    character->respawn();
}

DeathHandler::DeathHandler(EventManager* em, ScriptManager*sm)
{
    this->em = em;
    this->sm = sm;
}

void DeathHandler::onEvent(Event e)
{
    Event::events.insert_or_assign( e.guid, &e );
    sm->addArgs(&e);
    sm->runOne("handle_death", false);
}

ClosedHandler::ClosedHandler()
{
    em = NULL;
}

ClosedHandler::ClosedHandler(EventManager* em)
{
    this->em = em;
}

void ClosedHandler::onEvent(Event e)
{
    if (e.type == "Server_Closed") {
        //we are on the server, need to forcefully exit.
        try {
            std::cout << e.parameters.at("message").m_asString << std::endl;
            exit(1);
        }
        catch (std::out_of_range) {
            std::cout << "Invalid arguments ClosedHandler" << std::endl;
            exit(3);
        }
    }
    else if (e.type == "Client_Closed") {
        try {
            //Only execute if we are on the client (If there is no window, we are on the server)
            if (em && em->getWindow()) {
                std::cout << e.parameters.at("message").m_asString << std::endl;
                exit(1);
            }
        }
        catch (std::out_of_range) {
            std::cout << "Invalid arguments ClosedHandler (Client)" << std::endl;
            exit(3);
        }
    }
}

StopHandler::StopHandler()
{
}

void StopHandler::onEvent(Event e)
{
    Character* character;
    try {
        character = (Character*)e.parameters.at(std::string("character")).m_asGameObject;
    }
    catch (std::out_of_range) {
        std::cout << "Parameters for CollisionHandler are wrong";
        exit(3);
    }
    if (character->getSpeed().x == 0) {
        character->setSpeed(CHAR_SPEED);
    }
    else {
        character->setSpeed(0);
    }
}