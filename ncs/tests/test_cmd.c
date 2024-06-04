#include "../unity/unity.h"
#include "../unity/unity_internals.h"
#include "./no_nfr/cmd_no_nfr.h"
#include "./no_nfr/main_no_nfr.h"
#include <string.h>

void setUp(){}
void tearDown(){}

RTDB database;

void test_cmdProcessor_EmptyBuffer(){ // Tests a return to EMPTY_BUFFER
    TEST_ASSERT_EQUAL_INT(EMPTY_BUFFER, cmdProcessor(&database));
}

void test_cmdProcessor_Bcmd(){ // Tests for the B command
    int len = 0;
    char buf[20];

    // Test Temperature request
    strcpy(buf, "#B066!"); // expected return #b0000034!
    for(int i = 0; i < 7; i++){
        rxChar(buf[i]);
    }
    TEST_ASSERT_EQUAL_INT(SUCCESS, cmdProcessor(&database));

    getTxBuffer(buf, &len);
    TEST_ASSERT_EQUAL_INT(10, len);
    TEST_ASSERT_EQUAL_STRING_LEN("#b0000034!", buf, len);

}

void test_cmdProcessor_Lcmd(){ // Test for L cmd
    int len = 0;
    char buf[20];

    strcpy(buf, "#L1125!"); // expected return #l108!
    for(int i = 0; i < 7; i++){
        rxChar(buf[i]);
    }
    TEST_ASSERT_EQUAL_INT(SUCCESS, cmdProcessor(&database));
    getTxBuffer(buf, &len);
    TEST_ASSERT_EQUAL_INT(6, len);
    TEST_ASSERT_EQUAL_STRING_LEN("#l108!", buf, len);
}

void test_cmdProcessor_Acmd(){ // Test for A cmd
    int len = 0;
    char buf[20];

    strcpy(buf, "#A065!");
    for(int i = 0; i < 6; i++){ // #a010242!
		rxChar(buf[i]);
	}
    TEST_ASSERT_EQUAL_INT(SUCCESS, cmdProcessor(&database));
    getTxBuffer(buf, &len);
    TEST_ASSERT_EQUAL_INT(9, len);
    TEST_ASSERT_EQUAL_STRING_LEN("#a010242!", buf, len);
}

void test_cmdProcessor_Ucmd(){ // Test for R cmd
    char buf[20];
    int len = 0;

    strcpy(buf, "#U02183!"); // #u02215!
    for(int i = 0; i < 8; i++){
		rxChar(buf[i]);
	}    
    TEST_ASSERT_EQUAL_INT(SUCCESS, cmdProcessor(&database));
    getTxBuffer(buf, &len);
    TEST_ASSERT_EQUAL_INT(8, len);
    TEST_ASSERT_EQUAL_STRING_LEN("#u02215!", buf, len);
}

void test_cmdProcessor_Checksum(){ // Sending commands with wrong checksum
    int len = 0;
    char buf[20];
    
    strcpy(buf, "#B234!"); // Command P
    for(int i = 0; i < 8; i++){
		rxChar(buf[i]);
	}
	TEST_ASSERT_EQUAL_INT(WRONG_CS, cmdProcessor(&database));
    
    strcpy(buf, "#L1020!"); // Command A
    for(int i = 0; i < 7; i++){
		rxChar(buf[i]);
	}
    TEST_ASSERT_EQUAL_INT(WRONG_CS, cmdProcessor(&database));
    
    strcpy(buf, "#A050!"); // Command L
    for(int i = 0; i < 6; i++){
		rxChar(buf[i]);
	}
	TEST_ASSERT_EQUAL_INT(WRONG_CS, cmdProcessor(&database)); 
	
	strcpy(buf, "#U02185!"); // Command R
    for(int i = 0; i < 8; i++){
		rxChar(buf[i]);
	}
	TEST_ASSERT_EQUAL_INT(WRONG_CS, cmdProcessor(&database));
}

void test_cmdProcessor_UnknownCommand(){ // Sending an different unknown command to the system
    char buf[20];

    strcpy(buf, "#H72!");
    for(int i = 0; i < 8; i++){
		rxChar(buf[i]);
	}
	TEST_ASSERT_EQUAL_INT(UNKNOWN_CMD, cmdProcessor(&database));
}

void test_cmdProcessor_MissingSOF(){ // Sending a command without the Start of Frame symbold '#'
    char buf[20];

    strcpy(buf, "Pt196!"); 	// missing #
    for(int i = 0; i < 8; i++){
        rxChar(buf[i]);
    }
    TEST_ASSERT_EQUAL_INT(MISSING_SOF, cmdProcessor(&database));
}

int main(void){
    resetTxBuffer();
	resetRxBuffer();
    initRTDB(&database);

    UNITY_BEGIN();
    
    RUN_TEST(test_cmdProcessor_EmptyBuffer);
    RUN_TEST(test_cmdProcessor_Bcmd);
    RUN_TEST(test_cmdProcessor_EmptyBuffer);
    RUN_TEST(test_cmdProcessor_Lcmd);
    RUN_TEST(test_cmdProcessor_Acmd);
    RUN_TEST(test_cmdProcessor_Ucmd);
    RUN_TEST(test_cmdProcessor_Acmd);
    RUN_TEST(test_cmdProcessor_Checksum);
    RUN_TEST(test_cmdProcessor_UnknownCommand);
    RUN_TEST(test_cmdProcessor_MissingSOF);

    UNITY_END();

    return 0;
}