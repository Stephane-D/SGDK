/****************************************************************************
 * \brief Simple cpp demonstration.
 *
 * Simple program to demostrate C++ usage.
 *
 * Recompile SGDK with ENABLE_NEWLIB and ENABLE_CPLUSPLUS flag set to 1
 * and use makefilecpp.gen to compile the project.
 *
 * \author Pablo Porta (paspallas)
 ****************************************************************************/

#include <genesis.h>
#include <array>

class IMessage {
public:
    IMessage(const u16 pos_y) :
        _pos_y(pos_y) {}
    virtual ~IMessage() = default;
    virtual void say() const = 0;

protected:
    u16 _pos_y;
};

class Hello : public IMessage
{
public:
    Hello(const u16 pos_y) :
        IMessage(pos_y) {}

    ~Hello() = default;

    virtual void say() const override
    {
        VDP_drawText("Hello", 12, _pos_y);
    }
};

class World : public IMessage
{
public:
    World(const u16 pos_y) :
        IMessage(pos_y) {}
    ~World() = default;

    virtual void say() const override
    {
        VDP_drawText("World cpp!", 14, _pos_y);
    }
};

int main(bool hardReset)
{
    std::array<IMessage*, 2> messages{new Hello(10), new World(11)};

    for (const auto &msg : messages)
        msg->say();

    while (true)
    {
        SYS_doVBlankProcess();
    }

    return 0;
}
