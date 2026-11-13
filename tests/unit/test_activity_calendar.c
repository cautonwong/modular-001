#include "unity.h"
#include <string.h>

#include "domain/tariff/activity_calendar.h"

static const nd_calendar_name_t season_name = {2, {'S', '1'}};
static const nd_calendar_name_t week_name = {2, {'W', '1'}};
static const nd_calendar_action_t actions[] = {
    {.start = {8, 0, 0, 0}, .script_ln = {1, 2, 3, 4, 5, 6},
     .script_selector = 1},
    {.start = {20, 0, 0, 0}, .script_ln = {1, 2, 3, 4, 5, 6},
     .script_selector = 2},
};
static const nd_calendar_day_t days[] = {
    {.day_id = 1, .action_count = 2, .actions = actions},
    {.day_id = 2, .action_count = 2, .actions = actions},
};
static const nd_calendar_week_t weeks[] = {
    {.name = {2, {'W', '1'}}, .day_ids = {1, 1, 1, 1, 1, 1, 1}},
};
static const nd_calendar_season_t seasons[] = {
    {.name = {2, {'S', '1'}},
     .start = {.year = 2026, .month = 1, .day = 1},
     .week = {2, {'W', '1'}}},
};
static const nd_calendar_special_day_t special_days[] = {
    {.day = {.year = 2026, .month = 11, .day = 11}, .day_id = 2},
};
static const nd_calendar_bank_t bank = {
    .name = {2, {'A', 'C'}},
    .season_count = 1,
    .week_count = 1,
    .day_count = 2,
    .seasons = seasons,
    .weeks = weeks,
    .days = days,
    .special_day_count = 1,
    .special_days = special_days,
};

void setUp(void) {}
void tearDown(void) {}

static void test_selects_latest_season_and_week_day(void)
{
    nd_activity_calendar_t calendar = {.active = bank};
    nd_datetime_t now = {0, 0, 0, 10, 11, 2026}; /* Tuesday */
    TEST_ASSERT_EQUAL_UINT8(1, nd_calendar_day_id(&bank, &now));
    const nd_calendar_day_t *day = nd_calendar_find_day(&bank, 1);
    TEST_ASSERT_NOT_NULL(day);
    now.hour = 9;
    const nd_calendar_action_t *action = nd_calendar_select_action(day, &now);
    TEST_ASSERT_NOT_NULL(action);
    TEST_ASSERT_EQUAL_UINT8(1, action->script_selector);
    now.hour = 21;
    action = nd_calendar_select_action(day, &now);
    TEST_ASSERT_NOT_NULL(action);
    TEST_ASSERT_EQUAL_UINT8(2, action->script_selector);
    TEST_ASSERT_NOT_NULL(nd_calendar_active_season(&calendar, &now));
}

static void test_special_day_overrides_week(void)
{
    nd_datetime_t now = {0, 0, 0, 11, 11, 2026};
    TEST_ASSERT_EQUAL_UINT8(2, nd_calendar_day_id(&bank, &now));
}

static void test_wildcard_matching(void)
{
    nd_calendar_datetime_t pattern = {
        .month = 11,
        .wildcard_mask = ND_CALENDAR_WILD_YEAR | ND_CALENDAR_WILD_DAY
            | ND_CALENDAR_WILD_HOUR | ND_CALENDAR_WILD_MINUTE
            | ND_CALENDAR_WILD_SECOND,
    };
    nd_datetime_t now = {0, 0, 0, 11, 11, 2026};
    TEST_ASSERT_TRUE(nd_calendar_match_date(&pattern, &now));
    now.month = 12;
    TEST_ASSERT_FALSE(nd_calendar_match_date(&pattern, &now));
}

static void test_passive_activation(void)
{
    nd_activity_calendar_t calendar = {.active = bank};
    nd_calendar_bank_t passive = bank;
    passive.name = (nd_calendar_name_t){2, {'P', 'A'}};
    nd_calendar_datetime_t activate = {.year = 2026, .month = 11, .day = 12};
    TEST_ASSERT_EQUAL_INT(0, nd_calendar_set_passive(&calendar, &passive,
                                                     &activate));
    nd_datetime_t now = {0, 0, 0, 11, 11, 2026};
    TEST_ASSERT_FALSE(nd_calendar_activation_due(&calendar, &now));
    now.day = 12;
    TEST_ASSERT_TRUE(nd_calendar_activation_due(&calendar, &now));
    TEST_ASSERT_EQUAL_INT(0, nd_calendar_activate(&calendar));
    TEST_ASSERT_EQUAL_UINT8('P', calendar.active.name.data[0]);
    TEST_ASSERT_FALSE(calendar.has_passive);
    TEST_ASSERT_EQUAL_INT(-1, nd_calendar_activate(&calendar));
}

static void test_invalid_inputs(void)
{
    TEST_ASSERT_FALSE(nd_calendar_match_datetime(NULL, NULL));
    TEST_ASSERT_NULL(nd_calendar_active_season(NULL, NULL));
    TEST_ASSERT_NULL(nd_calendar_find_week(NULL, NULL));
    TEST_ASSERT_NULL(nd_calendar_find_day(NULL, 1));
    TEST_ASSERT_NULL(nd_calendar_select_action(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, nd_calendar_set_passive(NULL, &bank, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_selects_latest_season_and_week_day);
    RUN_TEST(test_special_day_overrides_week);
    RUN_TEST(test_wildcard_matching);
    RUN_TEST(test_passive_activation);
    RUN_TEST(test_invalid_inputs);
    return UNITY_END();
}
