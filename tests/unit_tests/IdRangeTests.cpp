#include "gtest/gtest.h"
#include "util/IdRange.h" // Assuming IdRange.h is accessible via this path
#include <stdexcept> // Required for std::runtime_error
#include <cstdint> // Required for uint32_t

// Test fixture for IdRange tests
class IdRangeTest : public ::testing::Test {
protected:
    IdRange range_default; // Default constructor
    IdRange range_params{5, 10}; // Constructor with parameters
};

// Test default constructor
TEST_F(IdRangeTest, DefaultConstructor) {
    EXPECT_EQ(range_default.getMinId(), 0);
    EXPECT_EQ(range_default.getMaxId(), 0);
}

// Test constructor with parameters
TEST_F(IdRangeTest, ParameterizedConstructor) {
    EXPECT_EQ(range_params.getMinId(), 5);
    EXPECT_EQ(range_params.getMaxId(), 10);
}

// Test getters with specific values
TEST(IdRangeGetterTest, Getters) {
    IdRange r1(100, 200);
    EXPECT_EQ(r1.getMinId(), 100);
    EXPECT_EQ(r1.getMaxId(), 200);

    IdRange r2(0, 0);
    EXPECT_EQ(r2.getMinId(), 0);
    EXPECT_EQ(r2.getMaxId(), 0);
}

// Test constructor with max value slightly larger than min
TEST(IdRangeConstructorTest, ConstructorMinMaxEqual) {
    IdRange r(10, 10);
    EXPECT_EQ(r.getMinId(), 10);
    EXPECT_EQ(r.getMaxId(), 10);
}

TEST(IdRangeConstructorTest, ConstructorTypical) {
    IdRange r(10, 20);
    EXPECT_EQ(r.getMinId(), 10);
    EXPECT_EQ(r.getMaxId(), 20);
}

// Test constructor validation: minId > maxId
TEST(IdRangeValidationTest, ConstructorInvalidRange) {
    EXPECT_THROW(IdRange(5, 0), std::runtime_error);
}

// Test setMinId validation: newMinId > maxId
TEST(IdRangeValidationTest, SetMinIdInvalid) {
    IdRange r(1, 5);
    EXPECT_THROW(r.setMinId(10), std::runtime_error);
}

// Test setMaxId validation: newMaxId < minId
TEST(IdRangeValidationTest, SetMaxIdInvalid) {
    IdRange r(5, 10);
    EXPECT_THROW(r.setMaxId(0), std::runtime_error);
}

// Test setters with valid ranges after default construction
// This test is tricky because setMinId might make the range (min,0) temporarily invalid
// if validate() is called inside setMinId.
// The prompt indicates validate() is called in setters.
TEST(IdRangeValidationTest, SettersValidRange) {
    IdRange r; // Default (0,0)
    // If r.setMinId(10) is called, r becomes (10,0). validate() in setMinId should throw.
    EXPECT_THROW(r.setMinId(10), std::runtime_error);

    // Let's test setting maxId first, then minId if starting from (0,0)
    IdRange r_reset_for_setters; // Default (0,0)
    EXPECT_NO_THROW(r_reset_for_setters.setMaxId(20)); // Now (0,20) - Valid
    EXPECT_EQ(r_reset_for_setters.getMinId(), 0);
    EXPECT_EQ(r_reset_for_setters.getMaxId(), 20);
    EXPECT_NO_THROW(r_reset_for_setters.setMinId(10)); // Now (10,20) - Valid
    EXPECT_EQ(r_reset_for_setters.getMinId(), 10);
    EXPECT_EQ(r_reset_for_setters.getMaxId(), 20);

    IdRange r2; // Default (0,0)
    EXPECT_NO_THROW(r2.setMaxId(5)); // Now (0,5) - Valid
    EXPECT_EQ(r2.getMinId(), 0);
    EXPECT_EQ(r2.getMaxId(), 5);
}

// Test setters that result in minId == maxId
TEST(IdRangeValidationTest, SettersMinMaxEqual) {
    IdRange r(1, 5);
    EXPECT_NO_THROW(r.setMinId(3)); // (3,5)
    EXPECT_EQ(r.getMinId(), 3);
    EXPECT_EQ(r.getMaxId(), 5);
    EXPECT_NO_THROW(r.setMaxId(3)); // (3,3)
    EXPECT_EQ(r.getMinId(), 3);
    EXPECT_EQ(r.getMaxId(), 3);
}

// Test sequence of valid settings - this test needs to be careful about intermediate states
TEST(IdRangeValidationTest, SetMinIdToValid) {
    IdRange r(1, 5);
    EXPECT_NO_THROW(r.setMinId(3)); // (3,5) - Valid
    EXPECT_EQ(r.getMinId(), 3);
    EXPECT_EQ(r.getMaxId(), 5);

    EXPECT_NO_THROW(r.setMinId(5)); // (5,5) - Valid
    EXPECT_EQ(r.getMinId(), 5);
    EXPECT_EQ(r.getMaxId(), 5);
}

TEST(IdRangeValidationTest, SetMaxIdToValid) {
    IdRange r(1, 5);
    EXPECT_NO_THROW(r.setMaxId(10)); // (1,10) - Valid
    EXPECT_EQ(r.getMinId(), 1);
    EXPECT_EQ(r.getMaxId(), 10);

    EXPECT_NO_THROW(r.setMaxId(1)); // (1,1) - Valid
    EXPECT_EQ(r.getMinId(), 1);
    EXPECT_EQ(r.getMaxId(), 1);
}

// Test setting minId first, then maxId to form a valid range from default
TEST(IdRangeValidationTest, SetMinThenMaxFromDefault) {
    IdRange r; // (0,0)
    // If we set min to 5, max is still 0. (5,0) -> validate() in setMinId throws
    EXPECT_THROW(r.setMinId(5), std::runtime_error);

    // Reset r for the next test structure
    r = IdRange(0,0);
    EXPECT_NO_THROW(r.setMaxId(10)); // (0,10) - Valid
    EXPECT_EQ(r.getMinId(), 0);
    EXPECT_EQ(r.getMaxId(), 10);
    EXPECT_NO_THROW(r.setMinId(5)); // (5,10) - Valid
    EXPECT_EQ(r.getMinId(), 5);
    EXPECT_EQ(r.getMaxId(), 10);
}

// Test setting maxId first, then minId to form a valid range from default
TEST(IdRangeValidationTest, SetMaxThenMinFromDefault) {
    IdRange r; // (0,0)
    EXPECT_NO_THROW(r.setMaxId(10)); // (0,10) - Valid
    EXPECT_EQ(r.getMinId(), 0);
    EXPECT_EQ(r.getMaxId(), 10);
    EXPECT_NO_THROW(r.setMinId(5));  // (5,10) - Valid
    EXPECT_EQ(r.getMinId(), 5);
    EXPECT_EQ(r.getMaxId(), 10);

    // Test setting minId to something valid, then maxId to something valid
    IdRange r2(0,0);
    EXPECT_NO_THROW(r2.setMinId(0)); // (0,0) still
    EXPECT_NO_THROW(r2.setMaxId(5)); // (0,5)
    EXPECT_EQ(r2.getMinId(),0);
    EXPECT_EQ(r2.getMaxId(),5);
}

// Test count() method
TEST(IdRangeCountTest, DefaultRange) {
    IdRange r; // (0,0)
    EXPECT_EQ(r.count(), 1);
}

TEST(IdRangeCountTest, SingleIdRange) {
    IdRange r(5, 5);
    EXPECT_EQ(r.count(), 1);
}

TEST(IdRangeCountTest, ValidRange) {
    IdRange r(5, 10); // 5, 6, 7, 8, 9, 10 -> 6 IDs
    EXPECT_EQ(r.count(), 6);
}

TEST(IdRangeCountTest, LargeRange) {
    IdRange r(0, 999); // 0 to 999 -> 1000 IDs
    EXPECT_EQ(r.count(), 1000);
}

// Test contains() method
TEST(IdRangeContainsTest, BasicContains) {
    IdRange r(5, 10);
    EXPECT_FALSE(r.contains(0));   // ID less than minId
    EXPECT_TRUE(r.contains(5));    // ID equal to minId
    EXPECT_TRUE(r.contains(7));    // ID between minId and maxId
    EXPECT_TRUE(r.contains(10));   // ID equal to maxId
    EXPECT_FALSE(r.contains(15));  // ID greater than maxId
}

TEST(IdRangeContainsTest, DefaultRangeContains) {
    IdRange r_default; // (0,0)
    EXPECT_TRUE(r_default.contains(0));
    EXPECT_FALSE(r_default.contains(1));
    EXPECT_FALSE(r_default.contains(static_cast<uint32_t>(-1))); // Test with a large value that might wrap around if not careful, though uint32_t won't be negative.
}

TEST(IdRangeContainsTest, SinglePointRangeContains) {
    IdRange r(5, 5);
    EXPECT_FALSE(r.contains(4));
    EXPECT_TRUE(r.contains(5));
    EXPECT_FALSE(r.contains(6));
}

TEST(IdRangeContainsTest, Boundaries) {
    IdRange r(0, 1);
    EXPECT_TRUE(r.contains(0));
    EXPECT_TRUE(r.contains(1));
    EXPECT_FALSE(r.contains(2));
}

// Test isOverlapping() method
TEST(IdRangeOverlapTest, NoOverlapSeparate) {
    IdRange r1(0, 5);
    IdRange r2(6, 10);
    EXPECT_FALSE(r1.isOverlapping(r2));
    EXPECT_FALSE(r2.isOverlapping(r1));
}

TEST(IdRangeOverlapTest, PartialOverlap) {
    IdRange r1(0, 5);
    IdRange r2(3, 7);
    EXPECT_TRUE(r1.isOverlapping(r2));
    EXPECT_TRUE(r2.isOverlapping(r1));
}

TEST(IdRangeOverlapTest, OneContainsAnother) {
    IdRange r1(0, 10);
    IdRange r2(3, 7);
    EXPECT_TRUE(r1.isOverlapping(r2));
    EXPECT_TRUE(r2.isOverlapping(r1));
}

TEST(IdRangeOverlapTest, TouchingAtBoundary) {
    // According to the implementation: std::max(this->minId, other.minId) <= std::min(this->maxId, other.maxId)
    // For r1(0,5) and r2(5,10): max(0,5) <= min(5,10)  => 5 <= 5 => TRUE
    IdRange r1(0, 5);
    IdRange r2(5, 10);
    EXPECT_TRUE(r1.isOverlapping(r2));
    EXPECT_TRUE(r2.isOverlapping(r1));
}

TEST(IdRangeOverlapTest, IdenticalRanges) {
    IdRange r1(0, 5);
    IdRange r2(0, 5);
    EXPECT_TRUE(r1.isOverlapping(r2));
    EXPECT_TRUE(r2.isOverlapping(r1));
}

TEST(IdRangeOverlapTest, DefaultRanges) {
    IdRange r1_default; // (0,0)
    IdRange r2_default; // (0,0)
    IdRange r3(0,0);
    EXPECT_TRUE(r1_default.isOverlapping(r2_default));
    EXPECT_TRUE(r1_default.isOverlapping(r3));
}

TEST(IdRangeOverlapTest, DefaultAndOther) {
    IdRange r_default; // (0,0)
    IdRange r_other(0, 5); // (0,5) overlaps with (0,0)
    EXPECT_TRUE(r_default.isOverlapping(r_other));
    EXPECT_TRUE(r_other.isOverlapping(r_default));

    IdRange r_non_overlap(1, 5); // (1,5) does not overlap with (0,0)
    EXPECT_FALSE(r_default.isOverlapping(r_non_overlap));
    EXPECT_FALSE(r_non_overlap.isOverlapping(r_default));
}

TEST(IdRangeOverlapTest, SinglePointOverlap) {
    IdRange r1(5,5);
    IdRange r2(5,5);
    EXPECT_TRUE(r1.isOverlapping(r2));

    IdRange r3(0,5);
    EXPECT_TRUE(r1.isOverlapping(r3));
    EXPECT_TRUE(r3.isOverlapping(r1));

    IdRange r4(5,10);
    EXPECT_TRUE(r1.isOverlapping(r4));
    EXPECT_TRUE(r4.isOverlapping(r1));

    IdRange r5(0,4);
    EXPECT_FALSE(r1.isOverlapping(r5));
    EXPECT_FALSE(r5.isOverlapping(r1));

    IdRange r6(6,10);
    EXPECT_FALSE(r1.isOverlapping(r6));
    EXPECT_FALSE(r6.isOverlapping(r1));
}

// Test Comparison Operators
TEST(IdRangeComparisonTest, Equality) {
    IdRange r1(0, 5);
    IdRange r2(0, 5);
    IdRange r3(0, 6);
    IdRange r4(1, 5);
    IdRange r5(1, 6);

    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 == r3);
    EXPECT_FALSE(r1 == r4);
    EXPECT_FALSE(r1 == r5);

    EXPECT_FALSE(r1 != r2);
    EXPECT_TRUE(r1 != r3);
    EXPECT_TRUE(r1 != r4);
    EXPECT_TRUE(r1 != r5);
}

TEST(IdRangeComparisonTest, LessThan) {
    IdRange r1(5, 10);
    IdRange r2(6, 10); // minId is greater
    IdRange r3(5, 11); // maxId is greater
    IdRange r4(5, 10); // equal
    IdRange r5(4, 10); // minId is less
    IdRange r6(5, 9);  // maxId is less

    EXPECT_TRUE(r1 < r2);  // (5,10) < (6,10)
    EXPECT_TRUE(r1 < r3);  // (5,10) < (5,11)
    EXPECT_FALSE(r1 < r4); // not less than equal
    EXPECT_FALSE(r1 < r5); // (5,10) is not < (4,10)
    EXPECT_FALSE(r1 < r6); // (5,10) is not < (5,9)
}

TEST(IdRangeComparisonTest, GreaterThan) {
    IdRange r1(5, 10);
    IdRange r2(6, 10);
    IdRange r3(5, 11);
    IdRange r4(5, 10);
    IdRange r5(4, 10);
    IdRange r6(5, 9);

    EXPECT_FALSE(r1 > r2);
    EXPECT_FALSE(r1 > r3);
    EXPECT_FALSE(r1 > r4); // not greater than equal
    EXPECT_TRUE(r1 > r5);  // (5,10) > (4,10)
    EXPECT_TRUE(r1 > r6);  // (5,10) > (5,9)
}

TEST(IdRangeComparisonTest, LessThanOrEqual) {
    IdRange r1(5, 10);
    IdRange r2(6, 10); // minId is greater
    IdRange r3(5, 11); // maxId is greater
    IdRange r4(5, 10); // equal
    IdRange r5(4, 10); // minId is less
    IdRange r6(5, 9);  // maxId is less

    EXPECT_TRUE(r1 <= r2);
    EXPECT_TRUE(r1 <= r3);
    EXPECT_TRUE(r1 <= r4); // less than or equal
    EXPECT_FALSE(r1 <= r5);
    EXPECT_FALSE(r1 <= r6);
}

TEST(IdRangeComparisonTest, GreaterThanOrEqual) {
    IdRange r1(5, 10);
    IdRange r2(6, 10);
    IdRange r3(5, 11);
    IdRange r4(5, 10);
    IdRange r5(4, 10);
    IdRange r6(5, 9);

    EXPECT_FALSE(r1 >= r2);
    EXPECT_FALSE(r1 >= r3);
    EXPECT_TRUE(r1 >= r4); // greater than or equal
    EXPECT_TRUE(r1 >= r5);
    EXPECT_TRUE(r1 >= r6);
}

TEST(IdRangeComparisonTest, MixedComparisons) {
    IdRange r_small_min_small_max(1, 5);
    IdRange r_small_min_large_max(1, 10);
    IdRange r_large_min_small_max(5, 7); // Max is smaller than r_small_min_large_max's max
    IdRange r_large_min_large_max(5, 10);

    // (1,5) vs (1,10)
    EXPECT_TRUE(r_small_min_small_max < r_small_min_large_max);
    EXPECT_TRUE(r_small_min_small_max <= r_small_min_large_max);
    EXPECT_FALSE(r_small_min_small_max > r_small_min_large_max);
    EXPECT_FALSE(r_small_min_small_max >= r_small_min_large_max);

    // (1,5) vs (5,7)
    EXPECT_TRUE(r_small_min_small_max < r_large_min_small_max);
    EXPECT_TRUE(r_small_min_small_max <= r_large_min_small_max);
    EXPECT_FALSE(r_small_min_small_max > r_large_min_small_max);
    EXPECT_FALSE(r_small_min_small_max >= r_large_min_small_max);

    // (1,10) vs (5,7)
    EXPECT_TRUE(r_small_min_large_max < r_large_min_small_max); // minId comparison first
    EXPECT_TRUE(r_small_min_large_max <= r_large_min_small_max);
    EXPECT_FALSE(r_small_min_large_max > r_large_min_small_max);
    EXPECT_FALSE(r_small_min_large_max >= r_large_min_small_max);

    // (5,7) vs (5,10)
    EXPECT_TRUE(r_large_min_small_max < r_large_min_large_max);
    EXPECT_TRUE(r_large_min_small_max <= r_large_min_large_max);
    EXPECT_FALSE(r_large_min_small_max > r_large_min_large_max);
    EXPECT_FALSE(r_large_min_small_max >= r_large_min_large_max);
}
