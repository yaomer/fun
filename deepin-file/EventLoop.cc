#include <cstring>
#include <sys/socket.h>
#include <errno.h>
#include <functional>
#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include "Timer.h"

void EventLoop::addWakeChannel(void)
{
    socketpair(AF_LOCAL, SOCK_STREAM, 0, _wakeFd);
    Channel *chl = new Channel(this);
    chl->socket().setFd(_wakeFd[0]);
    chl->setReadCb(std::bind(&EventLoop::handleRead, this));
    addChannel(chl);
}

void EventLoop::run(void)
{
    addWakeChannel();
    // 每隔 3s flush一次日志
    runEvery(1000 * 3, std::bind(&Logger::wakeUp, _log));

    while (!_quit) {
        int nevents = _poller->wait(this, _timer.timeout());
        if (nevents > 0) {
            logDebug("active channels are %d", nevents);
            for (auto& it : _activeChannels)
                it.get()->handleEvent();
            _activeChannels.clear();
        } else if (nevents == 0)
            _timer.tick();
        else
            logDebug("_poller->wait error: %s", strerror(errno));
    }
}

void EventLoop::wakeUp(void)
{
    uint64_t one = 1;
    ssize_t n = write(_wakeFd[1], &one, sizeof(one));
    if (n != sizeof(one))
        logDebug("write %zd bytes instead of 8");
}

void EventLoop::handleRead(void)
{
    uint64_t one;
    ssize_t n = read(_wakeFd[0], &one, sizeof(one));
    if (n != sizeof(one))
        logDebug("write %zd bytes instead of 8");
}

void EventLoop::runAfter(int64_t timeout, TimerCallback _cb)
{
    Timestamp *t = new Timestamp;
    t->setTimeout(timeout);
    t->setInterval(0);
    t->setTimerCb(_cb);
    _timer.add(t);
    wakeUp();
}

void EventLoop::runEvery(int64_t interval, TimerCallback _cb)
{
    Timestamp *t = new Timestamp;
    t->setTimeout(interval);
    t->setInterval(interval);
    t->setTimerCb(_cb);
    _timer.add(t);
    wakeUp();
}
