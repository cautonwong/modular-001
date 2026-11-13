#include "targets/idis_ip_meter_freertos/cosem/nd_idis_event_desc.h"

static const nd_idis_event_descriptor_t descriptors[] = {
    {1, ND_EVENT_POWER_OFF, 1, 3}, /* Power Down */
    {2, ND_EVENT_POWER_UP, 1, 0}, /* Power Up */
    {3, ND_EVENT_CLOCK, 3, 0}, /* Daylight saving time */
    {4, ND_EVENT_CLOCK, 3, 0}, /* Clock adjusted (old */
    {5, ND_EVENT_CLOCK, 3, 0}, /* Clock adjusted (new */
    {6, ND_EVENT_CLOCK, 3, 2}, /* Clock invalid */
    {7, ND_EVENT_BATTERY, 4, 1}, /* Replace Battery */
    {8, ND_EVENT_BATTERY, 4, 1}, /* Battery voltage low */
    {9, ND_EVENT_GENERIC, 11, 0}, /* TOU activated */
    {10, ND_EVENT_GENERIC, 5, 0}, /* Error register cleared */
    {11, ND_EVENT_GENERIC, 5, 0}, /* Alarm register cleared */
    {12, ND_EVENT_ERROR, 5, 2}, /* Program memory error */
    {13, ND_EVENT_ERROR, 5, 2}, /* RAM error */
    {14, ND_EVENT_ERROR, 5, 2}, /* NV memory error */
    {15, ND_EVENT_ERROR, 5, 2}, /* Watchdog error */
    {16, ND_EVENT_ERROR, 5, 2}, /* Measurement system */
    {17, ND_EVENT_GENERIC, 13, 0}, /* Firmware ready for */
    {18, ND_EVENT_GENERIC, 13, 0}, /* Firmware activated */
    {19, ND_EVENT_GENERIC, 11, 0}, /* Passive TOU */
    {20, ND_EVENT_FRAUD, 9, 1}, /* External alert detected */
    {21, ND_EVENT_GENERIC, 0, 0}, /* End of non-periodic */
    {22, ND_EVENT_COMMUNICATION, 7, 0}, /* IF_LO_2W enabled */
    {23, ND_EVENT_COMMUNICATION, 7, 0}, /* IF_LO_2W disabled */
    {26, ND_EVENT_COMMUNICATION, 7, 0}, /* Communication start on */
    {27, ND_EVENT_COMMUNICATION, 7, 0}, /* Communication end on */
    {28, ND_EVENT_COMMUNICATION, 7, 0}, /* Communication start on */
    {29, ND_EVENT_COMMUNICATION, 7, 0}, /* Communication end on */
    {30, ND_EVENT_MBUS, 8, 0}, /* M-Bus ch1 disconnector */
    {31, ND_EVENT_MBUS, 8, 0}, /* M-Bus ch1 disconnector */
    {32, ND_EVENT_MBUS, 8, 0}, /* M-Bus ch2 disconnector */
    {33, ND_EVENT_MBUS, 8, 0}, /* M-Bus ch2 disconnector */
    {34, ND_EVENT_MBUS, 8, 0}, /* M-Bus ch3 disconnector */
    {35, ND_EVENT_MBUS, 8, 0}, /* M-Bus ch3 disconnector */
    {36, ND_EVENT_MBUS, 8, 0}, /* M-Bus ch4 disconnector */
    {37, ND_EVENT_MBUS, 8, 0}, /* M-Bus ch4 disconnector */
    {38, ND_EVENT_MBUS, 8, 0}, /* M-Bus FW ready for */
    {39, ND_EVENT_MBUS, 8, 0}, /* M-Bus FW activated */
    {40, ND_EVENT_COVER_OPEN, 2, 1}, /* Terminal cover removed */
    {41, ND_EVENT_COVER_OPEN, 2, 0}, /* Terminal cover closed */
    {42, ND_EVENT_FRAUD, 9, 1}, /* Strong DC field detected */
    {43, ND_EVENT_FRAUD, 9, 1}, /* No strong DC field */
    {44, ND_EVENT_COVER_OPEN, 2, 1}, /* Meter cover removed */
    {45, ND_EVENT_COVER_OPEN, 2, 0}, /* Meter cover closed */
    {46, ND_EVENT_GENERIC, 9, 0}, /* Association */
    {47, ND_EVENT_GENERIC, 9, 1}, /* One or more parameters */
    {48, ND_EVENT_GENERIC, 9, 1}, /* Global key(s) changed */
    {49, ND_EVENT_FRAUD, 9, 3}, /* Decryption or */
    {50, ND_EVENT_FRAUD, 9, 3}, /* Replay attack */
    {51, ND_EVENT_ERROR, 13, 2}, /* FW verification failed */
    {52, ND_EVENT_FRAUD, 9, 1}, /* Unexpected consumption Indicates consumption is detected at least */
    {53, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_M1 enabled */
    {54, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_M1 disabled */
    {55, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_M2 enabled */
    {56, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_M2 disabled */
    {57, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_M3 enabled */
    {58, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_M3 disabled */
    {59, ND_EVENT_LOAD_CONTROL, 6, 0}, /* Disconnector ready for */
    {60, ND_EVENT_DISCONNECT, 12, 1}, /* Manual disconnection */
    {61, ND_EVENT_DISCONNECT, 12, 1}, /* Manual connection */
    {62, ND_EVENT_DISCONNECT, 12, 1}, /* Remote disconnection */
    {63, ND_EVENT_DISCONNECT, 12, 1}, /* Remote connection */
    {64, ND_EVENT_DISCONNECT, 12, 1}, /* Local disconnection */
    {65, ND_EVENT_DISCONNECT, 12, 1}, /* Limiter threshold */
    {66, ND_EVENT_DISCONNECT, 12, 0}, /* Limiter threshold ok */
    {67, ND_EVENT_DISCONNECT, 12, 1}, /* Limiter threshold */
    {68, ND_EVENT_DISCONNECT, 12, 1}, /* Disconnect/Reconnect */
    {69, ND_EVENT_DISCONNECT, 12, 0}, /* Local reconnection */
    {70, ND_EVENT_DISCONNECT, 12, 1}, /* Supervision monitor 1 */
    {71, ND_EVENT_DISCONNECT, 12, 1}, /* Supervision monitor 1 */
    {72, ND_EVENT_DISCONNECT, 12, 1}, /* Supervision monitor 2 */
    {73, ND_EVENT_DISCONNECT, 12, 1}, /* Supervision monitor 2 */
    {74, ND_EVENT_DISCONNECT, 12, 1}, /* Supervision monitor 3 */
    {75, ND_EVENT_DISCONNECT, 12, 1}, /* Supervision monitor 3 */
    {76, ND_EVENT_VOLTAGE, 10, 1}, /* Undervoltage L1 */
    {77, ND_EVENT_VOLTAGE, 10, 1}, /* Undervoltage L2 */
    {78, ND_EVENT_VOLTAGE, 10, 1}, /* Undervoltage L3 */
    {79, ND_EVENT_VOLTAGE, 10, 1}, /* Overvoltage L1 */
    {80, ND_EVENT_VOLTAGE, 10, 1}, /* Overvoltage L2 */
    {81, ND_EVENT_VOLTAGE, 10, 1}, /* Overvoltage L3 */
    {82, ND_EVENT_VOLTAGE, 10, 1}, /* Missing voltage L1 */
    {83, ND_EVENT_VOLTAGE, 10, 1}, /* Missing voltage L2 */
    {84, ND_EVENT_VOLTAGE, 10, 1}, /* Missing voltage L3 */
    {85, ND_EVENT_VOLTAGE, 10, 0}, /* Voltage L1 normal */
    {86, ND_EVENT_VOLTAGE, 10, 0}, /* Voltage L2 normal */
    {87, ND_EVENT_VOLTAGE, 10, 0}, /* Voltage L3 normal */
    {88, ND_EVENT_VOLTAGE, 10, 2}, /* Phase sequence reversal */
    {89, ND_EVENT_NEUTRAL_CURRENT, 10, 3}, /* Missing neutral */
    {90, ND_EVENT_VOLTAGE, 10, 1}, /* Phase Asymmetry */
    {91, ND_EVENT_OVER_CURRENT, 10, 1}, /* Current Reversal */
    {92, ND_EVENT_VOLTAGE, 10, 1}, /* Bad Voltage Quality L1 */
    {93, ND_EVENT_VOLTAGE, 10, 1}, /* Bad Voltage Quality L2 */
    {94, ND_EVENT_VOLTAGE, 10, 1}, /* Bad Voltage Quality L3 */
    {95, ND_EVENT_LOAD_CONTROL, 6, 0}, /* Disconnector activity */
    {96, ND_EVENT_LOAD_CONTROL, 6, 0}, /* Disconnector passive */
    {97, ND_EVENT_LOAD_CONTROL, 6, 0}, /* Load Mgmt activity */
    {98, ND_EVENT_LOAD_CONTROL, 6, 0}, /* Load Mgmt passive */
    {99, ND_EVENT_MBUS, 8, 1}, /* M-Bus FW verification */
    {100, ND_EVENT_MBUS, 8, 1}, /* Communication error M- */
    {101, ND_EVENT_MBUS, 8, 0}, /* Communication ok M- */
    {102, ND_EVENT_BATTERY, 8, 1}, /* Replace Battery M-Bus */
    {103, ND_EVENT_FRAUD, 8, 1}, /* Fraud attempt M-Bus */
    {104, ND_EVENT_CLOCK, 8, 0}, /* Clock adjusted M-Bus */
    {105, ND_EVENT_MBUS, 8, 0}, /* New M-Bus device */
    {106, ND_EVENT_MBUS, 8, 2}, /* Permanent Error M-Bus channel 1 */
    {107, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_M4 enabled */
    {108, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_M4 disabled */
    {109, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_1 enabled */
    {110, ND_EVENT_MBUS, 8, 1}, /* Communication error M- */
    {111, ND_EVENT_MBUS, 8, 1}, /* Communication ok M- */
    {112, ND_EVENT_BATTERY, 8, 1}, /* Replace Battery M-Bus */
    {113, ND_EVENT_FRAUD, 8, 1}, /* Fraud attempt M-Bus */
    {114, ND_EVENT_CLOCK, 8, 0}, /* Clock adjusted M-Bus */
    {115, ND_EVENT_MBUS, 8, 0}, /* New M-Bus device */
    {116, ND_EVENT_MBUS, 8, 2}, /* Permanent Error M-Bus */
    {117, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_1 disbled */
    {118, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_2 enabled */
    {119, ND_EVENT_LOAD_CONTROL, 6, 0}, /* LPCAP_2 disabled */
    {120, ND_EVENT_MBUS, 8, 1}, /* bus channel 3 */
    {121, ND_EVENT_MBUS, 8, 1}, /* Communication ok M- */
    {122, ND_EVENT_BATTERY, 8, 1}, /* Replace Battery M-Bus */
    {123, ND_EVENT_FRAUD, 8, 1}, /* Fraud attempt M-Bus */
    {124, ND_EVENT_CLOCK, 8, 0}, /* Clock adjusted M-Bus */
    {125, ND_EVENT_MBUS, 8, 0}, /* New M-Bus device */
    {126, ND_EVENT_MBUS, 8, 2}, /* Permanent Error M-Bus */
    {130, ND_EVENT_MBUS, 8, 1}, /* Communication error M- */
    {131, ND_EVENT_MBUS, 8, 1}, /* Communication ok M- */
    {132, ND_EVENT_BATTERY, 8, 1}, /* Replace Battery M-Bus */
    {133, ND_EVENT_FRAUD, 8, 1}, /* Fraud attempt M-Bus */
    {134, ND_EVENT_CLOCK, 8, 0}, /* Clock adjusted M-Bus */
    {135, ND_EVENT_MBUS, 8, 0}, /* New M-Bus device */
    {136, ND_EVENT_MBUS, 8, 2}, /* Permanent Error M-Bus */
    {140, ND_EVENT_COMMUNICATION, 7, 1}, /* No connection timeout */
    {141, ND_EVENT_COMMUNICATION, 7, 1}, /* Modem Initialization */
    {142, ND_EVENT_COMMUNICATION, 7, 2}, /* SIM Card failure */
    {143, ND_EVENT_COMMUNICATION, 7, 0}, /* SIM Card ok */
    {144, ND_EVENT_COMMUNICATION, 7, 2}, /* GSM registration failure */
    {145, ND_EVENT_COMMUNICATION, 7, 2}, /* GPRS registration failure */
    {146, ND_EVENT_COMMUNICATION, 7, 0}, /* PDP context established */
    {147, ND_EVENT_COMMUNICATION, 7, 0}, /* PDP context destroyed */
    {148, ND_EVENT_COMMUNICATION, 7, 2}, /* PDP context failure */
    {149, ND_EVENT_COMMUNICATION, 7, 0}, /* Modem SW reset */
    {150, ND_EVENT_COMMUNICATION, 7, 0}, /* Modem HW reset */
    {151, ND_EVENT_COMMUNICATION, 7, 0}, /* GSM outgoing */
    {152, ND_EVENT_COMMUNICATION, 7, 0}, /* GSM incoming */
    {153, ND_EVENT_COMMUNICATION, 7, 0}, /* GSM hang-up */
    {154, ND_EVENT_COMMUNICATION, 7, 1}, /* Diagnostic failure */
    {155, ND_EVENT_COMMUNICATION, 7, 2}, /* User initialization failure */
    {156, ND_EVENT_COMMUNICATION, 7, 1}, /* Signal quality low */
    {157, ND_EVENT_COMMUNICATION, 7, 0}, /* Auto Answer Number of */
    {158, ND_EVENT_COMMUNICATION, 7, 0}, /* Local communication */
    {160, ND_EVENT_DISCONNECT, 8, 1}, /* Manual disconnection M- Indicates that the disconnector has been */
    {161, ND_EVENT_DISCONNECT, 8, 1}, /* Manual connection M- */
    {162, ND_EVENT_DISCONNECT, 8, 1}, /* Remote disconnection */
    {163, ND_EVENT_DISCONNECT, 8, 1}, /* Remote connection M- */
    {164, ND_EVENT_DISCONNECT, 8, 1}, /* Valve alarm M-Bus */
    {165, ND_EVENT_DISCONNECT, 8, 1}, /* Local disconnection M- */
    {166, ND_EVENT_DISCONNECT, 8, 1}, /* Local connection M-Bus */
    {170, ND_EVENT_DISCONNECT, 8, 1}, /* Manual disconnection M- Indicates that the disconnector has been */
    {171, ND_EVENT_DISCONNECT, 8, 1}, /* Manual connection M- */
    {172, ND_EVENT_DISCONNECT, 8, 1}, /* Remote disconnection */
    {173, ND_EVENT_DISCONNECT, 8, 1}, /* Remote connection M- */
    {174, ND_EVENT_DISCONNECT, 8, 1}, /* Valve alarm M-Bus */
    {175, ND_EVENT_DISCONNECT, 8, 1}, /* Local disconnection M- */
    {176, ND_EVENT_DISCONNECT, 8, 1}, /* Local connection M-Bus */
    {180, ND_EVENT_DISCONNECT, 8, 1}, /* Manual disconnection M- Indicates that the disconnector has been */
    {181, ND_EVENT_DISCONNECT, 8, 1}, /* Manual connection M- */
    {182, ND_EVENT_DISCONNECT, 8, 1}, /* Remote disconnection */
    {183, ND_EVENT_DISCONNECT, 8, 1}, /* Remote connection M- */
    {184, ND_EVENT_DISCONNECT, 8, 1}, /* Valve alarm M-Bus */
    {185, ND_EVENT_DISCONNECT, 8, 1}, /* Local disconnection M- */
    {186, ND_EVENT_DISCONNECT, 8, 1}, /* Local connection M-Bus */
    {190, ND_EVENT_DISCONNECT, 8, 1}, /* Manual disconnection M- Indicates that the disconnector has been */
    {191, ND_EVENT_DISCONNECT, 8, 1}, /* Manual connection M- */
    {192, ND_EVENT_DISCONNECT, 8, 1}, /* Remote disconnection */
    {193, ND_EVENT_DISCONNECT, 8, 1}, /* Remote connection M- */
    {194, ND_EVENT_DISCONNECT, 8, 1}, /* Valve alarm M-Bus */
    {195, ND_EVENT_DISCONNECT, 8, 1}, /* Local disconnection M- */
    {196, ND_EVENT_DISCONNECT, 8, 1}, /* Local connection M-Bus */
    {254, ND_EVENT_GENERIC, 6, 0}, /* Load profile cleared */
    {255, ND_EVENT_GENERIC, 0, 0}, /* Event log cleared */
};

const nd_idis_event_descriptor_t *nd_idis_event_describe(uint16_t code)
{
    for (unsigned int index = 0; index < sizeof(descriptors) / sizeof(descriptors[0]); index++) {
        if (descriptors[index].code == code) {
            return &descriptors[index];
        }
    }
    return 0;
}
