# PRD: ZW6B secondary and FTU system

**Status:** Draft 0.1, 2026-11-23. This is the merged PRD. It contains the 0.1 software requirements and the 0.2 hardware and performance additions.

**Product:** ZW6B 12 kV pole-mounted breaker, secondary and FTU system.

**Owner:** Secondary and software team.

**Reviewers:** Product, primary hardware, protection, quality, cybersecurity.

**Source poster:** product poster supplied by the requester (outside the repo).

**Standards source:** `docs/plans/zw6b-ftu-standards.md`.

**Goal.** Define the secondary and FTU system for the ZW6B so the product and engineering team share one testable scope. The poster, the standards matrix, the hardware document, and the performance document are the sources. Every number is a design target, a limit, a measured value, or TBD. A TBD is an open question, not a commitment.

## Document conventions

- Assumptions carry A-xx IDs.
- Open questions carry Q-xx IDs.
- Hardware requirements carry HR-xx IDs. Hardware parameter rows carry HP-xx, ENV-xx, HW-xx, SEN-xx, and ACT-xx IDs.
- Performance NFRs carry NFR-P-xx IDs. Software and system NFRs carry NFR-S-xx IDs.
- Evidence levels are E-A, E-B, E-C, and TBD. E-A means confirmed by official standard text or a type test. E-B means a confirmed standard number, name, and version, or a vendor drawing or datasheet. E-C means a poster, third-party catalog, or scope-level statement. TBD means no verified value.
- A TBD value cannot be used as a pass criterion.
- The PRD writes requirements and acceptance methods. It does not write the hardware design or the implementation.
- The hardware detail source is `docs/plans/zw6b-ftu-hardware.md`. It contains the architecture, interfaces, target parameters, and constraints. This PRD references it and does not duplicate its detail.
- The performance target source is `docs/plans/zw6b-ftu-performance.md`. It contains the target values, limits, measurement methods, and verification stages. This PRD references it and does not duplicate the implementation.
- P is a design target, L is an external or physical limit, M is a measured value with evidence, and TBD is unverified. A TBD cannot be a pass criterion.
- No clause-level limit from any standard is copied into this document. Purchase the standard texts and the target utility specification before compliance review.

## 1. Product scope and boundaries

### 1.1 Product statement

The ZW6B is a deeply integrated primary-secondary pole-mounted breaker. The poster states that Hexing (海兴) developed it with the China Electric Power Research Institute (中国电科院). It applies to 12 kV, 50 Hz three-phase overhead distribution line control and protection. The poster lists an optional automation interface and FTU, integrated measurement, protection, control, and communication, fault detection, isolation, alarm, line monitoring, non-fault section restoration, and operation in harsh environments.

The primary side interrupts the line. The secondary side supplies sensing, protection, control, communication, records, power, cybersecurity, and feeder automation. The complete-set standard baseline includes DL/T 2859-2024, the 12kV 一二次融合柱上开关 standard effective 2025-06-25. This PRD covers the secondary and FTU system, including its hardware requirements and its verifiable performance.

### 1.2 Product variants

- **Automation-ready breaker.** The breaker body includes sensors, actuator drive interfaces, power take-off, and the aviation connector, but no FTU. It is tested with a simulator and a reference FTU.
- **Breaker with FTU.** The full secondary system in this PRD.

Both variants use the same breaker body and the same interface contract. A utility can add the FTU in the field. The poster describes the FTU as optional, so the connector and the interface contract are product requirements even when the FTU is not fitted.

### 1.3 The three physical parts

The complete set (成套) contains three parts.

1. **Breaker body.** Vacuum interrupter, insulation, bushings, operating mechanism, integrated sensors, actuator, auxiliary contacts, and primary grounding.
2. **FTU.** Signal conditioning, acquisition, protection, control, communication, local HMI, power module, backup energy, and cybersecurity.
3. **Aviation connector harness (航插线束).** The detachable electrical and signal connection between the breaker body and the FTU.

### 1.4 Ownership

| Item | Primary team | Secondary team | Shared |
|---|---|---|---|
| Breaker body, interrupter, mechanism, insulation | Owns design and primary type tests | Consumes the interface contract | Protection coordination |
| Sensor cores and primary-side accuracy | Owns core, insulation, mounting | Owns signal conditioning, calibration, accuracy verification | Sensor interface and calibration procedure |
| Actuator and coils | Owns coil, actuator, mechanism, auxiliary contacts | Owns drive, timing, supervision, safe state | ICD-02 |
| Power take-off | Owns PT or CT source and insulation | Owns power module, backup, load budget | ICD-03 |
| Aviation connector and harness | Owns mechanical integration and sealing on the breaker | Owns pinout, harness definition, FTU-side connector | Mating interface, EMC, grounding, fit check |
| FTU enclosure and electronics | Not applicable | Owns design, build, and acceptance | Mounting and thermal interface |
| System integration | Provides breaker body and primary data | Provides FTU and test fixtures | One-secondary integration test and type test plan |

### 1.5 Out of scope

- Master-station software and the backhaul network.
- Billing metering and settlement certification. Measurement is for monitoring and protection.
- Primary insulation, interrupter, and mechanism design.
- Condition monitoring beyond the self-diagnosis in this PRD, such as partial discharge, arc fault, or dissolved gas analysis.
- More than one breaker per FTU. One FTU controls one breaker.
- Ring main unit, distribution terminal unit, or transformer terminal unit functions.
- Export grid codes and non-Chinese certification.
- Utility security operations staffing and incident response after handover.
- The detailed hardware design. That belongs in `docs/plans/zw6b-ftu-hardware.md`.
- The selection of performance target values. That belongs in `docs/plans/zw6b-ftu-performance.md`.
- Clause-level standard limits. Those belong in the purchased standard texts and the target utility specification.

### 1.6 Assumptions

| ID | Assumption |
|---|---|
| A-01 | The poster, `docs/plans/zw6b-ftu-standards.md`, `docs/plans/zw6b-ftu-hardware.md`, and `docs/plans/zw6b-ftu-performance.md` are the sources. No datasheet, interface control document, type test report, or standard text was read. |
| A-02 | Rated voltage is 12 kV. Nominal system voltage is 10 kV, 50 Hz, three-phase three-wire, with an ungrounded or resonant-grounded neutral. The hardware document records 12 kV and 50 Hz with evidence B. |
| A-03 | The hardware document lists 630 A, 20 kA, and mechanical life ≥10000 as design targets with evidence B from the poster and the standards matrix. Confirm them against Q/GDW 11253-2023 and DL/T 2859-2024. |
| A-04 | The hardware document proposes a protection CT at 5P10 as a starting point with an ALF that covers 20 kA, a metering CT at 0.5S, a core-balance zero-sequence CT, and a PT or capacitive divider at 10/0.22 kV or 10/0.1 kV. Final ratios, classes, and response times are TBD. |
| A-05 | The hardware document baseline is a Cortex-M4F or Cortex-M33 controller. The repository Cortex-M0 is not the main FTU controller. Final selection, memory, and operating system are TBD. The ROM budget exception is in ADR-028 and is not yet approved. |
| A-06 | The hardware document baseline is CT take-off plus PT or AC 220 V, a 24 V DC control bus, a LiFePO4 battery, and a supercapacitor. 48 V DC and AC 220 V are alternatives. Trip and close energy come from the supercapacitor or battery, not the line. Backup endurance is an installation matrix with 60 s, 4 h, 8 h, and 24 h options. The option is selected per installation class and utility requirement, not a single product value. |
| A-07 | The primary master protocol is DL/T 634.5104-2009 over Ethernet or 4G. DL/T 634.5101-2022 over RS-485 is the serial fallback. The applicable State Grid distribution automation protocol, commonly Q/GDW 1376.1, is required for the Chinese market. DNP3 and DL/T 860 are optional variants. Editions are unverified. |
| A-08 | Feeder automation modes are centralized and local. Distributed peer-to-peer automation is optional. One mode is active at a time. |
| A-09 | The hardware document targets -40 to +70 °C operating, -40 to +85 °C storage, ≤95% condensing humidity, altitude ≤2000 m, FTU IP65, and body and mated connector IP67. Final values follow the tender and the target province. |
| A-10 | The hardware document targets GB/T 17626.2 level 4, GB/T 17626.3 level 3, GB/T 17626.5 level 4, GB/T 17626.6 level 3, and GB/T 17626.8 level 4. EFT and other parts are TBD. |
| A-11 | The cybersecurity target level is TBD. The candidate references are IEC 62351 and GB/Z 25320. Chinese cryptographic algorithms SM2, SM3, and SM4 are optional. |
| A-12 | Time sources are BeiDou, GPS, PTP, NTP, and optional IRIG-B. The performance document targets SOE resolution 1 ms, GPS absolute error ≤1 ms, and master time error ≤10 ms. Standard limits are TBD. |
| A-13 | Measurement is for monitoring and protection, not billing settlement. No measurement certification is planned. |
| A-14 | The FTU is pole-mounted with the breaker and sees wind vibration, switching shock, and solar load. |
| A-15 | The maintenance interval is TBD. The FTU is a field-replaceable module. Battery or supercapacitor replacement is TBD. |
| A-16 | Firmware updates that affect protection may require a planned outage or a local connection. The policy is TBD. |
| A-17 | The primary team provides trip and close coils, auxiliary contacts, and sensor assemblies. The interface control documents are not written. |
| A-18 | Condition monitoring beyond self-diagnosis is out of scope. |
| A-19 | The first market is China. Export variants and other grid codes are out of scope for v1. |
| A-20 | One FTU controls one breaker. The FTU is not a distribution terminal unit or a ring main unit controller. |
| A-21 | Timing budgets are maintained in `docs/plans/zw6b-ftu-performance.md` with IDs such as T-P, T-F, T-S, T-B, and C. P values are design targets. L values are limits. TBD limits are not commitments. |
| A-22 | The performance document assumes a 256-entry SOE ring and 24 h buffering at the assumed event rate. Communication availability is TBD. |
| A-23 | The performance document targets a 4 KiB SOE ring with an 8 KiB limit, an 8 KiB fault record with a 16 KiB limit, 4 KiB parameters with an 8 KiB limit, 8 KiB logs with a 16 KiB limit, and 24 KiB external flash with a 32 KiB limit. |
| A-24 | The certification scope is TBD. It must come from the target utility and the standards matrix. |
| A-25 | `docs/plans/zw6b-ftu-hardware.md` is present and is the source for hardware architecture, interfaces, and target parameters. Its target values are design inputs, not standard limits. |
| A-26 | `docs/plans/zw6b-ftu-performance.md` is present and is the source for performance targets, limits, and verification. P values are design targets. L values are limits. TBD values are not commitments. |
| A-27 | Evidence levels follow the standards note A, B, and C, plus target, limit, measured, and TBD. |
| A-28 | No clause-level limit from any standard is written in this PRD. Purchase the standard texts and the target utility specification before compliance review. |

### 1.7 Standards baseline

This table summarizes `docs/plans/zw6b-ftu-standards.md`. The source document holds the full matrix, the source links, and the open issues. The evidence levels are the source document's levels.

| Standard or document | Version and status | Use in this PRD | Evidence |
|---|---|---|---|
| GB/T 11022-2020 | Current, MOD IEC 62271-1:2017 | Primary common requirements, service conditions, dielectric | A |
| GB/T 1984-2024 | Current, replaces GB/T 1984-2014 | Primary circuit breaker, mechanism, breaking and making | A |
| DL/T 402-2016 | Current, MOD IEC 62271-100:2008 | Primary ordering, test, and acceptance | A |
| Q/GDW 11253-2023 | Current, replaces Q/GDW 11253-2014 | 12 kV pole-mounted breaker ratings, selection, tests, interfaces | A |
| DL/T 2859-2024 | Current, 12kV 一二次融合柱上开关, effective 2025-06-25 | Complete-set main standard for the integrated pole-mounted switch | A |
| 国网设备〔2021〕151号 | State Grid document, not a standard | Integrated primary-secondary design, 26-pin aviation connector baseline, interface | A |
| IEC 62271-100 | International | Primary circuit breaker reference | A |
| IEC 62271-111 | Recloser reference | Reclosing function, if used | B |
| T/CES 127 | Group standard | Smart pole-mounted switch, if adopted | B |
| DL/T 721-2024 | Current, replaces DL/T 721-2013, effective 2024-11-24 | Distribution terminal requirements, tests, backup power, security | A |
| Q/GDW 10514-2018 | Current, replaces Q/GDW 514-2010 | Terminal and substation functions, performance, immunity | A |
| Q/GDW 11815-2023 | Current, replaces Q/GDW 11815-2018 | Terminal technical specification and acceptance; Appendix B/C defines the 26-pin aviation connector baseline | A |
| 中国电科院 2026 入网要求附件 4 | Grid-entry requirement annex | 26-pin aviation connector baseline | A/B |
| Q/GDW 11813 | 2018, update to confirm | Terminal parameter configuration | B |
| Q/GDW 10639 | Version to confirm | Terminal test specification | B |
| GB/T 13729-2019 | Current, replaces 2002 version | RTU technical requirements and tests | A |
| GB/T 7261-2016 | Current; 2026 version effective 2027-02-01 | Protection and automatic device test methods | A |
| DL/T 634.5101-2022 | Current, 2022 edition, replaces 2002 edition | Serial telecontrol protocol | A |
| DL/T 634.5104-2009 | Current | TCP/IP telecontrol protocol | A |
| DL/T 860 series | Current, several parts | IEC 61850 model and services, optional | A/B |
| DL/T 698.45-2017 | Current; 2026 version effective 2026-12-26 | Data exchange, if required | A |
| DL/T 1914-2018 | Current | 698.45 to 860 model mapping | A |
| IEC 62351 series | Current, several parts | Communication security | A |
| GB/Z 25320 series | Partial adoption | Domestic security guidance | B |
| GB/T 17626.2-2018, .3-2023, .5-2019, .6-2017, .8-2006 | Current | EMC immunity test methods | A |
| GB/T 4208-2017 | Current, 2024 amendment | IP code | A |
| GB/T 2423 series | Versions vary | Environmental tests | B |
| Q/GDW 1519 | Not the aviation connector standard | Do not cite for the connector. The 26-pin baseline comes from Q/GDW 11815-2023 Appendix B/C, the 2021 standardized design, and the CEPRI 2026 Annex 4. The 37-pin layout is an early proposal kept for provincial tenders that specify it. | The standards note confirms this |

Open issues from the standards note:

- Q/GDW 11813 and Q/GDW 10639 versions are unconfirmed.
- Q/GDW 1519 is not the aviation connector standard. The 26-pin baseline comes from Q/GDW 11815-2023 Appendix B/C, the 2021 standardized design, and the CEPRI 2026 Annex 4. The 37-pin layout remains a provincial variant.
- DL/T 860 distribution profiles and the Chinese adoption need confirmation.
- DL/T 698.45 2026 is a transition decision.
- GB/T 1984-2024 and DL/T 402-2016 use different IEC 62271-100 editions.
- GB/T 7261-2026 takes effect on 2027-02-01.
- Provincial detection notices differ.
- Most standard texts are paid or not public.

### 1.8 Secondary software architecture constraints

These constraints come from the company architecture rules and apply to the FTU firmware.

- Protection, measurement, control, automation, communication, records, power, and security are separate bounded contexts. Each context owns its model and rules.
- A context that needs another context defines the port it needs. Ports use an opaque context pointer and are bound once in the target composition root.
- Domain and application code do not depend on the platform, RTOS, or vendor SDK. Drivers and RTOS adapters live at the edge.
- No mutable global state. Ports and aggregates are injected at construction.
- Breaker state, protection settings, event log, and configuration have explicit aggregate roots and invariants. Illegal states are rejected at the boundary.
- CPU, RAM, flash, energy, and communication bandwidth are allocated per context, not shared implicitly.
- Each context is testable on the host with a fake port. Each product variant is one composition root.

### 1.9 Release success criteria

A v1 release succeeds when a utility can install and commission the FTU with a documented procedure and a repeatable test report. The control center can monitor, control, and receive fault events over the selected protocol. Protection operates correctly on the fault library and on staged field faults. The use cases in section 3 pass their acceptance suite. Hardware type tests and a field pilot complete with no critical failure. Every numeric target in section 6 is filled and approved, not TBD. These criteria are assumptions to confirm.

## 2. Personas and jobs

| Persona | Context | Jobs | Pains | Success signal | Main use cases |
|---|---|---|---|---|---|
| Distribution control center (DCC) operator | 24-hour control room, many feeders | Monitor, acknowledge alarms, operate remotely, isolate faults, restore healthy sections, report | Nuisance alarms, stale data, unclear fault location, slow communication | Correct feeder state quickly, first-time-right remote operation, clear fault report | UC-01, UC-02, UC-03, UC-04, UC-05, UC-08 |
| Field operator | Outdoor pole site, weather, gloves | Inspect, operate locally, verify isolation, reset, perform emergency operation | Unclear indicators, hidden interlocks, difficult access, ambiguity between local and remote mode | Local operation and state check with no ambiguity | UC-06, UC-08, UC-10 |
| Commissioning engineer | Site commissioning with laptop or PDA | Configure, calibrate, test sensors and actuator, point-to-point test, time sync, FA test, record evidence | Many parameters, protocol variants, missing test hooks, sensor calibration | First-time-right commissioning and a repeatable test report | UC-07, UC-08, UC-11, UC-12 |
| Asset and maintenance engineer | Fleet health and maintenance planning | Review self-test and logs, manage battery and firmware, find root cause, plan spares | No remote data, false alarms, configuration drift, unknown battery state | Remote diagnosis, planned maintenance, module swap repair | UC-08, UC-09, UC-10, UC-11 |
| Cybersecurity auditor | Compliance review and incident investigation | Review access, cryptography, audit logs, update integrity, network paths | Shared accounts, default passwords, unsigned firmware, missing logs | Evidence for every control and no critical finding | UC-13, UC-07, UC-11 |

## 3. Use cases

**UC-01 Normal monitoring.**
Actor: DCC operator. Trigger: periodic sampling and a master request.
Flow: 1. The FTU samples currents and voltages. 2. It computes measurements and quality flags. 3. It publishes to the master and updates the local HMI. 4. It records periodic values.
Exceptions: A sensor fault sets a quality flag and raises an alarm. A communication loss buffers the data.
Postcondition: The master and the local HMI show the current state with time and quality.
FRs: FR-09 to FR-14, FR-27 to FR-30, FR-37.

**UC-02 Fault detection and trip.**
Actor: FTU protection function. Trigger: phase current or zero-sequence current above a protection threshold.
Flow: 1. The FTU detects and classifies the fault. 2. It starts a fault record. 3. It checks blocking, the settings group, and the trip circuit. 4. It issues a trip output. 5. It confirms the breaker open from auxiliary contacts and current. 6. It applies reclose lockout or reclose per settings. 7. It reports the event and waveform.
Exceptions: A trip circuit failure raises an alarm and a breaker-failure alarm. A saturated sensor still trips on the backup stage. A missing auxiliary contact uses current-based confirmation.
Postcondition: The breaker is open, the fault is recorded, and the master has the event.
FRs: FR-01 to FR-08, FR-33 to FR-35, FR-42.

**UC-03 Fault isolation.**
Actor: DCC operator or local FA logic. Trigger: a fault trip and fault events from one or more FTUs.
Flow: 1. The master or local logic locates the fault section from fault currents, directions, and topology. 2. It selects the switches to open. 3. It checks authority, mode, and interlocks. 4. It opens the switches and confirms. 5. It locks out reclose in the fault section. 6. It reports the isolation result.
Exceptions: Communication loss falls back to local FA or manual isolation. Ambiguous location triggers conservative isolation. A failed operation raises an alarm and selects a backup.
Postcondition: The fault section is isolated and the healthy sections are identified.
FRs: FR-15 to FR-21, FR-22 to FR-26, FR-27 to FR-30.

**UC-04 Auto-restoration.**
Actor: local FA logic or DCC operator. Trigger: fault isolation is confirmed.
Flow: 1. The logic verifies the fault section is isolated. 2. It verifies voltage on the healthy section. 3. It checks no fault, no manual lock, and protection ready. 4. It closes the upstream breaker or the tie switch in the configured sequence. 5. It confirms restoration and reports. 6. It stops on a failed close.
Exceptions: Missing voltage blocks restoration. A fault reappears and trips with lockout. Communication loss uses local logic or holds. Manual mode blocks restoration.
Postcondition: Healthy sections are energized and the fault section stays isolated.
FRs: FR-16 to FR-21, FR-24, FR-27 to FR-30.

**UC-05 Remote control.**
Actor: DCC operator. Trigger: a need to open or close the breaker.
Flow: 1. The operator selects the device. 2. The FTU checks authority, mode, interlocks, protection, and actuator energy. 3. The operator confirms the select. 4. The FTU executes the command. 5. The FTU reports the result and final state. 6. The FTU logs the action.
Exceptions: An interlock blocks the command with a reason. A timeout cancels the command. Communication loss prevents execution. A fault during close causes a trip.
Postcondition: The breaker is in the commanded state or unchanged with a reason.
FRs: FR-22 to FR-26, FR-33, FR-48 to FR-51.

**UC-06 Local operation.**
Actor: field operator. Trigger: a local need to operate or inspect.
Flow: 1. The operator authenticates at the local HMI. 2. The FTU shows state, measurements, and interlocks. 3. The operator selects open or close. 4. The FTU asks for confirmation. 5. The FTU operates and shows the result. 6. The FTU logs the action.
Exceptions: Missing authority rejects the operation. An interlock rejects with a reason. An uncharged actuator waits or alarms. Manual mechanical operation is detected and logged.
Postcondition: The local operation is complete and remote control is blocked while local mode is active.
FRs: FR-23 to FR-26, FR-37 to FR-39.

**UC-07 Parameterization.**
Actor: commissioning engineer. Trigger: installation or a settings change.
Flow: 1. The engineer connects locally or remotely with the right role. 2. The engineer reads the current settings. 3. The engineer edits protection, communication, FA, and time settings. 4. The FTU validates ranges and dependencies. 5. The engineer applies and tests. 6. The FTU stores a version and an audit record.
Exceptions: An invalid value or combination is rejected. A power loss during write leaves the old or the new set atomically. An unauthorized write is rejected.
Postcondition: The new settings are active and traceable.
FRs: FR-07, FR-32, FR-49 to FR-51.

**UC-08 Event and record retrieval.**
Actor: DCC operator, maintenance engineer, or commissioning engineer. Trigger: a fault, an alarm, or an investigation.
Flow: 1. The user requests SOE, fault records, or logs. 2. The FTU returns time-stamped data. 3. The master or local tool displays and exports the data. 4. The user acknowledges the event.
Exceptions: A full log overwrites the oldest entry and raises an alarm. Invalid time is marked. A slow link uses chunked transfer. A corrupt record is rejected.
Postcondition: The record is available, time-aligned, and integrity-checked.
FRs: FR-33 to FR-36, FR-40 to FR-41, FR-44.

**UC-09 Communication loss.**
Actor: FTU and DCC. Trigger: heartbeat timeout or link failure.
Flow: 1. The FTU detects the loss. 2. It raises a local alarm and enters autonomous mode. 3. Protection and local FA continue. 4. It buffers events. 5. It retries the primary and backup channels. 6. On recovery, it resyncs time, uploads buffered events, and reports status.
Exceptions: A long loss wraps the buffer and raises an alarm. Both channels down leaves local mode. Time drift raises an alarm. A reconnect storm uses backoff.
Postcondition: Protection is unaffected and buffered data is recovered.
FRs: FR-30, FR-33 to FR-36, FR-40 to FR-41, FR-45 to FR-47.

**UC-10 Power loss and restart.**
Actor: FTU. Trigger: loss of primary power.
Flow: 1. The FTU detects the loss and switches to backup. 2. It saves state and logs. 3. It sheds nonessential loads. 4. It keeps protection ready if the backup allows. 5. It shuts down in order if the backup is low. 6. On power return, it boots, self-tests, restores settings, verifies the breaker state, resyncs time, and reports.
Exceptions: A low battery triggers a controlled shutdown. A brownout resets with safe outputs. Repeated power cycling raises an alarm. An unknown breaker state blocks remote close until verified.
Postcondition: The FTU returns to service without a spurious trip or close.
FRs: FR-45 to FR-47, FR-42 to FR-43.

**UC-11 Firmware upgrade.**
Actor: maintenance or commissioning engineer. Trigger: a security patch or a feature update.
Flow: 1. The engineer verifies the package signature and compatibility. 2. The FTU checks power, mode, and protection state. 3. It backs up settings. 4. It writes the inactive bank. 5. It verifies and switches. 6. It reboots, self-tests, and reports the version.
Exceptions: A bad signature or wrong model is rejected. A power loss keeps the old bank bootable. An incompatible version is rejected. A protection outage requires a local connection or a planned outage.
Postcondition: The new firmware is active or the old firmware is restored.
FRs: FR-48, FR-53 to FR-55, FR-51.

**UC-12 Time sync.**
Actor: FTU. Trigger: boot, a periodic interval, or a time source change.
Flow: 1. The FTU acquires time from BeiDou, GPS, PTP, or NTP. 2. It validates the time. 3. It disciplines the clock and time-stamps events. 4. It monitors drift and holdover. 5. It alarms on source loss or a time jump.
Exceptions: A spoofed or jumped time is rejected. Source loss uses holdover. No source requires manual time with an audit record.
Postcondition: Time is within the declared accuracy and logs are aligned.
FRs: FR-40 to FR-41, FR-33 to FR-35.

**UC-13 Cyber incident.**
Actor: cybersecurity auditor and FTU. Trigger: an unauthorized login, an integrity failure, abnormal traffic, or tampering.
Flow: 1. The FTU detects the event. 2. It blocks the session or the control path. 3. It logs and alarms. 4. It preserves evidence. 5. It notifies the auditor. 6. The team recovers with clean firmware or credentials and reviews the audit log.
Exceptions: A false positive is reviewed. An attacker with control triggers a local lockout. A key compromise triggers rekeying. A firmware tamper uses secure boot recovery.
Postcondition: The incident is contained and evidence is available.
FRs: FR-48 to FR-52, FR-33 to FR-36.

## 4. Functional requirements

### 4.1 Protection

**FR-01 Phase overcurrent protection.** The FTU shall provide at least two independent phase overcurrent stages. Each stage shall support definite-time and inverse-time curves. The high stage shall be settable to instantaneous. Acceptance: current injection at 0.5, 1.0, 2.0, 5.0, and 10.0 times the setting produces a trip decision inside the target in NFR-P-02, which is TBD. Settings survive 100 power cycles without change.

**FR-02 Zero-sequence ground fault protection.** The FTU shall provide zero-sequence overcurrent protection for ungrounded and resonant-grounded systems. The setting range shall cover the declared primary sensitivity. The function shall support alarm-only and trip modes. Acceptance: HIL injection of phase-to-ground faults across the declared current range produces the configured trip or alarm. The test runs in each grounding mode used by the pilot. The sensitivity is TBD and must be set from the performance document and the target grounding mode.

**FR-03 Voltage protection.** The FTU shall provide overvoltage, undervoltage, phase-loss, and PT-failure functions. Voltage-dependent functions shall block or use a safe default when the PT fails. Acceptance: injected voltage steps produce the configured alarm or trip within the declared time, TBD. A PT failure is detected within the declared time, TBD, and raises an alarm. The blocking behavior is verified.

**FR-04 Reclosing.** The FTU shall provide configurable reclosing with single-shot or multi-shot sequences, dead time, reclaim time, and lockout. Reclose shall be blocked after a downstream fault in demarcation mode. Acceptance: fault sequences verify reclose, lockout, and coordination with the upstream recloser. A persistent fault causes lockout after the configured number of shots. Manual close is blocked during lockout. The demarcation behavior is TBD and open in Q-04.

**FR-05 Fault detection and classification.** The FTU shall detect phase-to-phase, phase-to-ground, and downstream or upstream faults. It shall report the fault type, faulted phase, current magnitude, direction when the sensor supports it, and a timestamp. Acceptance: a HIL fault library with at least 20 cases produces the correct classification. Ambiguous cases are reported as unknown, not guessed. Directional cases are skipped when the sensor has no direction capability, as assumed in A-04.

**FR-06 Trip output and supervision.** The FTU shall drive the trip coil, supervise trip-circuit continuity, monitor actuator energy, and detect breaker failure. Acceptance: the trip pulse meets ICD-02. An open or short trip coil raises an alarm. Persistent current after a trip raises a breaker-failure alarm within the declared time, TBD. A communication failure never blocks a trip.

**FR-07 Protection settings.** The FTU shall store protection settings with ranges, units, dependencies, password protection, and versioning. Settings shall be non-volatile and atomic. Acceptance: invalid values and incompatible combinations are rejected with a reason. Settings survive 100 power cycles. An unauthorized write is rejected and logged. A version and audit record is created on every accepted change.

**FR-08 Fault records.** The FTU shall record pre-fault and post-fault currents and voltages, protection decisions, timestamps, and breaker status. The record shall be triggered by protection and by manual request. Acceptance: a fault produces a record with at least 10 cycles of pre-fault data and 20 cycles of post-fault data at the protection sampling rate in NFR-P-01, which is TBD. The record is complete, time-aligned, and retrievable.

### 4.2 Measurement

**FR-09 Electrical measurements.** The FTU shall measure three-phase current, voltage, active power, reactive power, power factor, frequency, and energy in both directions. Acceptance: a reference meter comparison at the declared points meets the accuracy target in NFR-P-06, which is TBD. Measurements reach the master within the target in NFR-P-08, which is TBD. The values carry timestamps and quality flags.

**FR-10 Zero-sequence measurements.** The FTU shall measure zero-sequence current and voltage. Acceptance: injection tests across the declared range meet the accuracy target in NFR-P-06. A missing or saturated zero-sequence sensor sets a quality flag and raises an alarm. The values are available locally and remotely.

**FR-11 Accuracy and calibration.** The FTU shall support factory calibration and a field calibration check. It shall detect sensor open circuits, short circuits, and drift. Calibration coefficients shall be protected. Acceptance: the calibration procedure restores the declared accuracy. A sensor fault raises a quality flag within the declared time, TBD. Coefficients survive power loss and unauthorized changes are rejected.

**FR-12 Data quality.** Every measurement and status point shall carry validity, stale, substituted, and test flags. Acceptance: a disconnected sensor, a stale value, and a test value are distinguishable at the master and the local HMI. A stale value is never presented as valid. Test values are not used by protection or automation.

**FR-13 Measurement presentation and update.** The FTU shall publish measurements periodically and on change. The local HMI shall update within the declared update time, TBD. Acceptance: the master receives the configured periodic and change-of-value reports. The timestamp comes from the synchronized clock. A communication loss does not stop local measurement.

**FR-14 Energy and demand monitoring.** The FTU shall provide energy registers and demand profiles for line monitoring. These values are not for billing settlement. Acceptance: energy accuracy meets the monitoring target in NFR-P-06. Registers survive power loss. The documentation states that the values are not billing-certified, per A-13.

### 4.3 Automation and feeder logic

**FR-15 Fault event reporting.** The FTU shall report fault current, fault type, direction when available, breaker status, and time to the master and to peers. Acceptance: a fault event reaches the master within NFR-P-03 or NFR-P-08, which are TBD, with the correct fields. The event is buffered during a communication loss. A duplicate report is suppressed.

**FR-16 Local feeder automation isolation.** The FTU shall support local FA logic that isolates a fault section from local and peer fault indications. The logic shall be configurable and shall use the configured topology. Acceptance: HIL tests with at least five feeder topologies isolate the correct section. No healthy section is isolated without a configured reason. Only one FA mode is active at a time.

**FR-17 Centralized feeder automation support.** The FTU shall execute remote isolation and restoration commands from the master with local checks and confirmation. Acceptance: a command that violates an interlock is rejected with a reason. A successful command is confirmed within NFR-P-04, which is TBD. The FTU reports the final state and the blocking reason.

**FR-18 Non-fault section restoration.** The FTU shall restore a healthy section only after fault isolation is confirmed, voltage is verified, and no manual lock or protection lockout is active. Acceptance: HIL scenarios verify restoration, blocking on missing voltage, blocking on a persistent fault, and stop on a failed close. The FTU never closes into an unverified fault. The restoration time target is in NFR-P-05, which is TBD.

**FR-19 Topology and peer configuration.** The FTU shall store feeder topology, device identifiers, peer addresses, and switch roles. Acceptance: an invalid topology is rejected. A configuration export and import round-trips without loss. The active version is reported to the master.

**FR-20 Feeder automation test mode.** The FTU shall support a simulation mode for fault and restoration logic without operating the primary. Acceptance: test mode cannot drive the trip or close outputs. Simulated events carry a test flag. Exiting test mode restores normal operation and logs the change.

**FR-21 Restoration blocking.** The FTU shall block automatic restoration in manual mode, during a communication loss per the configured policy, after a protection lockout, after a breaker-failure alarm, or when the breaker state is unknown. Acceptance: each blocking condition is exercised in HIL. The FTU reports the blocking reason to the master and the local HMI.

### 4.4 Control and interlock

**FR-22 Remote control.** The FTU shall support remote open and close with select-before-operate, authenticated authority, confirmation, timeout, and result reporting. Acceptance: an unselected command is rejected. A command from an unauthorized role is rejected. A command after the timeout is rejected. A successful operation reports the final state within NFR-P-08, which is TBD.

**FR-23 Local control.** The FTU shall support local open and close from the local HMI with authentication, clear state, and a local or remote mode selection. Acceptance: local operation works without the master. Remote commands are blocked while local mode is active. The operation is logged with the actor and result.

**FR-24 Interlocks.** Trip shall have priority over close. Close shall be blocked on a detected fault, protection lockout, manual lock, uncharged actuator, or mode conflict. Acceptance: each interlock is tested. The FTU never issues close and trip at the same time. The rejection reason is reported and logged.

**FR-25 Operation authority.** The FTU shall enforce role-based authority for remote control, local control, parameterization, and maintenance. Acceptance: a viewer cannot operate. A field operator cannot change protection settings. An engineer cannot bypass the cybersecurity policy. Every attempt is logged.

**FR-26 Emergency operation.** The FTU shall allow manual mechanical operation and shall detect and record the resulting state change. Acceptance: a manual trip or close is reflected within the declared time, TBD. The FTU does not fight the manual operation. The event is logged with time and source.

### 4.5 Communication protocols

**FR-27 DL/T 634.5104.** The FTU shall support DL/T 634.5104-2009 over TCP/IP on Ethernet and 4G. Acceptance: a conformance test against the declared profile passes. Spontaneous, cyclic, and general-interrogation data are correct. The link reconnects after a loss.

**FR-28 DL/T 634.5101.** The FTU shall support DL/T 634.5101-2022 over RS-485 as the serial fallback. Acceptance: a conformance test with the declared link settings passes. The FTU switches between 104 and 101 without losing buffered events.

**FR-29 Other protocols.** The FTU shall support the applicable State Grid distribution automation protocol, commonly Q/GDW 1376.1 with the edition to confirm, per A-07. DNP3 and DL/T 860 shall be product variants, not default code. Acceptance: each supported variant passes a protocol conformance test and a point-mapping test. The variant matrix is documented.

**FR-30 Communication supervision and buffering.** The FTU shall supervise the link with heartbeats, reconnect with backoff, and buffer events for store-and-forward. Acceptance: a link loss is detected within the declared time, TBD. Buffered events are uploaded on recovery without loss or duplication within the buffer capacity, which is TBD. A full buffer raises an alarm.

**FR-31 Local maintenance interface.** The FTU shall provide local maintenance over Ethernet, RS-485, RS-232, infrared, or Bluetooth. Access shall require authentication and the maintenance role. Acceptance: an unauthorized session is rejected. Maintenance actions are logged. The interface can be disabled by policy.

**FR-32 Configuration and point mapping.** The FTU shall support configuration of protocol parameters, point mapping, and the data model. Acceptance: a configuration export and import round-trips. A wrong mapping is detected by a validation test. The active configuration has a version and an audit record.

### 4.6 Logging and records

**FR-33 Sequence-of-events log.** The FTU shall record SOE entries with the resolution in NFR-P-07, which is TBD, and a synchronized timestamp. The log shall be non-volatile and circular. Acceptance: at least the capacity in A-23, which is TBD, is stored and retrieved in order. A full log overwrites the oldest entry and raises an alarm. Time accuracy meets NFR-P-07.

**FR-34 Operation and audit logs.** The FTU shall log remote and local operations, logins, settings changes, firmware updates, and security events. Acceptance: every control action has an actor, time, result, and reason. Logs cannot be edited through the HMI. An export preserves integrity with a checksum or signature.

**FR-35 Fault record retrieval.** The FTU shall allow local and remote retrieval of fault records and waveforms. Acceptance: retrieval completes within NFR-P-09, which is TBD. The record matches the trigger and has a checksum. A failed transfer can resume without duplicating data.

**FR-36 Storage and integrity.** The FTU shall manage storage capacity, retention, and integrity for logs and records. Acceptance: the declared capacity is met. The oldest data is overwritten only after an alarm. A power loss during a write does not corrupt existing records.

### 4.7 Local HMI

**FR-37 Local indicators.** The FTU shall provide local indicators for power, run, fault, communication, breaker position, local or remote mode, and battery. Acceptance: each state is unambiguous in sunlight and at night, with a target TBD from the usability test. A lamp test is available. A failed indicator is detectable by self-test.

**FR-38 Local menu.** The FTU shall provide a local menu for measurements, status, events, settings view, and operation. Acceptance: the menu is operable with gloves, with a target TBD from the usability test. The menu is available in the configured language. Critical actions require confirmation.

**FR-39 Local test.** The FTU shall provide local test functions for protection, outputs, and communication. Test mode shall isolate the primary outputs or use a test connector. Acceptance: test mode cannot trip or close the primary. Test results are logged and marked as test data.

### 4.8 Time sync

**FR-40 Time source.** The FTU shall synchronize to BeiDou, GPS, PTP, NTP, or IRIG-B. It shall select a source automatically and report the active source. Acceptance: each supported source is tested. Accuracy meets NFR-P-07, which is TBD. A source failure raises an alarm.

**FR-41 Holdover and drift.** The FTU shall hold time during a source loss and monitor drift. It shall reject a time jump above the configured threshold. Acceptance: after a source loss, drift stays within the declared limit, TBD. A time jump is rejected and logged. A manual time set requires the maintenance role and an audit record.

### 4.9 Self-diagnosis

**FR-42 Self-test.** The FTU shall run continuous self-tests on the CPU, memory, flash, analog-to-digital converter, sensors, trip circuit, actuator energy, power supply, communication, and storage. Acceptance: each fault is injected and detected within the declared time, TBD. The FTU reports the fault and enters the declared degraded mode.

**FR-43 Health and degraded mode.** The FTU shall report a health status and shall declare a degraded mode when a function is unavailable. Acceptance: the master sees the health object. Protection remains available or the FTU declares loss of protection. No failure is silent.

**FR-44 Watchdog and safe state.** The FTU shall use a watchdog, a safe output state, and boot-loop protection. Acceptance: a forced hang resets the FTU. Outputs stay in the safe state during reset. Repeated resets raise an alarm and block remote control.

### 4.10 Power management

**FR-45 Power supply and switchover.** The FTU shall take power from the PT or CT take-off and switch to backup energy without a reset. It shall not cause a spurious trip or close. Acceptance: input loss and return tests meet the target in NFR-P-11, which is TBD. The breaker state does not change. The switchover is logged.

**FR-46 Battery management.** The FTU shall manage battery or supercapacitor charging, temperature, health, low-power behavior, and replacement. Acceptance: backup health is reported. A low backup raises an alarm before shutdown. A backup swap does not lose settings or time beyond the holdover limit.

**FR-47 Power-loss restart.** The FTU shall save state, shut down in order, boot, self-test, restore settings, verify the breaker state, resync time, and report. Acceptance: the declared number of power cycles, TBD, causes no corruption. Boot to protection ready meets NFR-P-10, which is TBD. No spurious operation occurs. An unknown breaker state blocks remote close until verified.

### 4.11 Security

**FR-48 Secure boot and signed firmware.** The FTU shall verify firmware signatures at boot and before an update. It shall enforce anti-rollback. Acceptance: an unsigned or modified image is rejected. A rollback attempt is rejected. The failure is logged and alarmed.

**FR-49 Access control.** The FTU shall use unique accounts, role-based access, a password policy, session timeouts, and no default credentials. Acceptance: a default login fails. The role matrix is enforced. Failed attempts cause a lockout. Every access is logged.

**FR-50 Encryption.** The FTU shall protect remote sessions with TLS 1.2 or later and shall support an APN or VPN for 4G. Chinese cryptographic algorithms SM2, SM3, and SM4 shall be a product option, per A-11. Acceptance: a plaintext session is rejected. Cipher suites are configurable. A test tool cannot read the session.

**FR-51 Audit and tamper.** The FTU shall log security events and detect physical tampering. Acceptance: opening the enclosure or removing a seal is logged and alarmed. Logs are append-only from the device view. An export is signed or checksummed.

**FR-52 Vulnerability and update.** The FTU shall support security advisories, patching, and key revocation. Acceptance: a security update applies with the firmware update process. A compromised key can be revoked. The product has a documented vulnerability intake and response process.

### 4.12 Firmware update

**FR-53 Upgrade package.** The FTU shall accept only signed firmware packages with a model and version check. Acceptance: a bad signature, a wrong model, and an incompatible version are rejected. The active version is reported after an upgrade.

**FR-54 A/B banks and rollback.** The FTU shall use two firmware banks and shall roll back on a failed boot or a failed self-test. Acceptance: a power loss at each upgrade stage leaves a bootable image. The previous version can be restored. Settings are preserved or migrated.

**FR-55 Upgrade conditions.** The FTU shall allow an upgrade only with authorization, sufficient power, and a declared protection impact. Acceptance: an upgrade during a fault or on a low battery is blocked. A remote upgrade requires the declared role. The audit log records the operator, time, and result.

## 5. Hardware requirements

This chapter writes requirements and acceptance methods. It does not select a sensor, a connector, a power supply, or a battery. The hardware document owns those choices. The hardware document is `docs/plans/zw6b-ftu-hardware.md`. It is the source for the hardware architecture, interfaces, target parameters, and constraints. This chapter references it and does not duplicate its detail.

### 5.1 Hardware set and documentation

**HR-01 Complete set composition.** The delivered set shall consist of the breaker body, the FTU, and the aviation connector harness. The interface between them shall be defined in a controlled interface control document. Acceptance: inspection of the set, the interface document, and the labeling. The document shall identify every signal, pin, connector, cable, and grounding point. No signal shall depend on an undefined wire.

**HR-02 Aviation connector and harness.** The connector and harness shall meet the mechanical, electrical, and environmental requirements of the target utility's 一二次融合 specification. The baseline is a 26-pin connector defined by Q/GDW 11815-2023 Appendix B/C, the 2021 State Grid standardized design, and the CEPRI 2026 grid-entry requirement Annex 4. The 37-pin layout is an early proposal and remains a variant for provincial tenders that specify it. The exact pin assignment, keying, mating cycle rating, ingress protection, strain relief, bend radius, and cable length remain TBD. Sources are the standards matrix, `docs/plans/zw6b-ftu-hardware.md`, and the target utility's detection specification. Do not cite Q/GDW 1519 as the aviation connector standard. Acceptance: pinout continuity, dielectric test, mating cycle test, pull test, ingress protection test, and a fit check on the breaker body.

**HR-03 Hardware documentation and traceability.** The hardware design shall be documented in `docs/plans/zw6b-ftu-hardware.md` with a block diagram, BOM, schematics, connector pinout, power budget, thermal analysis, sensor and actuator specification, EMC and environmental test plan, and DFMEA. Acceptance: document review and a traceability check from each HR to a document section and a test. The hardware document is present. Its values are design inputs, not standard limits. This requirement stays open until the document is reviewed and its values are frozen.

### 5.2 Electrical and mechanical parameter register

Values marked TBD are not requirements yet. The team must fill them from the named standard text, the target utility specification, and the hardware document. No clause-level limit is copied into this PRD. The hardware document section 0 records the target parameters.

| ID | Parameter | Value or requirement | Source | Evidence | Acceptance |
|---|---|---|---|---|---|
| HP-01 | Rated voltage | 12 kV | Poster, hardware document section 0 | E-B | Nameplate and type test against Q/GDW 11253-2023 and DL/T 2859-2024 |
| HP-02 | Rated frequency | 50 Hz | Poster, hardware document section 0 | E-B | Nameplate and type test |
| HP-03 | Rated current | 630 A | Hardware document section 0, poster | E-B | Type test |
| HP-04 | Rated short-circuit breaking current | 20 kA | Hardware document section 0, poster | E-B | Type test |
| HP-05 | Peak withstand | TBD | Q/GDW 11253-2023, DL/T 2859-2024 | E-A standard, TBD value | Type test |
| HP-06 | Short-time withstand | TBD | Q/GDW 11253-2023, DL/T 2859-2024 | E-A standard, TBD value | Type test |
| HP-07 | Mechanical life | ≥10000 operations | Hardware document section 0, poster | E-B | Mechanical life type test |
| HP-08 | Electrical life | TBD | Q/GDW 11253-2023 | E-A standard, TBD value | Type test |
| HP-09 | Power-frequency withstand | TBD | GB/T 11022-2020 | E-A standard, TBD value | Dielectric type test |
| HP-10 | Lightning impulse withstand | TBD | GB/T 11022-2020 | E-A standard, TBD value | Impulse type test |
| HP-11 | Creepage distance and clearance | TBD | GB/T 11022-2020, target province | E-A standard, TBD value | Inspection and type test |
| HP-12 | IP rating | FTU IP65; body and mated connector IP67 | Hardware document section 7.1 | Target | IP type test |
| HP-13 | Operating and storage temperature | -40 to +70 °C operating; -40 to +85 °C storage | Hardware document section 7.2 | Target, to confirm in tender | Environmental type test |
| HP-14 | Humidity and altitude | ≤95% condensing; ≤2000 m | Hardware document section 7.2 | Target | Environmental type test or analysis |
| HP-15 | Pollution degree | TBD, per target province | GB/T 11022-2020 | E-A standard, TBD value | Inspection and type test |
| HP-16 | Salt spray and UV | TBD, per target province | GB/T 2423 series | E-B method, TBD value | Type test |
| HP-17 | Vibration and shock | GB/T 2423 series, transport and installation | Hardware document section 7.2 | E-B method | Type test |
| HP-18 | EMC immunity | GB/T 17626.2 level 4, .3 level 3, .5 level 4, .6 level 3, .8 level 4; other parts TBD | Hardware document section 7.4 | E-A method, target levels | Type test |
| HP-19 | Dimensions, weight, mounting | TBD | Hardware document sections 7.1 and 9.3 | TBD | Drawing review and fit check |
| HP-20 | Connector type, pin count, cable length, bend radius | 26-pin baseline per Q/GDW 11815-2023 Appendix B/C, 2021 standardized design, and CEPRI 2026 Annex 4. 37-pin variant for tenders that specify it. Pin assignment and cable details TBD. | Standards matrix, hardware document section 6, target detection specification | E-A for the baseline, TBD for pin assignment | Inspection, fit check, mating test |
| HP-21 | Control voltage | 24 V DC baseline; 48 V DC and AC 220 V alternatives | Hardware document sections 0 and 4.1; this PRD sets the 24 V baseline | Target | Bench test and type test |

The poster image I read shows only 12 kV and 50 Hz. The hardware document records 630 A, 20 kA, and mechanical life ≥10000 as design targets with evidence B from the poster and the standards matrix. They remain design targets until the primary team and the standard text confirm them. The values in this table are design inputs, not standard limits.

### 5.3 Sensor and actuator list

| ID | Item | Qty | Interface | Accuracy or response requirement | Source | Acceptance |
|---|---|---|---|---|---|---|
| SEN-01 | Protection CT | 3 | Analog | 5P10 starting, ALF must cover 20 kA, ratio TBD | Hardware document sections 3.1 and 3.2 | Calibration and type test |
| SEN-02 | Metering CT | 3 | Analog | 0.5S, ratio and secondary current TBD | Hardware document sections 3.1 and 3.2 | Calibration and type test |
| SEN-03 | Zero-sequence CT | 1 | Analog | Core balance, ratio and sensitivity TBD | Hardware document section 3.1 | Calibration and type test |
| SEN-04 | PT or capacitive divider | 3 | Analog | 10/0.22 kV or 10/0.1 kV, class TBD | Hardware document section 3.1 | Calibration and type test |
| SEN-05 | CT take-off winding | 1 | Analog | Separate from the protection CT, output TBD | Hardware document section 3.1 | Power sweep |
| SEN-06 | Temperature and humidity sensor | 1 | Digital | Range TBD | Hardware document section 3.1 | Calibration and test |
| SEN-07 | Position and energy feedback | 1 set | Aux contacts or limit switches | Timing TBD | Hardware document section 3.1 | Signal injection and timing test |
| ACT-01 | Trip coil | 1 | Drive from FTU | Voltage, current, pulse, and operate time TBD in ICD-02 | Primary team, hardware document | Pulse measurement and endurance test |
| ACT-02 | Close coil | 1 | Drive from FTU | Voltage, current, pulse, and operate time TBD in ICD-02 | Primary team, hardware document | Pulse measurement and endurance test |
| ACT-03 | Actuator energy | 1 | Monitor and control | Type and energy TBD | Primary team, hardware document | Charge time and energy test |
| ACT-04 | Breaker auxiliary contacts | 2 | Dry contact, 52a and 52b | Timing and bounce TBD | Primary team, ICD-02 | Signal injection and timing test |
| ACT-05 | Mechanism ready and spring charged | 1 or 2 | Dry contact | Timing TBD | Primary team, ICD-02 | Signal injection and timing test |

**HR-04 Sensor set.** The FTU shall interface with the protection CTs, metering CTs, zero-sequence CT, PT or capacitive divider, CT take-off winding, temperature and humidity sensor, and position and energy feedback. The sensor technology and channel count are in the hardware document section 3.1. The number of spare channels is TBD. Acceptance: interface inspection and a channel-by-channel calibration check.

**HR-05 Sensor accuracy and response.** The protection CT shall start at 5P10 with an ALF that covers 20 kA. The metering CT shall target 0.5S. The zero-sequence CT shall be core balance. The PT or capacitive divider shall be 10/0.22 kV or 10/0.1 kV. Final ratios, classes, burden, phase error, and response times are TBD. These values are design targets, not standard limits. Acceptance: calibration against a reference source, step response test, and type test. The test points and tolerances come from `docs/plans/zw6b-ftu-performance.md` and the standards.

**HR-06 Actuator drive.** The FTU shall drive the trip and close coils within the coil specification from the primary team. Coil voltage, current, pulse width, and operating time are TBD in ICD-02. The drive shall be isolated, short-circuit protected, and incapable of a spurious operation during reset. Acceptance: bench measurement of the pulse, a 100-operation endurance test, and a fault-injection test on the drive.

**HR-07 Actuator feedback and supervision.** The FTU shall monitor breaker position, mechanism ready, spring or capacitor energy, and trip-circuit continuity. The contact types and timing are TBD in ICD-02. Acceptance: signal injection for each state, a timing test against the actuator, and a broken-wire test.

### 5.4 Power architecture and power budget

The architecture is defined in the hardware document sections 4.1 to 4.4.

- CT energy harvesting from the line current.
- PT take-off or AC 220 V auxiliary input.
- 24 V DC control bus for control and actuator energy. 48 V DC and AC 220 V are alternatives.
- LiFePO4 battery plus supercapacitor for backup.

The trip and close energy comes from the supercapacitor or battery, not the line. The hardware document selects the topology and component values. The performance document holds the power and energy budgets.

**HR-08 Power source architecture.** The FTU shall accept power from a CT take-off, a PT take-off or AC 220 V auxiliary input, and a 24 V DC control bus with a LiFePO4 battery and a supercapacitor. 48 V DC and AC 220 V are alternatives. The source priority and thresholds are in the hardware document. The hardware document and this PRD both use 24 V DC as the baseline. The FTU shall switch sources without a reset and without a spurious trip or close. Acceptance: source emulation, switchover test, and a power-fail ride-through test.

**HR-09 CT energy harvesting.** The CT take-off winding shall be separate from the protection CT. It shall supply the FTU at the minimum line current declared for self-powered operation. The minimum current and output power are TBD and must come from the performance document and the hardware document. The FTU shall report when it is on auxiliary or backup power. Acceptance: a current source sweep from the minimum to the rated current, with a record of the operating envelope and the switchover points.

**HR-10 Auxiliary power and backup.** The backup shall be a LiFePO4 battery plus a supercapacitor. The battery management system shall provide cell balancing, overcurrent, overtemperature, undervoltage, and low-temperature charge inhibit. Backup endurance shall be selected from the installation matrix: 60 s, 4 h, 8 h, or 24 h. The option is per installation class and utility requirement, not a single product value. Capacity and replacement interval are TBD in the performance document. The backup shall be replaceable in the field and shall not lose settings. Acceptance: discharge test at the temperature extremes, a backup swap test, and a low-battery alarm test.

**HR-11 Power budget and thermal.** The hardware document shall contain a power budget per mode. Each load shall have a typical value, a peak value, and a duty cycle. The values are TBD. The thermal design shall keep every component inside its rating at the declared ambient and solar load. Acceptance: power analyzer measurement in each mode, a 24-hour profile, and a thermal chamber test. The PRD requires the budget and the evidence, not the implementation.

| Load | Mode | Typical | Peak | Duty | Source |
|---|---|---|---|---|---|
| MCU and logic | Protection ready | TBD | TBD | TBD | Performance document |
| ADC and signal conditioning | Protection ready | TBD | TBD | TBD | Performance document |
| Communication module | Communication active | TBD | TBD | TBD | Performance document |
| Local HMI | Local operation | TBD | TBD | TBD | Performance document |
| Actuator charging | Charge and operate | TBD | TBD | TBD | Performance document |
| Backup charging | Charge | TBD | TBD | TBD | Performance document |
| Heater or dehumidifier | Cold or humid | TBD | TBD | TBD | Performance document |
| Total | Each mode | TBD | TBD | TBD | Performance document |

### 5.5 Environment, EMC, and ingress protection

The target classes come from the hardware document section 7.2 and 7.4. The test methods come from the standards note. Final values follow the target utility specification and the purchased standard text.

| ID | Parameter | Requirement | Standard or method | Target | Evidence | Acceptance |
|---|---|---|---|---|---|---|
| ENV-01 | Operating temperature | -40 to +70 °C | GB/T 11022-2020 chapter 4, GB/T 2423 series | Target, to confirm in tender | E-A method | Type test |
| ENV-02 | Storage temperature | -40 to +85 °C | GB/T 2423 series | Target | E-B method | Type test |
| ENV-03 | Humidity | ≤95%, condensing allowed | GB/T 11022-2020 chapter 4 | Target | E-A method | Type test |
| ENV-04 | Altitude | ≤2000 m, derate above | GB/T 11022-2020 chapter 4 | Target | E-A method | Type test or analysis |
| ENV-05 | Pollution degree | TBD, per target province | GB/T 11022-2020 | TBD | E-A method | Inspection and type test |
| ENV-06 | IP rating | FTU IP65; body and mated connector IP67 | GB/T 4208-2017 | Target | E-A method | IP type test |
| ENV-07 | Salt spray | TBD, per target province | GB/T 2423 series | TBD | E-B method | Type test |
| ENV-08 | UV exposure | TBD, per target province | GB/T 2423 series | TBD | E-B method | Type test |
| ENV-09 | Vibration and shock | GB/T 2423 series, transport and installation | GB/T 2423 series; hardware document section 7.2 | TBD | E-B method | Type test |
| ENV-10 | EMC immunity | GB/T 17626.2 level 4, .3 level 3, .5 level 4, .6 level 3, .8 level 4 | Hardware document section 7.4 | Target levels | E-A method | Type test |
| ENV-11 | EMC emission | TBD | Applicable GB/T or DL/T standard | TBD | TBD | Type test |
| ENV-12 | Insulation and dielectric | Per GB/T 11022-2020 and Q/GDW 11253-2023 | GB/T 11022-2020, Q/GDW 11253-2023 | TBD value | E-A method | Type test |

**HR-12 Environmental classes.** The FTU and harness shall meet the environmental targets in the hardware document section 7: -40 to +70 °C operating, -40 to +85 °C storage, ≤95% condensing humidity, altitude ≤2000 m, FTU IP65, body and mated connector IP67. Final values follow the tender and the target province. Acceptance: type test at a recognized lab. No numeric standard limit is written in this PRD.

**HR-13 EMC classes.** The FTU shall meet the EMC target levels in the hardware document section 7.4: GB/T 17626.2 level 4, GB/T 17626.3 level 3, GB/T 17626.5 level 4, GB/T 17626.6 level 3, and GB/T 17626.8 level 4. EFT, voltage dips, and damped oscillatory are TBD. Acceptance: type test per the selected standards. The test report shall list each port and each level.

**HR-14 Ingress protection and corrosion.** The FTU enclosure, connectors, and harness shall meet the FTU IP65 and body and mated connector IP67 targets. Salt spray and UV are TBD per the target province. The test methods are GB/T 4208-2017 and the GB/T 2423 series. Acceptance: IP test, salt spray test, UV test, and a visual inspection after the tests.

### 5.6 Hardware interfaces

The connector set is partitioned into control, measurement, power, communication, and maintenance connectors. The connectors use different shells and keying to prevent cross-mating. A grounding pin is longer than the signal pins. The CT secondary uses a shorting pin and a make-before-break structure. Pin assignments are TBD until the official 26-pin drawing is available. The 37-pin early proposal is a provincial variant.

| ID | Interface | Connector | Signal | Electrical requirement | Protection | Acceptance |
|---|---|---|---|---|---|---|
| HW-01 | Aviation connector | 26-pin baseline, 37-pin variant | Power, analog, digital, communication, shield | Pinout, voltage, current, and isolation TBD | Keying, strain relief, IP, surge path | Continuity, dielectric, mating cycle, pull test |
| HW-02 | CT and PT analog inputs | Measurement connector | Protection CT, metering CT, zero-sequence CT, PT or capacitive divider, CT take-off winding | Levels, burden, common-mode range TBD | Shielded, isolated, surge-protected; CT secondary must not open; PT secondary must not short | Signal injection, insulation, surge, noise test |
| HW-03 | Analog inputs | TBD | 0 to 5 V or digital; 4 to 20 mA is not used | Range and accuracy TBD | Isolation and filtering TBD | Loop and accuracy test |
| HW-04 | Digital inputs | Control and measurement connectors | 52a, 52b, mechanism ready, spring charged, local or remote, tamper; 8 to 16 channels | 24 V dry or wet contact, opto-isolated, TVS and RC, debounce | Opto-isolation and surge protection | Channel injection and timing test |
| HW-05 | Digital outputs | Control connector | Trip, close, lockout, alarm; 4 to 8 relay outputs; 2 to 4 pulse outputs for metering | Relay dry contact, RC and MOV | Safe state, isolation, short-circuit protection | Pulse, endurance, safe-state test |
| HW-06 | RS-485 | Communication connector | Master serial fallback and local devices; 2 or 3 isolated ports | 2-wire half-duplex, baud, termination, isolation TBD | Surge and common-mode protection | Loopback, conformance, isolation, cable-length test |
| HW-07 | RS-232 | Maintenance connector | Local maintenance | Baud and connector TBD | Isolation TBD | Loopback and local session test |
| HW-08 | Ethernet | Communication connector | Master and local maintenance | 10 or 100 Mbit/s, RJ45, transformer isolation | Isolation and surge protection | Conformance and link-loss test |
| HW-09 | 4G or 5G | Communication connector and antenna | Wireless backhaul | Cat.1 baseline, Cat.4 optional, SMA, SIM or eSIM, APN or VPN TBD | Antenna protection and access control | Conformance, link-loss, antenna test |
| HW-10 | Infrared | Maintenance window | Local HMI or PDA | 38 kHz, protocol and range TBD | Authentication and physical access | Local session and unauthorized-access test |
| HW-11 | Local maintenance | Maintenance connector | USB or serial | Port type and access policy TBD; default off; cover or interlock | Authentication, disable policy | Session, audit, port-disable test |
| HW-12 | Power input | Power connector | CT take-off, PT or AC 220 V, 24 V DC bus, 48 V DC or AC 220 V alternative, battery, supercapacitor | Voltage, current, isolation TBD | Fusing, reverse polarity, surge, precharge | Source emulation, dielectric, bonding test |
| HW-13 | Grounding | Power connector and chassis | Protective earth and shield | Bonding resistance TBD | Bonding and corrosion protection | Bonding resistance measurement |

**HR-15 CT and PT analog inputs.** The FTU shall provide shielded, isolated, surge-protected inputs for the protection CTs, metering CTs, zero-sequence CT, PT or capacitive divider, and CT take-off winding. The CT secondary circuit shall not open, and the PT secondary circuit shall not short. The CT connector shall use a shorting pin and a make-before-break structure. The signal levels, burden, common-mode range, and protection are TBD. Acceptance: signal injection, insulation test, surge test, and a noise test with the breaker in operation.

**HR-16 DI and DO.** The FTU shall provide 8 to 16 opto-isolated digital inputs for the breaker aux contacts, mechanism ready, spring charged, local or remote mode, and tamper. It shall provide 4 to 8 relay outputs for trip, close, lockout, and alarm, plus 2 to 4 pulse outputs for metering. The 4 to 20 mA interface is not used. The voltage ranges, current ratings, debounce, and safe state are TBD. Acceptance: channel-by-channel injection, timing test, and a safe-state test during reset and power loss.

**HR-17 Serial interfaces.** The FTU shall provide two or three isolated RS-485 ports for the master serial fallback, local devices, and cascading. It shall provide RS-232 or USB for local maintenance. The baud rates, isolation, termination, and connector types are TBD. Acceptance: loopback and protocol conformance tests, an isolation test, and a cable-length test.

**HR-18 Ethernet and 4G.** The FTU shall provide 10/100 Ethernet with transformer isolation for the master and local maintenance. It shall provide a 4G module with Cat.1 as the cost baseline and Cat.4 as an option for larger files or higher throughput. The antenna uses SMA or N type, and the SIM or eSIM choice is TBD. Acceptance: protocol conformance over each medium, a link-loss test, and an antenna installation check.

**HR-19 IR and local maintenance.** The FTU shall provide a 38 kHz infrared interface and a USB or serial maintenance port for commissioning and field service. The maintenance port shall be off by default and enabled only after authentication or with the cover open. The protocol and physical access control are TBD. Acceptance: a local session with each tool, an unauthorized-access test, and a port-disable test.

**HR-20 Power and grounding interface.** The FTU shall define the power input from the CT take-off, the PT or AC 220 V auxiliary source, the 24 V DC control bus, the 48 V DC or AC 220 V alternative, the LiFePO4 battery, and the supercapacitor, and the protective earth connection. The voltage range, current, isolation, precharge, and bonding resistance are TBD. Acceptance: power source emulation, a dielectric test, and a bonding resistance measurement.

### 5.7 Hardware acceptance and type tests

**HR-21 Hardware acceptance plan.** The hardware shall pass incoming inspection, factory test, type test, system integration test, and field pilot. The type test items are dielectric, insulation resistance, temperature rise, short-circuit, mechanical life, IP, EMC, environmental, vibration, salt spray, UV, connector mating, and corrosion. The limits and methods come from DL/T 2859-2024, Q/GDW 11253-2023, DL/T 721-2024, Q/GDW 10514-2018, Q/GDW 11815-2023, Q/GDW 10639, GB/T 7261-2016, and the 151号 document. Acceptance: signed test reports and a traceability matrix. No clause-level limit is written in this PRD.

**HR-22 Hardware evidence set.** The acceptance package shall contain the incoming inspection report, the factory test report, the calibration certificate, the type test report, the system integration report, the field pilot report, and the hardware document review record. Acceptance: inspection of the evidence set against HR-21 and the traceability table.

**HR-23 Hardware configuration control.** The FTU, harness, and connector shall have a controlled part number, revision, and serial number. A change to the harness pinout, connector, sensor, actuator interface, or power architecture shall trigger a review of the interface control documents and a regression test. Acceptance: configuration audit and a change record.

**HR-24 Hardware production test.** Production shall test the power input, self-test, sensor channels, digital inputs and outputs, communication ports, local HMI, backup energy, and connector pinout. The test limits are TBD from the hardware document and the production engineering study. Acceptance: a production test procedure and a first-article report.

## 6. Non-functional requirements

### 6.1 Performance NFRs

These NFRs define the performance requirements. The target values, limits, measurement methods, and verification stages come from `docs/plans/zw6b-ftu-performance.md`. A value still marked TBD is an open question, not a commitment. The PRD sets the requirement and the acceptance method, not the implementation.

Method codes are defined once in section 9 and used in every traceability table. The set is A, I, T, L, S, H, Y, and F.

| ID | Metric | Target | Source | Acceptance method | Notes |
|---|---|---|---|---|---|
| NFR-P-01 | Sampling rate | 4 kHz per protection channel (250 µs, 80 points per 50 Hz cycle). Measurement chain rate TBD. | Performance document T-P1, section 3.1 | L, H | Count samples over 10 cycles at 50 Hz and at 0.9 to 1.1 times nominal frequency. Report rate and jitter. A missed sample is a failure. |
| NFR-P-02 | Protection operation time | Electronic chain from sample to trip coil ≤ 25 ms target. Criterion window 20 ms. Optional fast path ≤ 15 ms. Breaker mechanical time TBD. Standard limits TBD. | Performance document T-P4, T-P9, T-P10 | H, L, Y | Inject faults at 1.2, 2, 5, and 10 times setting. 100 events per point. Report minimum, median, maximum, and 95th percentile. System total adds the breaker mechanical time. |
| NFR-P-03 | FA fault detection time | Fault detection 20 ms. FA event enqueue ≤ 5 ms. Local encode and send ≤ 20 ms. Standard limits TBD. | Performance document T-F1 to T-F3 | H, S | 100 scenarios. Timestamp at injection and at the fault event. Report distribution. |
| NFR-P-04 | FA isolation time | Terminal isolation path ≤ 50 ms. Local FA end-to-end ≤ 200 ms plus switch time. Centralized end-to-end external TBD. Standard limits TBD. | Performance document T-F8, T-F10, T-F11 | H, S, F | 100 scenarios. Count from fault event to isolation confirmation. Report success rate and time distribution. |
| NFR-P-05 | Restoration time | Terminal restoration path ≤ 50 ms. Local FA end-to-end ≤ 200 ms plus switch time. Centralized end-to-end external TBD. Standard limits TBD. | Performance document T-F9, T-F10, T-F11 | H, S, F | 100 scenarios. Count from isolation to voltage restoration. Report success rate and time distribution. |
| NFR-P-06 | Measurement accuracy | Metering CT 0.5S. Protection CT 5P10 starting with ALF covering 20 kA. Final class and test points TBD. Standard limits TBD. | Hardware document sections 3.1 and 3.2; performance document accuracy budget | T, Y | Compare against a reference meter at the declared points and temperatures. Report error at each point. |
| NFR-P-07 | SOE resolution and time accuracy | SOE resolution 1 ms. Event edge to timestamp ≤ 100 µs. Event to master ≤ 1 s. GPS absolute error ≤ 1 ms. Master time error ≤ 10 ms. Standard limits TBD. | Performance document T-S1, T-S2, T-S7, section 6.4 | L, Y | Inject time-marked events. 100 events. Report the minimum resolvable interval and the absolute error. Without time sync, 1 ms is resolution, not absolute accuracy. |
| NFR-P-08 | Communication latency | Command receive to output ≤ 20 ms, limit ≤ 50 ms. Telemetry response ≤ 500 ms, limit ≤ 1 s. SOE to master ≤ 1 s. Command to master confirmation ≤ 1 s. Link failure detection ≤ 3 times the timeout. Reconnect ≤ 5 s. | Performance document section 3.5 | H, S | Timestamps at both ends. 100 messages. Report minimum, median, maximum, and loss. |
| NFR-P-09 | Communication throughput | 9600 8N1 is about 960 B/s. A 32 B frame ≤ 33 ms. 10 frames per second of 32 B is 320 B/s, 33 percent utilization. 104 SOE ≤ 1 s. 3 connections, maximum 4. Per-connection memory ≤ 2 KiB, total ≤ 8 KiB. | Performance document sections 6.1 to 6.3 | H, S | Protocol tester or network emulator. Sustained load for 1 hour. Report throughput, loss, and buffer behavior. |
| NFR-P-10 | Cold start time | Protection ready ≤ 150 ms, limit ≤ 250 ms. Local communication ready ≤ 250 ms, limit ≤ 500 ms. Master session ≤ 3 s after link, limit ≤ 5 s. Whole-unit communication available TBD. | Performance document section 3.4 | L, T | Power supply switch. 100 cycles at temperature extremes. Report minimum, median, maximum, and failures. Protection ready before local communication ready before remote link is an order invariant. |
| NFR-P-11 | Operating and standby power | Run ≤ 100 mA. Idle ≤ 10 mA. Sleep ≤ 200 µA. Shutdown leakage TBD. Module peak TBD. These are P targets, not commitments until hardware selection. | Performance document section 5.2 | T, L | Power analyzer. 24-hour profile. Report typical, peak, and energy per day. The communication module is recorded separately. |
| NFR-P-12 | Backup endurance | Backup endurance is an installation matrix, not a single value. The options are 60 s, 4 h, 8 h, and 24 h. The option is selected per installation class and utility requirement. CT energy at the minimum line current ≥ average consumption plus 2 times margin. Line current and cycle life TBD. | Performance document section 5.3; hardware document sections 4.3 and 4.6; installation matrix below | T, Y | Discharge test at low, nominal, and high temperature. Count operations. Report capacity, time, and end voltage. |
| NFR-P-13 | Availability | Annual availability, MTBF, and MTTR are TBD. Reliability prediction and field data must supply the values. | Performance document section 8; field data | A, F | Reliability prediction, field pilot data, and maintenance records. Report the calculation and the assumptions. |
| NFR-P-14 | Compute and memory | CPU average ≤ 50%, 10 ms peak ≤ 80%. Sample ISR WCET ≤ 125 µs. Protection task WCET ≤ 5 ms per tick. Communication task WCET ≤ 5 ms per tick. Interrupt latency ≤ 50 µs. Longest interrupt disable ≤ 20 µs. Task jitter ≤ 100 µs. ROM ≤ 92 KiB target and < 100 KiB platform limit. ADR-028 in `docs/plans/zw6b-ftu-adrs.md` records the per-product exception: FTU Phase 1 96 KiB, Phase 2 192 KiB, Phase 4 480 KiB, 1 MiB MCU, pending product and architecture approval. Until ADR-028 is approved, the platform default applies. RAM static ≤ 70%. Initial stack and RAM budget 11.5 KiB. SOE ring 4 KiB target, 8 KiB limit. Fault record 8 KiB target, 16 KiB limit. Parameters 4 KiB target, 8 KiB limit. Logs 8 KiB target, 16 KiB limit. External flash 24 KiB target, 32 KiB limit. | Performance document sections 4.1 to 4.3 | A, T, L | CI checks ROM, RAM, static stack frames, and allocation. WCET, ISR latency, CPU, and stack high-water need target bench and HIL. No dynamic allocation in the protection, interrupt, or communication paths. |

**Backup endurance installation matrix.** The matrix replaces a single backup-endurance number. The option is selected per installation class and the utility requirement. The load profiles are proposed and TBD until the site survey.

| Option | Intended installation class | Load profile during backup | Status |
|---|---|---|---|
| 60 s | Ride-through and safe shutdown on loss of line | Protection and save path only | Target |
| 4 h | Short standby | Protection and periodic communication | Target |
| 8 h | Extended standby | Protection and periodic communication | Target |
| 24 h | Long outage or remote site | Reduced load, communication duty cycle TBD | Target |
| Selection | Per installation class and utility requirement | TBD | Open |

The targets in the performance table above are P or L values from `docs/plans/zw6b-ftu-performance.md`. They remain provisional until product approval. A value that is not approved remains TBD for acceptance and cannot be used as a pass criterion.

**Performance acceptance gate.** No performance NFR is accepted until its target value, measurement point, test conditions, and pass criterion from `docs/plans/zw6b-ftu-performance.md` are approved by the product and protection engineers. A TBD value cannot be used as a pass criterion.

### 6.2 Software and system NFRs

**NFR-S-01 Maintainability.** The firmware shall be modular, with configuration export and import, remote diagnostics, and replaceable modules. Verification: design review, service drill, and field data.

**NFR-S-02 Testability.** The FTU shall provide HIL interfaces, test points, fault injection hooks, simulation mode, and a factory test fixture. Verification: test plan review and HIL readiness review.

**NFR-S-03 Safety.** No single fault shall cause a spurious trip or close. Trip circuits shall be supervised. Outputs shall be isolated and in a safe state during reset. Verification: analysis, fault injection, and type test.

**NFR-S-04 Usability.** The local HMI shall be readable in sunlight, operable with gloves, and labeled in the configured language. The exact targets are TBD from the usability test. Verification: field usability test.

**NFR-S-05 Documentation.** The project shall deliver an interface control document, a configuration guide, a test report, a cyber hardening guide, a maintenance manual, and the hardware document. Verification: inspection.

**NFR-S-06 Certification and compliance.** The product shall complete the applicable Chinese type tests, grid entry evaluation, protocol conformance, and cybersecurity assessment, per A-24. Verification: certification evidence.

**NFR-S-07 Architecture and resource allocation.** The secondary software shall follow the constraints in section 1.8. Accepted ports use an opaque context pointer and are bound once in the composition root. No mutable global state. Each context has a CPU, RAM, flash, energy, and bandwidth budget. Verification: architecture review, dependency check, host tests, and a resource budget table.

**NFR-S-08 Configuration and traceability.** Every configuration item and requirement shall have a version, an owner, and a verification record. A change to a hardware interface, a protection setting, or a communication profile shall trigger a review and a regression test. Verification: configuration audit and traceability review.

## 7. Interfaces

### 7.1 Hardware interfaces

The hardware interface requirements are in section 5.6, table HW-01 to HW-13. They cover the aviation connector, CT and PT analog inputs, analog inputs, digital inputs, digital outputs, RS-485, RS-232, Ethernet, 4G or 5G, infrared, local maintenance, power input, and grounding.

### 7.2 External communication and software interfaces

| Interface | Direction | Medium | Protocol | Key parameters | Source |
|---|---|---|---|---|---|
| Master telecontrol | Bidirectional | Ethernet or 4G | DL/T 634.5104-2009 | Port, common address, point map TBD | A-07, DL/T 634.5104-2009 |
| Master serial fallback | Bidirectional | RS-485 | DL/T 634.5101-2022 | Link settings and point map TBD | A-07, DL/T 634.5101-2022 |
| State Grid distribution automation | Bidirectional | Ethernet or serial | Q/GDW 1376.1, edition TBD | Profile and point map TBD | A-07, Q/GDW 1376.1 |
| Optional DNP3 | Bidirectional | Ethernet or serial | IEEE 1815 | Profile and point map TBD | A-07 |
| Optional IEC 61850 | Bidirectional | Ethernet | DL/T 860 series | Model and services TBD | A-07, DL/T 860 |
| Optional DL/T 698.45 | Bidirectional | Ethernet or serial | DL/T 698.45-2017 or 2026 | Model and profile TBD | Standards note |
| Time sync | Input | Antenna or network | BeiDou, GPS, PTP, NTP, IRIG-B | Accuracy and holdover TBD | A-12 |
| Firmware update | Bidirectional | Local port or master channel | Signed package, file transfer | Package format and signing TBD | FR-53 |
| Local maintenance | Bidirectional | Ethernet, RS-232, infrared, Bluetooth | TBD | Authentication and role TBD | FR-31 |
| Security | Bidirectional | All remote channels | TLS 1.2 or later, APN or VPN, optional SM algorithms | Cipher suites and keys TBD | A-11, FR-50 |

### 7.3 Interface control documents

| ID | Interface | Owner | Status |
|---|---|---|---|
| ICD-01 | Sensor interface and calibration | Shared | Not written |
| ICD-02 | Trip and close actuator, auxiliary contacts, mechanism ready | Shared | Not written |
| ICD-03 | Power take-off and backup | Shared | Not written |
| ICD-04 | Enclosure, harness, aviation connector, grounding | Shared | Not written |

## 8. Acceptance and certification

### 8.1 Verification stages

- Bench test: protection, measurement, control, HMI, power, calibration, serial and Ethernet interfaces.
- HIL: feeder model, fault injection, communication emulation, power loss, time sync, and security events.
- Type test: EMC, environmental, ingress protection, vibration, salt spray, UV, dielectric, temperature rise, short-circuit, and mechanical life.
- Conformance: DL/T 634.5104, DL/T 634.5101, the State Grid protocol, and optional DNP3 or DL/T 860.
- Security assessment: configuration audit, vulnerability test, and penetration test.
- System integration: breaker body, aviation connector harness, FTU, sensors, actuator, and one-secondary operation.
- Field pilot: sites and duration TBD, with staged faults where the utility allows.
- Certification: type test report, grid entry evaluation, protocol conformance report, and cybersecurity assessment report.

All durations and site counts are TBD.

### 8.2 Hardware acceptance path

The hardware path is incoming inspection, factory test, type test, system integration test, field pilot, and certification. The evidence set is in HR-22. The type test limits and methods are TBD from the standards and the target utility specification. No clause-level limit is written in this PRD.

### 8.3 Performance acceptance path

The performance path is bench measurement, logic analyzer, HIL, type test, and field pilot. Each NFR-P row names the method. The target value, measurement point, test conditions, and pass criterion come from `docs/plans/zw6b-ftu-performance.md`. A TBD value is not a pass criterion.

### 8.4 Release acceptance

- Every FR passes its acceptance criteria.
- Every HR passes its acceptance criteria.
- Every NFR-P target is filled, measured, and approved.
- Every NFR-S requirement is verified.
- The traceability table is complete.
- No critical or high defect is open.
- The field pilot has zero spurious operations and correct protection on every staged fault.
- The interface control documents are signed by both teams.
- The documentation set is complete.
- The certification plan is approved.

A requirement is not done until its evidence is recorded in the traceability table. Verification is the object of review, not the test plan.

### 8.5 Certification expectations

- Chinese type test at a recognized lab against DL/T 2859-2024 and the applicable standards.
- Grid entry evaluation for the target utility.
- Protocol conformance for the selected variants.
- Cybersecurity assessment at the target level.
- Measurement calibration if the utility requires it. Billing certification is out of scope, per A-13.
- Primary type tests are provided by the primary team.
- One-secondary integration evidence for the 26-pin aviation connector baseline and the actuator.

The lab, the exact standard editions, and the certification scope are open.

### 8.6 Open questions

**Q-01** What are the primary ratings, continuous current, short-circuit current, mechanical life, and actuator coil specification? Are 630 A, 20 kA, and mechanical life ≥10000 confirmed?

**Q-02** Which sensor technology is used, electronic current and voltage sensors or conventional transformers, and what is the interface? What is the round device in the poster?

**Q-03** What is the grounding system at the target utility, ungrounded, resonant grounded, or low-resistance grounded?

**Q-04** Is the device a sectionalizer, a demarcation breaker, or both?

**Q-05** Which feeder automation mode is required, centralized, local, or distributed?

**Q-06** Which master protocols and editions are required, including the State Grid protocol?

**Q-07** What is the cybersecurity target level, and is 国密 required?

**Q-08** Is backup energy a battery or a supercapacitor, and what backup time and operation count are required?

**Q-09** Is remote firmware update allowed, and does a protection update require an outage?

**Q-10** What are the actual environmental extremes, altitude, and salt spray class?

**Q-11** Which certification scope and lab are required?

**Q-12** Is measurement certification required, or is monitoring only?

**Q-13** Is a time source available at the pole, and which one?

**Q-14** What log and fault record capacity and retention are required?

**Q-15** What is the primary interface contract for coils, auxiliary contacts, sensors, and power take-off?

**Q-16** What communication medium is available at the pilot sites?

**Q-17** What is the production test and calibration strategy?

**Q-18** Are export variants or other grid codes required in the first release?

**Q-19** Who owns cyber incident response and signing keys after delivery?

**Q-20** What is the compute platform and operating system, and does it fit the power, memory, and cold-start budgets?

**Q-21** What are the hardware parameters in `docs/plans/zw6b-ftu-hardware.md`, including the BOM, schematics, connector pinout, power budget, and thermal analysis?

**Q-22** The aviation connector baseline is 26 pins per Q/GDW 11815-2023 Appendix B/C, the 2021 standardized design, and the CEPRI 2026 Annex 4. Which provincial tenders require the 37-pin early proposal? What is the exact pin assignment, keying, and mating cycle rating? Do not assume Q/GDW 1519.

**Q-23** What are the performance targets in `docs/plans/zw6b-ftu-performance.md` for sampling, protection, FA detection, isolation, restoration, accuracy, SOE, communication latency and throughput, cold start, power, backup, and availability?

**Q-24** What is the target utility's environmental, EMC, IP, and corrosion specification?

**Q-25** Which type test items and limits apply in the target province?

## 9. Traceability seed

Method codes used in every traceability table: A analysis; I inspection; T bench test and reference measurement; L logic analyzer or oscilloscope; S simulation counting; H HIL; Y type test; F field pilot. A requirement may use more than one code.

### 9.1 Functional requirements

| FR | Primary use case | Method | Evidence artifact |
|---|---|---|---|
| FR-01 | UC-02 | T, H | Bench protection report, HIL fault scenario |
| FR-02 | UC-02 | T, H, F | Bench ground-fault report, HIL, field ground-fault test |
| FR-03 | UC-02 | T, H | Bench voltage report, HIL scenario |
| FR-04 | UC-02, UC-04 | T, H, F | HIL reclose and lockout report |
| FR-05 | UC-02, UC-03 | H, F | HIL fault library report, field fault record |
| FR-06 | UC-02 | T, H | Bench trip and supervision report |
| FR-07 | UC-07 | T, I | Configuration and audit report |
| FR-08 | UC-02, UC-08 | T, H | Fault record retrieval report |
| FR-09 | UC-01 | T | Measurement calibration report |
| FR-10 | UC-01 | T, H | Zero-sequence measurement report |
| FR-11 | UC-07 | T | Calibration and sensor-fault report |
| FR-12 | UC-01, UC-09 | T, H | Data-quality report |
| FR-13 | UC-01 | T | Measurement update report |
| FR-14 | UC-01 | T | Energy monitoring report |
| FR-15 | UC-02, UC-03 | H, F | HIL event report, field event report |
| FR-16 | UC-03 | H, F | HIL topology report, field isolation report |
| FR-17 | UC-03 | H, F | HIL centralized FA report |
| FR-18 | UC-04 | H, F | HIL restoration report, field restoration report |
| FR-19 | UC-07 | I, T | Configuration round-trip report |
| FR-20 | UC-03, UC-04 | T, H | Test-mode isolation report |
| FR-21 | UC-04 | T, H | Blocking-condition report |
| FR-22 | UC-05 | T, H, F | Control conformance report |
| FR-23 | UC-06 | T, F | Local operation report |
| FR-24 | UC-05, UC-06 | T, H | Interlock test report |
| FR-25 | UC-05, UC-06, UC-07 | T, I | Role matrix report |
| FR-26 | UC-06 | T, F | Emergency operation report |
| FR-27 | UC-01, UC-05 | T, H | DL/T 634.5104 conformance report |
| FR-28 | UC-01, UC-05 | T | DL/T 634.5101 conformance report |
| FR-29 | UC-01, UC-05 | T | Variant conformance report |
| FR-30 | UC-09 | H, F | Communication-loss report |
| FR-31 | UC-07 | T, I | Maintenance access report |
| FR-32 | UC-07 | T, I | Configuration mapping report |
| FR-33 | UC-08 | T, H | SOE report |
| FR-34 | UC-05, UC-06, UC-07 | T, I | Audit log report |
| FR-35 | UC-08 | T, H | Record retrieval report |
| FR-36 | UC-08 | T, H | Storage integrity report |
| FR-37 | UC-06 | T, I | HMI indicator report |
| FR-38 | UC-06, UC-07 | T, F | HMI usability report |
| FR-39 | UC-07 | T | Local test report |
| FR-40 | UC-12 | T, F | Time source report |
| FR-41 | UC-12 | T | Holdover and drift report |
| FR-42 | UC-10, UC-13 | T, H | Self-test report |
| FR-43 | UC-09, UC-10 | T, H | Health and degraded-mode report |
| FR-44 | UC-10 | T, H | Watchdog and safe-state report |
| FR-45 | UC-10 | T, H, F | Power switchover report |
| FR-46 | UC-10 | T, F | Backup management report |
| FR-47 | UC-10 | T, H | Power-cycle report |
| FR-48 | UC-11, UC-13 | T, I | Secure boot report |
| FR-49 | UC-05, UC-06, UC-13 | T, I | Access-control report |
| FR-50 | UC-05, UC-13 | T, I | Encryption report |
| FR-51 | UC-13 | T, I | Audit and tamper report |
| FR-52 | UC-13 | I, T | Vulnerability process report |
| FR-53 | UC-11 | T | Upgrade package report |
| FR-54 | UC-11 | T, H | Rollback report |
| FR-55 | UC-11 | T, I | Upgrade condition report |

### 9.2 Hardware requirements

| HR | Method | Evidence |
|---|---|---|
| HR-01 | I | Complete set inspection and interface control document |
| HR-02 | I, T, Y | Aviation connector pinout, dielectric, mating cycle, pull, and IP reports |
| HR-03 | I | Hardware document review and traceability matrix |
| HR-04 | T, I | Sensor channel interface inspection and calibration report |
| HR-05 | T, Y | Sensor accuracy, step response, and type test report |
| HR-06 | T | Actuator drive pulse, endurance, and fault-injection report |
| HR-07 | T | Actuator feedback, timing, and broken-wire report |
| HR-08 | T, H | Power source emulation, switchover, and ride-through report |
| HR-09 | T | CT source sweep and operating envelope report |
| HR-10 | T | Backup discharge, swap, and low-battery report |
| HR-11 | T, Y | Power budget, thermal chamber, and temperature profile report |
| HR-12 | Y | Environmental type test report |
| HR-13 | Y | EMC type test report |
| HR-14 | Y | IP, salt spray, and UV type test report |
| HR-15 | T, Y | CT and PT analog input signal, insulation, and surge report |
| HR-16 | T, H | DI and DO injection, timing, and safe-state report |
| HR-17 | T | RS-485 and RS-232 loopback, conformance, and isolation report |
| HR-18 | T | Ethernet and 4G conformance, link-loss, and antenna report |
| HR-19 | T, I | Infrared and local maintenance session and access report |
| HR-20 | T, I | Power input, dielectric, and bonding report |
| HR-21 | I, T, Y, H, F | Incoming, factory, type, integration, and field pilot reports |
| HR-22 | I | Evidence set inspection |
| HR-23 | I | Configuration audit and change record |
| HR-24 | T, I | Production test procedure and first-article report |

### 9.3 Performance NFRs

| NFR-P | Method | Evidence |
|---|---|---|
| NFR-P-01 | L, H | Sampling rate and jitter report |
| NFR-P-02 | H, L, Y | Protection timing report, 100 events per point |
| NFR-P-03 | H, S | FA detection report, 100 scenarios |
| NFR-P-04 | H, S, F | FA isolation report, 100 scenarios |
| NFR-P-05 | H, S, F | Restoration report, 100 scenarios |
| NFR-P-06 | T, Y | Accuracy report |
| NFR-P-07 | L, Y | SOE resolution and time accuracy report |
| NFR-P-08 | H, S | Communication latency report |
| NFR-P-09 | H, S | Communication throughput report |
| NFR-P-10 | L, T | Cold start report, 100 cycles |
| NFR-P-11 | T, L | Power profile report |
| NFR-P-12 | T, Y | Backup endurance report |
| NFR-P-13 | A, F | Reliability prediction and field data |
| NFR-P-14 | A, T, L | Compute, memory, stack, and allocation budget report |

### 9.4 Software and system NFRs

| NFR-S | Method | Evidence |
|---|---|---|
| NFR-S-01 | I, F | Maintainability review and service drill |
| NFR-S-02 | I, T | Test readiness review |
| NFR-S-03 | A, T, H | Safety and fault-injection report |
| NFR-S-04 | T, F | Usability report |
| NFR-S-05 | I | Documentation review |
| NFR-S-06 | I | Certification evidence |
| NFR-S-07 | A, I, T | Architecture review and resource budget |
| NFR-S-08 | I, A | Configuration audit and traceability review |

## 10. Evidence limits and source status

- I read the product poster supplied by the requester. It shows 12 kV and 50 Hz and the product claims quoted in section 1.1. It does not show the internal hardware, the connector, or the performance targets.
- I read `docs/plans/zw6b-ftu-standards.md`. It confirms the standard numbers, names, and versions listed in section 1.7 at evidence level A or B. It does not contain clause-level limits. It records that Q/GDW 1519 is not the aviation connector standard.
- The standards matrix and the verified research supply DL/T 634.5101-2022, the 2022 edition that replaces 2002, DL/T 2859-2024, the 12kV 一二次融合柱上开关 standard effective 2025-06-25, and the 26-pin aviation connector baseline from Q/GDW 11815-2023 Appendix B/C, the 2021 standardized design, and the CEPRI 2026 Annex 4. The 37-pin layout is an early proposal kept for provincial tenders.
- I read `docs/plans/zw6b-ftu-hardware.md`. It supplies the hardware architecture, interfaces, target parameters, and constraints. Its target values are design inputs, not standard limits. This PRD references that document and does not duplicate its detail.
- I read `docs/plans/zw6b-ftu-performance.md`. It supplies the performance targets, limits, measurement methods, and verification stages. P values are design targets, L values are limits, and TBD values are not commitments. This PRD references that document and does not duplicate its implementation.
- I did not read any standard text. I did not invent a clause-level limit or a vendor parameter.
- The only repository file written for this PRD is `docs/plans/zw6b-ftu-prd.md`. No other file was modified. No git add or commit was run.
