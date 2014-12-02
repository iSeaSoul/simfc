#include "WordSegmenter.h"
#include "gtest/gtest.h"

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class test_wordsegmenter_suite : public :: testing :: Test {
protected:
    virtual void SetUp() {
        testWS = new WordSegmenter();
    }
    virtual void TearDown() {
        delete testWS;
    }
    WordSegmenter* testWS;
    scw_out_t* pout;
};

TEST_F(test_wordsegmenter_suite, test_segment) {
    ASSERT_EQ(true, testWS->load("../conf/fcscw.conf", "../../worddict/worddict/", 2));
    u_int scw_out_flag = SCW_OUT_ALL | SCW_OUT_PROP;
    pout = scw_create_out(10000, scw_out_flag);
    // ASSERT_NE(NULL, pout);
    
    char *test_str = "北京上海";
    char output[10][10];
    u_int segcount;
    ASSERT_EQ(true, testWS->segment(pout, test_str, output[0], 10, segcount));
    // EXCEPT_EQ("北京", output[0]) << output[0];
}
