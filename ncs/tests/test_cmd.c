#include "../unity/unity.h"
#include "../unity/unity_internals.h"
#include "./no_nfr/cmdproc.h"
#include <string.h>

void setUp(){}
void tearDown(){}

RTDB database;

void test_cmdProcessor_Bcmd(){ // Tests for the B command
    char buf[20], resp[20];
    database.but[0] = 1;
    database.but[1] = 0;
    database.but[2] = 0;
    database.but[3] = 1;

    strcpy(buf, "#B066!"); // expected return #b1001036!
    TEST_ASSERT_EQUAL_INT(SUCCESS, cmdProcessor(buf, resp, &database));
    TEST_ASSERT_EQUAL_STRING_LEN("#b1001036!", resp, 11);
}

void test_cmdProcessor_Lcmd(){ // Test for L cmd
    char buf[20], resp[20];
    database.led[0] = 0;

    // Command to toggle LED 1
    strcpy(buf, "#L1125!"); // expected return #l11206!

    TEST_ASSERT_EQUAL_INT(SUCCESS, cmdProcessor(buf, resp, &database));
    TEST_ASSERT_EQUAL_STRING_LEN("#l11206!", resp, 9);
    TEST_ASSERT_EQUAL_INT(1, database.led[0]); // Check if LED 1 is on
}

void test_cmdProcessor_Acmd(){ // Test for A cmd
    char buf[20], resp[20];
    database.anRaw = 1021;
    
    strcpy(buf, "#A065!");
    
    TEST_ASSERT_EQUAL_INT(SUCCESS, cmdProcessor(buf, resp, &database));
    TEST_ASSERT_EQUAL_STRING_LEN("#a1021037!", resp, 11);
}

void test_cmdProcessor_Ucmd(){ // Test for R cmd
    char buf[20], resp[20];

    strcpy(buf, "#U02183!"); // #u02215!

    TEST_ASSERT_EQUAL_INT(SUCCESS, cmdProcessor(buf, resp, &database));
    TEST_ASSERT_EQUAL_STRING_LEN("#u02215!", resp, 9);
}

void test_cmdProcessor_Checksum(){ // Sending commands with wrong checksum
    char buf[20], resp[20];
    
    strcpy(buf, "#B234!");      // Command B
	TEST_ASSERT_EQUAL_INT(WRONG_CS, cmdProcessor(buf, resp, &database));
    
    strcpy(buf, "#L1020!");     // Command L
    TEST_ASSERT_EQUAL_INT(WRONG_CS, cmdProcessor(buf, resp, &database));
    
    strcpy(buf, "#A050!");      // Command A
	TEST_ASSERT_EQUAL_INT(WRONG_CS, cmdProcessor(buf, resp, &database)); 
	
	strcpy(buf, "#U02185!");    // Command U
	TEST_ASSERT_EQUAL_INT(WRONG_CS, cmdProcessor(buf, resp, &database));
}

void test_cmdProcessor_UnknownCommand(){ // Sending an different unknown command to the system
    char buf[20], resp[20];

    strcpy(buf, "#H72!");
	TEST_ASSERT_EQUAL_INT(UNKNOWN_CMD, cmdProcessor(buf, resp, &database));
}

void test_cmdProcessor_MissingSOF(){ // Sending a command without the Start of Frame symbold '#'
    char buf[20], resp[20];

    strcpy(buf, "Pt196!"); 	// missing #
    TEST_ASSERT_EQUAL_INT(MISSING_SOF, cmdProcessor(buf, resp, &database));
}

int main(void){

    UNITY_BEGIN();
    
    RUN_TEST(test_cmdProcessor_Bcmd);           // Tests for B command
    RUN_TEST(test_cmdProcessor_Lcmd);           // Tests for L command
    RUN_TEST(test_cmdProcessor_Acmd);           // Tests for A command 
    RUN_TEST(test_cmdProcessor_Ucmd);           // Tests for U command
    RUN_TEST(test_cmdProcessor_Checksum);       // Tests for the Checksum
    RUN_TEST(test_cmdProcessor_UnknownCommand); // Tests for command structure
    RUN_TEST(test_cmdProcessor_MissingSOF);     // Tests for commands without SOF

    UNITY_END();

    return 0;
}