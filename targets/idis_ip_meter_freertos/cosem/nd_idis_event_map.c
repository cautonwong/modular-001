#include "targets/idis_ip_meter_freertos/cosem/nd_idis_event_map.h"

const nd_idis_event_rule_t nd_idis_event_rules[] = {
    {6, 0, 0}, /* Clock invalid */
    {7, 0, 1}, /* Battery replace */
    {12, 0, 8}, /* Program memory error */
    {13, 0, 9}, /* RAM error */
    {14, 0, 10}, /* NV memory error */
    {16, 0, 11}, /* Measurement system error */
    {15, 0, 12}, /* Watchdog error */
    {40, 0, 13}, /* Fraud attempt */
    {42, 0, 13}, /* Fraud attempt */
    {44, 0, 13}, /* Fraud attempt */
    {46, 0, 13}, /* Fraud attempt */
    {49, 0, 13}, /* Fraud attempt */
    {50, 0, 13}, /* Fraud attempt */
    {100, 0, 16}, /* M-Bus communication error ch1 */
    {110, 0, 17}, /* M-Bus communication error ch2 */
    {120, 0, 18}, /* M-Bus communication error ch3 */
    {130, 0, 19}, /* M-Bus communication error ch4 */
    {103, 0, 20}, /* M-Bus fraud attempt ch1 */
    {113, 0, 21}, /* M-Bus fraud attempt ch2 */
    {123, 0, 22}, /* M-Bus fraud attempt ch3 */
    {133, 0, 23}, /* M-Bus fraud attempt ch4 */
    {106, 0, 24}, /* Permanent error M-Bus ch1 */
    {116, 0, 25}, /* Permanent error M-Bus ch2 */
    {126, 0, 26}, /* Permanent error M-Bus ch3 */
    {136, 0, 27}, /* Permanent error M-Bus ch4 */
    {102, 0, 28}, /* Battery low on M-Bus ch1 */
    {112, 0, 29}, /* Battery low on M-Bus ch2 */
    {122, 0, 30}, /* Battery low on M-Bus ch3 */
    {132, 0, 31}, /* Battery low on M-Bus ch4 */
    {1, 1, 0}, /* Total Power Failure */
    {2, 1, 1}, /* Power Resume */
    {82, 1, 2}, /* Voltage Missing Phase L1 */
    {83, 1, 3}, /* Voltage Missing Phase L2 */
    {84, 1, 4}, /* Voltage Missing Phase L3 */
    {85, 1, 5}, /* Voltage Normal Phase L1 */
    {86, 1, 6}, /* Voltage Normal Phase L2 */
    {87, 1, 7}, /* Voltage Normal Phase L3 */
    {89, 1, 8}, /* Missing Neutral */
    {90, 1, 9}, /* Phase Asymmetry */
    {91, 1, 10}, /* Current Reversal */
    {88, 1, 11}, /* Wrong Phase Sequence */
    {52, 1, 12}, /* Unexpected Consumption */
    {48, 1, 13}, /* Key Exchanged */
    {92, 1, 14}, /* Bad Voltage Quality L1 */
    {93, 1, 15}, /* Bad Voltage Quality L2 */
    {94, 1, 16}, /* Bad Voltage Quality L3 */
    {20, 1, 17}, /* External Alert */
    {158, 1, 18}, /* Local communication established */
    {105, 1, 19}, /* New M-Bus Device Installed Ch1 */
    {115, 1, 20}, /* New M-Bus Device Installed Ch2 */
    {125, 1, 21}, /* New M-Bus Device Installed Ch3 */
    {135, 1, 22}, /* New M-Bus Device Installed Ch4 */
    {164, 1, 27}, /* M-Bus valve alarm Ch1 */
    {174, 1, 28}, /* M-Bus valve alarm Ch2 */
    {184, 1, 29}, /* M-Bus valve alarm Ch3 */
    {194, 1, 30}, /* M-Bus valve alarm Ch4 */
    {68, 1, 31}, /* Disconnect/Reconnect Failure */
};

const unsigned int nd_idis_event_rule_count =
    sizeof(nd_idis_event_rules) / sizeof(nd_idis_event_rules[0]);
