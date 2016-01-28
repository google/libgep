// Copyright Google Inc. Apache 2.0.

#include "utils.h"

#include <stdint.h>  // for uint8_t, int64_t
#include <stdlib.h>  // for setenv
#include <string.h>  // for memcmp, strlen
#include <sys/time.h>  // for timeval
#include <time.h>  // for tzset
#include "gtest/gtest.h"  // for EXPECT_EQ, TEST, etc

using namespace libgep_utils;

TEST(UtilTest, TimeConversion) {
  struct time_conversion_test {
    int line;
    struct timeval tv;
    int64_t usecs;
  } test_arr[] = {
    {__LINE__, { 0, 0 }, 0 },
    {__LINE__, { 0, 1 }, 1 },
    {__LINE__, { 0, kUsecsPerSec - 1 }, kUsecsPerSec - 1 },
    {__LINE__, { 1, 0 }, kUsecsPerSec },
    {__LINE__, { 1, kUsecsPerSec - 1 }, (2 * kUsecsPerSec) - 1},
  };

  for (const auto &test_item : test_arr) {
    int64_t out_usecs = timeval_to_usecs(&test_item.tv);
    EXPECT_EQ(test_item.usecs, out_usecs) << "line " << test_item.line;
    struct timeval out_tv = usecs_to_timeval(test_item.usecs);
    EXPECT_EQ(out_tv.tv_sec, test_item.tv.tv_sec) << "line " << test_item.line;
    EXPECT_EQ(out_tv.tv_usec, test_item.tv.tv_usec) << "line " <<
        test_item.line;
  }
}

TEST(UtilTest, nice_snprintf) {
  char buf[10];
  size_t bufsize = sizeof(buf);
  size_t bi = 0;

  // snprintf() returns the number of bytes that would have been written
  // had there been enough space in the buffer, not the actual number of
  // bytes written
  EXPECT_EQ(15, snprintf(buf+bi, bufsize-bi, "1234567890abcde"));

  // nice_snprintf() returns the number of bytes it actually wrote
  EXPECT_EQ(10, nice_snprintf(buf+bi, bufsize-bi, "1234567890abcde"));
}

TEST(UtilTest, snprintf_hex) {
  char tmp[1024];
  int bi;
  const char *ins = "\x01\x02\x03\x04\x00\x01\xff";
  uint8_t *in = (uint8_t *)ins;
  struct snprintf_hex_test {
    int line;
    int tmp_size;
    int in_size;
    char *expected_out;
    int expected_bi;
  } test_arr[] = {
    { __LINE__, sizeof(tmp), 4, (char *)"01020304", 8 },
    { __LINE__, sizeof(tmp), 5, (char *)"0102030400", 10 },
    { __LINE__, sizeof(tmp), 6, (char *)"010203040001", 12 },
    { __LINE__, sizeof(tmp), 7, (char *)"010203040001ff", 14 },
    { __LINE__, sizeof(tmp), 0, (char *)"", 0 },
    { __LINE__, sizeof(tmp), -1, (char *)"", 0 },
    { __LINE__, 7, 7, (char *)"010203", 7 },
    { __LINE__, 8, 7, (char *)"0102030", 8 },
  };

  for (const auto &test_item : test_arr) {
    bi = snprintf_hex(tmp, test_item.tmp_size, in, test_item.in_size);
    EXPECT_STREQ(test_item.expected_out, tmp) << test_item.line;
    EXPECT_EQ(test_item.expected_bi, bi) << test_item.line;
  }
}

TEST(UtilTest, snprintf_printable) {
  char tmp[1024];
  int bi;
  const char *ins = "\x01\x02" "a\x04\x00\x01\xff";
  uint8_t *in = (uint8_t *)ins;
  struct snprintf_printable_test {
    int line;
    int tmp_size;
    int in_size;
    char *expected_out;
    int expected_bi;
  } test_arr[] = {
    { __LINE__, sizeof(tmp), 4, (char *)"\\x01\\x02a\\x04", 13 },
    { __LINE__, sizeof(tmp), 5, (char *)"\\x01\\x02a\\x04\\x00", 17 },
    { __LINE__, sizeof(tmp), 6, (char *)"\\x01\\x02a\\x04\\x00\\x01", 21 },
    { __LINE__, sizeof(tmp), 7, (char *)"\\x01\\x02a\\x04\\x00\\x01\\xff", 25 },
    { __LINE__, sizeof(tmp), 0, (char *)"", 0 },
    { __LINE__, sizeof(tmp), -1, (char *)"", 0 },
    { __LINE__, 7, 7, (char *)"\\x01\\x0", 7 },
    { __LINE__, 8, 7, (char *)"\\x01\\x02", 8 },
    { __LINE__, 9, 7, (char *)"\\x01\\x02a", 9 },
  };

  for (const auto &test_item : test_arr) {
    bi = snprintf_printable(tmp, test_item.tmp_size, in, test_item.in_size);
    EXPECT_EQ(0, memcmp(tmp, test_item.expected_out, strlen(tmp)))
        << test_item.line;
    EXPECT_EQ(test_item.expected_bi, bi) << test_item.line;
  }
}

TEST(UtilTest, snprintf_date) {
  char tmp[1024];
  struct snprintf_date_test {
    int line;
    struct timeval tvin;
    bool full;
    int tmp_size;
    char *expected_out;
  } test_arr[] = {
    // full dates
    { __LINE__, {0, 0}, true, sizeof(tmp), (char *)"1969-12-31T16:00:00.000-0800"},
    { __LINE__, {0, 99999}, true, sizeof(tmp), (char *)"1969-12-31T16:00:00.099-0800"},
    { __LINE__, {0, 999999}, true, sizeof(tmp), (char *)"1969-12-31T16:00:00.999-0800"},
    { __LINE__, {1000000000, 0}, true, sizeof(tmp),
        (char *)"2001-09-08T18:46:40.000-0700"},
    { __LINE__, {1111111111, 0}, true, sizeof(tmp),
        (char *)"2005-03-17T17:58:31.000-0800"},
    { __LINE__, {1425808799, 0}, true, sizeof(tmp),
        (char *)"2015-03-08T01:59:59.000-0800"},
    { __LINE__, {1425808800, 0}, true, sizeof(tmp),
        (char *)"2015-03-08T03:00:00.000-0700"},
    { __LINE__, {1425808800, 0}, true, 1, (char *)""},
    { __LINE__, {1425808800, 0}, true, 10, (char *)""},
    { __LINE__, {1425808800, 0}, true, 20, (char *)"2015-03-08T03:00:00"},
    { __LINE__, {1425808800, 0}, true, 22, (char *)"2015-03-08T03:00:00.0"},
    { __LINE__, {1425808800, 0}, true, 24, (char *)"2015-03-08T03:00:00.000"},
    { __LINE__, {1425808800, 0}, true, 25, (char *)"2015-03-08T03:00:00.000"},
    { __LINE__, {1425808800, 0}, true, 26, (char *)"2015-03-08T03:00:00.000"},
    // short dates
    { __LINE__, {0, 0}, false, sizeof(tmp), (char *)"31,16:00:00.000"},
    { __LINE__, {0, 99999}, false, sizeof(tmp), (char *)"31,16:00:00.099"},
    { __LINE__, {0, 999999}, false, sizeof(tmp), (char *)"31,16:00:00.999"},
    { __LINE__, {1000000000, 0}, false, sizeof(tmp), (char *)"08,18:46:40.000"},
    { __LINE__, {1111111111, 0}, false, sizeof(tmp), (char *)"17,17:58:31.000"},
    { __LINE__, {1425808799, 0}, false, sizeof(tmp), (char *)"08,01:59:59.000"},
    { __LINE__, {1425808800, 0}, false, sizeof(tmp), (char *)"08,03:00:00.000"},
    { __LINE__, {1425808800, 0}, false, 1, (char *)""},
    { __LINE__, {1425808800, 0}, false, 10, (char *)""},
    { __LINE__, {1425808800, 0}, false, 12, (char *)"08,03:00:00"},
    { __LINE__, {1425808800, 0}, false, 14, (char *)"08,03:00:00.0"},
    { __LINE__, {1425808800, 0}, false, 16, (char *)"08,03:00:00.000"},
    { __LINE__, {1425808800, 0}, false, 17, (char *)"08,03:00:00.000"},
    { __LINE__, {1425808800, 0}, false, 18, (char *)"08,03:00:00.000"},
  };

  // somebody needs to set the TZ for snprintf_date to work
  setenv("TZ", "PST8PDT", 1);
  tzset();

  for (const auto &test_item : test_arr) {
    snprintf_date(tmp, test_item.tmp_size, &(test_item.tvin), test_item.full);
    EXPECT_STREQ(test_item.expected_out, tmp) << test_item.line;
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
