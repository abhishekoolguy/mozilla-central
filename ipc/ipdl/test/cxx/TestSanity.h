#ifndef mozilla__ipdltest_TestSanity_h
#define mozilla__ipdltest_TestSanity_h 1

#include "mozilla/_ipdltest/IPDLUnitTests.h"

#include "mozilla/_ipdltest/PTestSanityParent.h"
#include "mozilla/_ipdltest/PTestSanityChild.h"

namespace mozilla {
namespace _ipdltest {


class TestSanityParent :
    public PTestSanityParent
{
public:
    TestSanityParent();
    virtual ~TestSanityParent();

    static bool RunTestInProcesses() { return true; }
    static bool RunTestInThreads() { return true; }

    void Main();

protected:    
    virtual bool RecvPong(const int& one, const float& zeroPtTwoFive,
                          const uint8_t& dummy) MOZ_OVERRIDE;

    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE
    {
        if (NormalShutdown != why)
            fail("unexpected destruction!");  
        passed("ok");
        QuitParent();
    }
};


class TestSanityChild :
    public PTestSanityChild
{
public:
    TestSanityChild();
    virtual ~TestSanityChild();

protected:
    virtual bool RecvPing(const int& zero, const float& zeroPtFive,
                          const int8_t& dummy) MOZ_OVERRIDE;

    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE
    {
        if (NormalShutdown != why)
            fail("unexpected destruction!");
        QuitChild();
    }
};


} // namespace _ipdltest
} // namespace mozilla


#endif // ifndef mozilla__ipdltest_TestSanity_h
