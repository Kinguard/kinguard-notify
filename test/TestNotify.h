#include <cppunit/extensions/HelperMacros.h>
#include <kgpNotify.h>

#define VALID_ALERT_NOTIFIERS 2
#define VALID_NOTICE_NOTIFIERS 1
#define REMOVEHISTCOUNT 1
#define ARCHIVECOUNT 1
#define ACTIVETIME  2 * MAX_ACTIVE
#define HISTORYTIME 2 * MAX_HISTORY


using namespace CppUnit;

class TestNotify : public TestFixture
{

    CPPUNIT_TEST_SUITE(TestNotify);

    CPPUNIT_TEST(testNewMsg);
    CPPUNIT_TEST(testContentMsg);
    CPPUNIT_TEST(testDetailedMsg);
    CPPUNIT_TEST(testExistingMsg);
    CPPUNIT_TEST(testNoticeMsg);
    CPPUNIT_TEST(testAckMsg);
    CPPUNIT_TEST(testCleanup);
    CPPUNIT_TEST(testCleanupPersistance);
    CPPUNIT_TEST(testCleanupLevel);
    CPPUNIT_TEST(testCleanupBoot);

    CPPUNIT_TEST_SUITE_END();

private:

    string CreateTestMsg();

public:
    void setUp();
    void tearDown();

    void testNewMsg();
    void testExistingMsg();
    void testContentMsg();
    void testDetailedMsg();
    void testNoticeMsg();
    void testAckMsg();
    void testCleanup();
    void testCleanupPersistance();
    void testCleanupLevel();
    void testCleanupBoot();
};
