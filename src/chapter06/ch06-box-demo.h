#include "AppBase.h"

class BoxDemo : public AppBase
{
public:
    using AppBase::AppBase;

    virtual void initialize() {};
    virtual void update(float /*dt*/) {};
    virtual void render() {};
};