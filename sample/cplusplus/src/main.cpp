#include <genesis.h>
#include <vector>

class Message {

public:
    Message(u16 y) :
        posy(y)
    {};

    ~Message() = default;

    void say()
    {
        VDP_drawText("Hello world !", 12, posy);
    }

private:
    u16 posy;
};

int main(bool hardReset)
{
    auto msg = Message(16);
    auto msg2 = new Message(20);
    msg.say();
    msg2->say();

    std::vector<Message*> msgvec;
    msgvec.push_back(new Message(21));
    msgvec.push_back(new Message(22));

    //for (auto it = msgvec.begin(); it != msgvec.end(); ++it)
    //    (*it)->say();

    for (const auto &i : msgvec)
        i->say();

    while (true)
    {
        SYS_doVBlankProcess();
    }

    return 0;
}
