#ifndef ND_TARGET_STATEGRID_METER_FLAGS_H
#define ND_TARGET_STATEGRID_METER_FLAGS_H

/* Relay command bits, mirrored from the legacy kernel flags while the
 * target migrates off nidavellir. Values are protocol constants. */
#define F_KeyDispay             0x04
#define F_KeyAwake              0x08

#define F_RemoteNull            0x00
#define F_RemoteRelayOn         0x01
#define F_RemoteRelayOff        0x02
#define F_RemoteAlarm           0x04
#define F_RemoteGaoJing0        0x08
#define F_RemoteGaoJing1        0x10
#define F_RemoteRelayOnAllow    0x20
#define F_RemoteRelayOnDirect   0x40

#define F_Password_En           0x0002
#define F_Crad_En               0x0008
#define F_RelayonAuto_En        0x0004
#define F_ExternRelay_En        0x0020

#define REMOTE_CMD_REALAY_OFF_STA \
    (F_RemoteRelayOff | F_RemoteGaoJing0 | F_RemoteGaoJing1)

#endif
