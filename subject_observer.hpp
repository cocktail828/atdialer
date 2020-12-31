#ifndef __SUBJECT_OBSERVER__
#define __SUBJECT_OBSERVER__

#include <list>
#include <string>

class IObserver
{
public:
    IObserver() {}
    virtual ~IObserver() {}
    virtual void update(const std::string &) = 0;
    virtual void update() = 0;
};

class ISubject
{
protected:
    std::list<IObserver *> observers;

public:
    ISubject() {}
    virtual ~ISubject() {}

    virtual void addObserver(IObserver *o) final
    {
        for (auto iter = observers.begin(); iter != observers.end(); iter++)
        {
            if (*iter == o)
                return;
        }
        observers.push_back(o);
    }

    virtual void removeObserver(IObserver *o) final
    {
        observers.remove(o);
    }

    virtual void notifyAll() = 0;
};

#endif //__SUBJECT_OBSERVER__
