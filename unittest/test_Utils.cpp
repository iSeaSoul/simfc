#include "Utils.h"
#include "gtest/gtest.h"

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class test_utils_suite : public :: testing :: Test {
protected:
    virtual void SetUp() {
        bitem = new bid_item_t();
    }
    virtual void TearDown() {
        delete bitem;
    }
    bid_item_t* bitem;
};

TEST_F(test_utils_suite, test_parse) {
    char parsed_str[100] = "1\tterm\t100\t100\ttitle\tdesc1\tdesc2\ttargeturl\tshowurl";
    bitem->parse(parsed_str, '\t');
    ASSERT_EQ(1, bitem->winfoid);
    ASSERT_EQ(100, bitem->bid);
    ASSERT_EQ(100, bitem->q);
    ASSERT_EQ(0, strcmp("term", bitem->term.c_str()));
    ASSERT_EQ(0, strcmp("title", bitem->title.c_str()));
    ASSERT_EQ(0, strcmp("desc1", bitem->desc1.c_str()));
    ASSERT_EQ(0, strcmp("desc2", bitem->desc2.c_str()));
    ASSERT_EQ(0, strcmp("targeturl", bitem->targeturl.c_str()));
    ASSERT_EQ(0, strcmp("showurl", bitem->showurl.c_str()));

    bitem->calc_weight();
    ASSERT_EQ(10000, bitem->weight);
}
