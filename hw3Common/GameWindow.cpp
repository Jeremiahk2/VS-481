#include "GameWindow.h"

GameWindow::GameWindow() {

    innerMutex = (std::mutex*)malloc(sizeof(std::mutex));
    std::mutex tempMutex;
    if (innerMutex != NULL) {
        memcpy(innerMutex, &tempMutex, sizeof(std::mutex));
    }

    character = NULL;
}

bool GameWindow::checkCollisions(GameObject* collides) {
    bool foundCollision = false;
    GameObject *collided;
    //Cycle through the list of collidables and check if they collide with the player.
    {
        std::lock_guard<std::mutex> lock(*innerMutex);
        for (GameObject *i : collidables) {

            if (((sf::Sprite *)character)->getGlobalBounds().intersects(((sf::Shape *)i)->getGlobalBounds())) {
                // If the found collision is not moving, return it immediately
                collides = i;
                return true;
            }
        }
        for (std::shared_ptr<GameObject> i : nonStaticObjects) {
            if (i->isCollidable() && ((sf::Sprite*)character)->getGlobalBounds().intersects(((sf::Shape*)i.get())->getGlobalBounds())) {
                // If the found collision is not moving, return it immediately
                if (i->isStatic()) {
                    collides = i.get();
                    return true;
                }
            }
        }

    }
    // No collision found.
    return false;
}

//Add this client's playable character.
void GameWindow::addPlayableObject(GameObject* character) {
    std::lock_guard<std::mutex> lock(*innerMutex);
    this->character = character;
}

GameObject* GameWindow::getPlayableObject() {
    std::lock_guard<std::mutex> lock(*innerMutex);
    return character;
}

void GameWindow::addGameObject(GameObject *object) {
    std::lock_guard<std::mutex> lock(*innerMutex);
    if (object->isStatic()) {
        staticObjects.push_back(object);
    }
    if (object->isCollidable()) {
        collidables.push_front(object);
    }
    if (object->isDrawable()) {
        drawables.push_front(object);
    }
}
//Contains client objects like death bounds and side bounds.
list<GameObject*>* GameWindow::getStaticObjects() {
    std::lock_guard<std::mutex> lock(*innerMutex);
    return &staticObjects;
}
//Contains server objects like moving platforms and other characters.
list<std::shared_ptr<GameObject>>* GameWindow::getNonstaticObjects() {
    std::lock_guard<std::mutex> lock(*innerMutex);
    return &nonStaticObjects;
}

void GameWindow::update() {
    std::lock_guard<std::mutex> lock(*innerMutex);
    clear();
    //Cycle through the list of platforms and draw them.
    for (GameObject* i : drawables) {
        draw(*((sf::Drawable*)i));
    }
    for (std::shared_ptr<GameObject> i : nonStaticObjects) {
        if (i->isDrawable()) {
            draw(*((sf::Drawable*)i.get()));
        }
    }
    display();
}

void GameWindow::addTemplate(std::unique_ptr<GameObject> templateObject) {
    templates.insert_or_assign(templateObject->getObjectType(), templateObject);
}

void GameWindow::updateNonStatic(std::string updates) {
    std::lock_guard<std::mutex> lock(*innerMutex);
    nonStaticObjects.clear();
    char* current = (char*)malloc(updates.size() + 1);
    while (sscanf(updates.data(), "%s\n")) {
        int type;
        int matches = sscanf(current, "%d", &type);
        if (matches != 1) {
            throw std::invalid_argument("Failed to read string");
        }
        nonStaticObjects.push_back(templates.at(type)->constructSelf(current));
    }
}

void GameWindow::changeScaling() {
    isProportional = !isProportional;
}

bool GameWindow::checkMode() {
    return isProportional;
}

void GameWindow::handleResize(sf::Event event) {
    if (!isProportional) {
        sf::FloatRect visibleArea(0, 0, (float)event.size.width, (float)event.size.height);
        setView(sf::View(visibleArea));
    }
}