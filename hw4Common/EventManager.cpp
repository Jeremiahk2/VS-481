#include "EventManager.h"

EventManager::EventManager()
{
	this->window = NULL;
	this->global = NULL;
}

EventManager::EventManager(GameWindow* window)
{
	this->window = window;
	this->global = NULL;
}

EventManager::EventManager(Timeline* global)
{
	this->window = NULL;
	this->global = global;
}

EventManager::EventManager(GameWindow* window, Timeline* global)
{
	this->window = window;
	this->global = global;
}

GameWindow* EventManager::getWindow()
{
	return window;
}

Timeline* EventManager::getTimeline()
{
	return global;
}



void EventManager::registerEvent(std::list<std::string> list, EventHandler* handler)
{
	for (std::string i : list) {
		if (handlers.count(i)) {
			handlers[i].push_back(handler);
		}
		else {
			std::list<EventHandler*> newList;
			newList.push_back(handler);
			handlers.insert({ i, newList });
		}
	}
}

void EventManager::deregister(std::list<std::string> list, EventHandler* handler)
{
	for (std::string i : list) {
		if (handlers.count(i)) {
			handlers[i].remove(handler);
			if (handlers[i].empty()) {
				handlers.erase(i);
			}
		}
	}
}

void EventManager::raise(Event e)
{

	//If a map at that time already exists, add it to it.
	if (auto search = raised_events.find(e.time); search != raised_events.end()) {
		search->second.insert({ e.order, e });
	}
	//If there is no map at that time, make one.
	else {
		std::multimap<int, Event> newMap;
		newMap.insert({ e.order, e });
		raised_events.insert({e.time, newMap});
	}
}
