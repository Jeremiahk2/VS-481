#include "ReqSubThread.h"

char* status;
GameWindow* rswindow;
CThread* other;
bool* rsbusy;
std::condition_variable* rscv;
Timeline* rstime;



ReqSubThread::ReqSubThread(char *status, GameWindow *window, CThread *other, bool *busy, std::condition_variable *rscv,
                           Timeline *timeline) {
    this->status = status;
    this->rswindow = window;
    this->other = other;
    this->rsbusy = busy;
    this->rscv = rscv;
    this->rstime = timeline;

}

void ReqSubThread::run() {
    //  Prepare our context and socket
    zmq::context_t context(1);
    zmq::socket_t reqSocket(context, zmq::socket_type::req);
    zmq::socket_t subSocket(context, zmq::socket_type::sub);

    //Connect
    reqSocket.connect("tcp://localhost:5555");
    subSocket.connect("tcp://localhost:5556");
    subSocket.set(zmq::sockopt::subscribe, "");

    Character* character = rswindow->getCharacter();

    list<MovingPlatform*> *movings = rswindow->getMovings();


    int64_t tic = 0;
    int64_t currentTic = 0;
    float ticLength;
    while (*status == 'c') {
        ticLength = rstime->getRealTicLength();
        currentTic = rstime->getTime();

        if (currentTic > tic) {
            //Send updated character information to server or disconnect if status is 'd'
            char characterString[MESSAGE_LIMIT];
            sprintf_s(characterString, "%d %f %f %c", character->getID(), character->getPosition().x, character->getPosition().y, *status);
            zmq::message_t request(strlen(characterString) + 1);
            memcpy(request.data(), &characterString, strlen(characterString) + 1);
            reqSocket.send(request, zmq::send_flags::none);

            //Receive confirmation
            zmq::message_t reply;
            reqSocket.recv(reply, zmq::recv_flags::none);
            char* replyString = (char*)reply.data();
            int newID;
            int matches = sscanf_s(replyString, "%d", &newID);
            character->setID(newID);
            if (newID > 9) {
                exit(1);
            }
            if (*status != 'd') {
                //Receive updated platforms
                zmq::message_t newPlatforms;
                subSocket.recv(newPlatforms, zmq::recv_flags::none);

                char* platformsString = (char*)newPlatforms.data();
                int pos = 0;
                for (MovingPlatform* i : *movings) {
                    float x = 0;
                    float y = 0;
                    int matches = sscanf_s(platformsString + pos, "%f %f %n", &x, &y, &pos);

                    i->move(x - i->getPosition().x, y - i->getPosition().y);
                }
                while (other->isBusy())
                {
                    rscv->notify_all();
                }
                *rsbusy = true;

                //Receive updated characters
                zmq::message_t newCharacters;
                subSocket.recv(newCharacters, zmq::recv_flags::none);

                char* newCharString = (char*)newCharacters.data();
                //Update the characters in the window with new ones
                rswindow->updateCharacters(newCharString);
            }
        }
        tic = currentTic;
    }

}

bool ReqSubThread::isBusy() {
    return *rsbusy;
}
