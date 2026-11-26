# ZW6B 一二次融合 FTU 架构决策记录（ADR 集）

**Status.** Draft 0.1, 2026-11-23.

**Scope.** 本文覆盖 ZW6B 12 kV、50 Hz 三相交流配网架空线路一二次融合成套柱上断路器的二次侧 FTU，包括硬件、软件、软硬件接口和性能。ADR-001 至 ADR-014 是产品与软件决策。ADR-015 至 ADR-021 是硬件决策，对应 `docs/plans/zw6b-ftu-hardware.md` 第 10 节的 HW-001 至 HW-010。ADR-022 至 ADR-027 是性能决策。ADR-028 是平台预算决策。

**Source of truth.**
- 标准号、名称、版本和证据等级以 [zw6b-ftu-standards.md](zw6b-ftu-standards.md) 为准。本文不编造条款级限值。
- 硬件架构、接口、器件类别、目标参数和约束以 [zw6b-ftu-hardware.md](zw6b-ftu-hardware.md) 为准。本文引用其第 10 节。
- 性能数值以 [zw6b-ftu-performance.md](zw6b-ftu-performance.md) 为准。该文件已于 2026-11-23 生成。本文采用其中的 P/L/M/TBD 预算、测量方法、阶段门禁和回归阈值。本文不再保留旧的 provisional 预算。
- 2026-11-23 研究更新：DL/T 634.5101 现行版为 2022 版，不是 2009 版。DL/T 2859-2024《12kV 一二次融合柱上开关》于 2025-06-25 实施，是成套主标准。航插基线是 Q/GDW 11815-2023 附录 B/C、国网设备〔2021〕151号和中国电科院 2026 入网要求附件 4 的 26 芯。37 芯是早期提案。Q/GDW 1519 不是航插标准。
- 实施阶段、板卡组、HSI 里程碑和仓库建议以 [zw6b-ftu-implementation-plan.md](zw6b-ftu-implementation-plan.md) 为准。

**Evidence legend.**
- [V] verified in this repository or in the product image.
- [A] standard number, name, and status confirmed in the standards matrix.
- [B] third-party, vendor, or poster source, pending official confirmation.
- [C] standard scope only. The clause and the limit were not read.
- [E] engineering estimate or design target. It needs a measurement or a product decision.
- [目标] design target in the hardware doc, not a standard limit and not an acceptance value.
- [TBD] not confirmed.

The standards matrix's A/B/C levels apply to every standard citation. The hardware doc's evidence levels apply to every hardware target. No clause-level limit is invented.

**Product reading.** The slide is titled 什么是一二次深度融合成套柱上断路器 [V image]. It says ZW6B 成套柱上断路器 is a new-generation outdoor 一二次融合成套设备 developed by 海兴 with 中国电科院, for 12 kV, 50 Hz three-phase overhead distribution line control and protection [V image]. The listed functions are an optional automation interface and FTU intelligent controller; fault detection, isolation, alarm, line monitoring, and automatic restoration of non-faulted sections; and integrated measurement, protection, control, and communication modules for frequent-operation and harsh environments [V image]. The image also shows a sealed pole breaker and a round sensor or CT assembly. The sensor type is not labeled, so the interpretation is [E].

The poster values 630 A, 20 kA, and mechanical life ≥10000 are vendor or typical values per the standards matrix [V standards matrix]. They are not acceptance values until Q/GDW 11253-2023 and 国网设备〔2021〕151号 are checked [C].

**Reuse judgment.** `modular-001` is a componentized C monorepo for meters and communication modules [V `docs/monorepo.md`]. Its layer rules, consumer-defined ports, component manifests, `product.yaml`, composition roots, host strict tests, formal checks, and record-storage design fit the FTU's business layer [V `docs/platform-layout.md`, `docs/guides/port-binding.md`, `docs/guides/component-standard.md`, `docs/design/record-retrieval.md`]. Its Cortex-M0, 103 KiB ROM, single-phase meter domain, and 698/645 main-protocol assumptions do not fit. The implementation plan recommends the same repository with a new product and target [V implementation plan section 12]. ADR-013 records that decision and its split conditions.

**ADR index.**

| ADR | Decision | Evidence |
| --- | --- | --- |
| 001 | One FTU firmware image with a protection partition and a communication partition. Five bounded contexts. Metering AFE is in hardware; billing approval is separate. | V image, V hardware doc, V implementation plan |
| 002 | Single Cortex-M4F or Cortex-M33 main controller. External window watchdog, voltage monitor, hardware overcurrent comparator, hardwired anti-pump. No lockstep baseline. | V hardware doc, A standards, E safety case |
| 003 | FreeRTOS for communication and management. Protection in the highest-priority ADC ISR and a small main loop. MPU or TrustZone isolation. | V hardware doc, V repo pattern, E |
| 004 | Independent protection and metering sampling chains. Protection element table. DFT and RMS. Internal ADC only if simultaneity and ENOB pass. | V hardware doc, V implementation plan, A test standards |
| 005 | 24 V bus baseline, 48 V alternative. CT plus PT or AC 220 V harvesting. LiFePO4 plus supercap. Protection energy independent of line harvesting. | V hardware doc, A standards, E numbers |
| 006 | 104 over 4G or Ethernet, 101 over RS-485, 61850 MMS and GOOSE over Ethernet. 698.45 and 645 only on the metering channel. MQTT diagnostics only. | V hardware doc, V implementation plan, A/B standards |
| 007 | Local voltage-time baseline, distributed GOOSE premium, centralized for monitoring. | V image, A standards, E timing |
| 008 | EEPROM for calibration and parameters, SPI NOR for events and waveforms, FRAM optional. Reuse the modular-001 block store and ring. | V hardware doc, V repo, B COMTRADE |
| 009 | LEDs, buttons, hardware local/remote keyswitch, 38 kHz IR, USB or serial maintenance. No pole LCD. No BLE baseline. | V hardware doc, A IP standards |
| 010 | ESAM plus secure element or TrustZone, secure boot, signed firmware, TLS, RBAC, audit, debug lock. | V hardware doc, A/B standards |
| 011 | A/B slots or external staging, signed manifest, trial boot, rollback, anti-rollback. Protection-affecting update only in a maintenance window. | V hardware doc, V implementation plan, E |
| 012 | Host tests, device HIL, system HIL, certification, field pilot. HIL-only metrics are listed. | V implementation plan, A/B standards |
| 013 | Same monorepo, `products/ftu_zw6b` and `targets/ftu_zw6b`, five new bounded contexts, new SoC and drivers. Split only under named conditions. | V repo, V implementation plan |
| 014 | 101/104 time sync baseline. GNSS 1PPS optional for 1 ms SOE. PTP optional. Honest time-quality flags. | V hardware doc, V implementation plan, B/C time standards |
| 015 | Spring mechanism baseline. Permanent magnet only if the tender specifies it and capacitor health is monitored. | V hardware doc HW-001, A standards |
| 016 | Protection and metering CT separate. Protection CT ALF must cover 20 kA. Zero-sequence core-balance CT. PT or capacitive divider. | V hardware doc HW-002, A standards, C limits |
| 017 | 24 V bus baseline, 48 V alternative, dual harvesting, LiFePO4 plus supercap, BMS, heater, OR-ing and protection. | V hardware doc HW-003/HW-008, A standards |
| 018 | Single M4F or M33, not the repo M0. External three-phase 0.5S metering AFE. Dual-core only for Linux or rich HMI. | V hardware doc HW-004, V implementation plan |
| 019 | Isolated analog front end, four supply domains, single-point shield ground, EMC target levels. | V hardware doc, A/B standards, C limits |
| 020 | Primary 一二次融合 interface is the 26-pin aviation connector from Q/GDW 11815-2023 Annex B/C, 2021 standardized design, and CEPRI 2026 Annex 4. 37-pin is an early proposal. Q/GDW 1519 is not the connector standard. | V research update, V hardware doc HW-006, A 151号, C pinout |
| 021 | 304 stainless or aluminum enclosure, sun shield, FTU IP65, body and mated connector IP67, -40 to +70 ℃ target. | V hardware doc, A standards, C levels |
| 022 | ADC end-of-conversion ISR runs protection. No RTOS in the path. 12.8 kHz, ISR WCET ≤39 µs, trip ≤10 ms target and ≤20 ms limit. | V performance doc, E |
| 023 | Fixed-priority preemptive RTOS. GOOSE highest. Priority inheritance on shared buses. CPU average ≤50%, worst-case ≤70%, protection and comm WCET ≤5 ms per tick. | V performance doc, E |
| 024 | Zero heap in protection. Fixed pools in communication. Phase 1 96 KiB Flash and 24 KiB RAM; Phase 2 192 KiB and 64 KiB. | V performance doc, E |
| 025 | GOOSE on a dedicated high-priority path. 104 and MMS below. 4G is an unreliable transport. 101 32 B frame ≤33 ms; 104 SOE and command ≤1 s; GOOSE <10 ms. | V performance doc, E |
| 026 | Armed low-power protection front end. CT or PT threshold wake. 4G duty-cycled. Terminal body ≤100 mA at 24 V DC (2.4 W); whole machine ≤10 W target and ≤30 VA limit; sleep ≤200 µA; wake ≤2 ms. | V performance doc, V hardware doc targets |
| 027 | Protection self-test and arming before close. Communication boots independently. Protection ready ≤150 ms target and ≤250 ms limit; local communication ready ≤250 ms; cold start to link ≤30 s. | V performance doc, E |
| 028 | Replace the single 100K ROM rule with per-product budgets: IDIS 103 KiB, stategrid grandfathered, FTU Phase 1 96K, Phase 2 192K, Phase 4 480K, 1 MiB/256 KiB MCU. Requires product and architecture approval. Not yet approved. | V CLAUDE.local.md, V performance doc, V implementation plan, V stategrid text size |

---

## Product and software ADRs

### ADR-001 Primary and secondary scope, and the software boundary

**Status.** proposed, 2026-11-23.

**Context.** ZW6B is sold as an integrated set. The primary side is the vacuum interrupter, insulation, operating mechanism, and embedded sensors. The secondary side is the FTU boards, the embedded software, the backup power, and the local maintenance interface [V hardware doc section 1]. The software boundary must say what the FTU firmware owns, what is configuration, and what is outside the product. The buyer is a utility with a master station, a protection philosophy, and a certification process. The product must fit a legacy 101/104 master and a newer 61850 master. The hardware doc defines the board set as main board, metering AFE board, power board, communication board, backplane or connector board, and debug or maintenance board [V implementation plan section 4.2].

**Decision drivers.** Safety and certification scope. Deterministic trip. Update isolation. Variant management for the optional FTU. BOM cost. Reuse of the monorepo. Legal metrology boundary.

**Options considered.**
1. Sensor interface only. The firmware reads sensors and publishes. Protection and FA live in the master or a separate relay. Lowest certification burden. The product loses the protection claim and cannot trip autonomously.
2. FTU-only firmware. The firmware owns protection, control, measurement, feeder automation, fault recording, power management, device control, records, local access, security, and update. Billing approval is separate.
3. Full secondary including billing certification. Adds legal metrology, sealing, calibration approval, and a separate certification path. It conflicts with a fast protection release cycle.
4. Two independent firmware images, one protection and one communication. Strong isolation, but the hardware doc's baseline is one main controller, so the images would share the same MCU and the same reset domain.

**Decision.** Option 2. One FTU firmware image on one main controller. The image has two internal partitions with a frozen interface. The protection partition owns sampling, protection, trip and close decisions, interlocks, and the hardware comparator supervision. The communication partition owns protocols, records, HMI, update, and diagnostics. The partitions are enforced by code structure, static allocation, interrupt priority, and MPU or TrustZone where the MCU provides it. The five bounded contexts are `domain.protection`, `domain.feeder_automation`, `domain.fault_recording`, `domain.power`, and `domain.device_control` [V implementation plan section 12]. The hardware includes a three-phase 0.5S metering AFE [V hardware doc section 5.4]. The firmware reads and reports metering data. Legal metrology approval, sealing, and billing certification are a separate scope; if the tender requires billing, the metering subsystem must be certified as such. The breaker mechanism, insulation, arc interruption, the master station, the carrier network, and the cloud are out of scope.

**Consequences.** One image, one update path, one safety case, and one configuration model. The protection partition can be verified independently even though it shares the MCU. The cost is a strict internal boundary that must be enforced and tested. A communication fault must not reach protection, so shared mutable state is forbidden, the protection path has no heap and no blocking, and the communication tasks run below the ADC interrupt. Billing approval is not claimed by the FTU firmware.

**Evidence level.** Scope [V image, V hardware doc, V implementation plan]. Standards [A per the matrix; A DL/T 2859-2024 as the complete-set main standard, verified research update]. Partition choice [E]. Billing boundary [E until the tender].

**What evidence would overturn it.** A certification rule that requires independent protection and communication processors. A measured communication load that cannot coexist with protection on one MCU. A tender that requires billing approval inside the FTU firmware.

**Verification.** A scope traceability matrix from the slide, the standards matrix, and the hardware doc to the partitions. An interface contract test for the partition boundary. A dependency graph showing the protection partition does not link communication code. A fault-injection test showing a communication task failure does not change the protection output. A variant build matrix. A review that no billing claim appears without certification.

**Open questions.** Does the first tender require billing? Is the automation interface a separate box or integrated? Which functions must remain local when the communication partition is unhealthy? Is the FTU a separate sale item?

### ADR-002 SoC, MCU, and safety architecture

**Status.** proposed, 2026-11-23.

**Context.** The device must trip on a fault, not trip on noise, survive a communication failure, and be certifiable. The trip path is the dominant safety concern. The product is distribution-grade, not transmission-grade. The hardware doc's baseline is a single Cortex-M4F or Cortex-M33 with FPU, at least two UARTs, SPI, I2C, Ethernet MAC, and enough ADC channels [V hardware doc section 5.2]. The hardware doc rejects the repo's Cortex-M0 as the FTU main controller and allows a dual-core Cortex-A plus real-time core only when Linux, containers, or a complex HMI is required [V hardware doc section 5.2]. The hardware doc requires an external window watchdog plus voltage monitor, with an independent clock and an enable pin the firmware cannot disable [V hardware doc section 5.9].

**Decision drivers.** Trip determinism. Failure-to-trip versus spurious-trip risk. Cost and power. WCET. Freedom from interference. Update isolation. Supply chain. Certification.

**Options considered.**
1. Single Cortex-M4F, FreeRTOS, external window watchdog, hardware overcurrent comparator, hardwired anti-pump. Lowest cost, deterministic, no TrustZone.
2. Single Cortex-M33 with TrustZone, external watchdog, hardware comparator, hardwired anti-pump. Better isolation for the protection partition and secure boot, at some ecosystem and cost risk.
3. Dual-core Cortex-M7 plus M4 or Cortex-A plus real-time core. More headroom for Linux or a rich HMI, but shared memory, power, cost, and software complexity. The hardware doc does not use this as the baseline.
4. Lockstep safety MCU. Highest integrity, but cost, power, toolchain, and certification evidence exceed the distribution FTU's needs.
5. FPGA or CPLD protection plus MCU. Deterministic hardware protection, but development and verification cost, and inflexible settings.

**Decision.** Option 1 or option 2, selected by the security and ecosystem evaluation. The baseline is a single main controller with an external window watchdog, a voltage monitor, a hardware overcurrent comparator in the trip path, and hardwired anti-pump and anti-slow-close logic. The MCU does not carry the anti-pump or anti-slow-close safety function alone [V hardware doc HW-005]. The trip coils and the charging motor are 24 V DC short-time loads from the supercap or battery; 48 V DC is an alternative [V hardware doc sections 2.3 and 4.1]. The trip path is a comparator from the current signal, filtered and debounced, driving a one-shot and the trip driver. The MCU trip output is ORed through a second switch and a coil-current monitor. The trip pulse is bounded. The close path is separate and interlocked with spring charged, control voltage normal, no fault, local or remote, and no blocking condition. Brownout holds the MCU in reset, disables the drivers, and leaves the trip reserve charged. The external watchdog is an availability mechanism, not a trip mechanism. Lockstep is rejected for the baseline. If the product later claims transmission-grade protection or a SIL target, reopen option 4.

The safety goals are: SG1, do not fail to trip on high current when the trip coil and energy are available, covered by the hardware comparator. SG2, do not close on a fault or with the spring uncharged, covered by hardwired interlocks and anti-pump. SG3, do not lose protection because of a communication failure, covered by the partition and the upstream feeder relay. SG4, do not corrupt records on power loss, covered by ADR-008. SG5, do not let a single software fault issue a trip or close, covered by the output readback and the one-shot.

**Consequences.** A single MCU keeps cost, power, and board area low and matches the hardware doc. The protection partition shares the MCU with communication, so the safety case must prove freedom from interference. The hardware comparator and the hardwired anti-pump carry the dominant dangerous failures. The watchdog and voltage monitor cover availability. The cost is a strict partition design and a fault-injection campaign.

**Evidence level.** Platform baseline [V hardware doc]. Standard identities [A per the matrix]. Safety goals and coverage [E until FMEA and HIL]. Trip timing targets [目标 in the hardware doc].

**What evidence would overturn it.** A measured WCET or fault-injection result that shows the single-MCU partition cannot isolate protection. A certification rule that requires lockstep or a dual-channel trip path. A BOM target that rules out the external watchdog or the comparator. A communication load that requires a dual-core SoC.

**Verification.** FMEA against SG1 to SG5. Fault injection on the stuck GPIO, stuck comparator, watchdog, brownout, trip coil open, communication task hang, and MPU or TrustZone violation. Trip-time measurement with a relay test set. Comparator threshold and delay test. Production trip-path test. EMC and temperature. HIL. The safety argument must state that loss of FTU protection is covered by the upstream feeder relay and that failure to trip on high current is covered by the comparator.

**Open questions.** M4F or M33? Is TrustZone required by the security profile? Is upstream backup protection guaranteed by the utility? What trip time and MTBF are required? Is the external watchdog window fixed by the performance doc?

### ADR-003 RTOS versus baremetal versus mixed criticality, and the task and ISR model

**Status.** proposed, 2026-11-23.

**Context.** Protection needs fixed latency and no jitter. Communication needs blocking I/O, TCP/IP, timers, and a large stack. The hardware doc's baseline is one main controller, so the two domains share the MCU. The repo already has a FreeRTOS target pattern and a thread-model document [V `docs/guides/product-platform-extension.md`, `docs/design/freertos-thread-model.md`].

**Decision drivers.** Determinism. WCET. Isolation. Developer productivity. Reuse. Certification.

**Options considered.**
1. Baremetal everywhere. Deterministic, but the communication stack becomes a cooperative state machine and is hard to maintain.
2. FreeRTOS everywhere, protection in the highest-priority task. Easier communication, but preemption and interrupt latency add jitter. A communication fault can starve protection.
3. Mixed. Protection in the ADC end-of-conversion ISR and a small main loop. FreeRTOS for communication, records, HMI, update, and diagnostics. MPU or TrustZone for the partition. Recommended.
4. Two MCUs. Protection and communication on separate chips. Strongest isolation, but the hardware doc does not use it as the baseline and it adds cost and board area.

**Decision.** Option 3. A hardware timer starts the ADC conversion. The ADC end-of-conversion interrupt runs sampling, the DFT, the protection element evaluation, and the trip decision. The ISR is the highest priority and never blocks. A 1 ms tick handles timekeeping and supervision. The protection main loop handles self-test, calibration, and housekeeping. FreeRTOS runs the communication and management tasks. The protection partition has no dynamic allocation, no blocking, no flash write, and no protocol code. Shared state is limited to immutable snapshots and bounded queues. The communication tasks run below the ADC interrupt. If the MCU is an M33, the protection partition runs in the secure world and communication in the non-secure world. If the MCU is an M4F, the MPU isolates the protection region and the communication tasks cannot write it. The protection output is written at the end of the ISR. The communication partition cannot command a trip or close directly. It sends a validated control request that the protection partition checks.

**Consequences.** Protection is deterministic even with an RTOS on the same core. Communication can block and use a full stack. The cost is a strict priority and memory design, a trace tool, and a fault-injection campaign. The partition boundary is a new internal contract that must be validated and fuzzed.

**Evidence level.** Architecture [E]. Platform baseline [V hardware doc]. Repo pattern [V]. RTOS model check exists in the repo [V `AGENTS.md` `check:rtos-model`].

**What evidence would overturn it.** A measured ISR latency or CPU load that misses the protection budget. An MPU or TrustZone configuration that cannot isolate the partition without unacceptable overhead. A dual-core requirement from the hardware or certification team.

**Verification.** WCET with a GPIO pulse and a logic analyzer. Interrupt latency. Stack high-water marks. Queue depth. Fault injection on the partition boundary. Watchdog tests. Long-run soak. The repo's RTOS model check for the FTU target.

**Open questions.** M4F with MPU or M33 with TrustZone? Which functions must remain local when the communication partition is unhealthy? Is the protection partition allowed to use the FPU? What is the maximum communication task priority?

### ADR-004 Sampling front end and protection algorithm architecture

**Status.** proposed, 2026-11-23.

**Context.** The hardware doc separates protection and metering sampling into two independent chains [V hardware doc section 3.3]. The protection chain uses the protection CT, PT, and zero-sequence CT. The metering chain uses the metering CT and the metering AFE. Protection needs simultaneous three-phase sampling, hardware triggering, sample timestamps, and independent calibration. Metering needs 0.5S accuracy, multi-point calibration, and temperature compensation. The protection ADC is the main controller's internal ADC only if it meets simultaneity and ENOB; otherwise an external synchronous ADC is used [V hardware doc section 5.4]. The performance document owns the sample rate, resolution, ENOB, synchronization error, and reference drift [V hardware doc section 3.3].

**Decision drivers.** Simultaneity. Accuracy. Anti-alias. Dynamic range. Deterministic latency. Cost. Isolation. Calibration.

**Options considered.**
1. Main controller internal ADC with synchronized channels. Lowest cost, but sample-and-hold skew and noise may not meet the phase and zero-sequence accuracy.
2. External simultaneous-sampling SAR ADC, AD7616 class, 16-bit, 8 channels. Deterministic and well understood. The supplied datasheet set contains AD7616, AD7771, and AD7779 [V file presence in the supplied set; part specs are E].
3. External delta-sigma modulators, AD7771 or AD7779 class, 24-bit. Higher resolution and isolation, but group delay and filter design complicate protection timing. Better for the metering AFE.
4. Metering IC with hardware RMS and DFT plus a comparator. Good for measurement, weak for protection flexibility and fault recording.

**Decision.** The protection architecture is independent of the ADC part choice. Use the main controller's internal ADC if the measured simultaneity, ENOB, and reference drift pass the performance budget. Otherwise use an external simultaneous-sampling SAR ADC. The metering chain always uses a separate three-phase 0.5S AFE. The protection chain has eight channels: Ia, Ib, Ic, I0, Ua, Ub, Uc, U0. The zero-sequence channel has a lower range. The performance document fixes the protection sample rate at 12.8 kHz, 78.125 µs, 256 points per cycle, and the ENOB target at ≥11 bit [V performance doc T-P1 and M-A1; P-24 B]. The performance document also sets the ADC and algorithm budgets, and it supersedes any older provisional rate.

The protection engine is a table of protection elements, not one function per feature. Each element has type, source, pickup, dropout, delay, curve, harmonic restraint, directional mode, and output. Each runs a small state machine: idle, pickup, timing, trip, reset. The engine evaluates all elements per sample. The trip is the OR of the element outputs and is latched until the breaker opens and the current drops. Settings are data validated at the configuration boundary. The element table is the domain model for the protection matrix. It keeps the settings table and the evaluator together and makes the protection behavior auditable.

Algorithms are full-cycle DFT for fundamental phasors, half-cycle RMS or peak for instantaneous overcurrent, second-harmonic restraint for inrush, zero-sequence from the dedicated core-balance CT, directional from the phase angle with voltage memory for close-in faults, and frequency tracking across 50 Hz ±5 Hz [E]. The fast path uses fixed-point or single-precision FPU with precomputed coefficients. No dynamic allocation, no recursion, no library call with variable latency. The protection CT's accuracy limit factor must cover the 20 kA fault current, which is about 31.7 times the 630 A rated current. A 5P10 core may saturate, so the CT selection must calculate the ALF for 20 kA, the secondary burden, the DC component, and remanence [V hardware doc section 3.2]. This is a hardware constraint that the algorithm cannot fix.

**Consequences.** Simultaneous sampling gives correct phase and zero-sequence. The element table makes settings and behavior auditable. The protection kernel is testable on the host with recorded waveforms. The cost is an ADC and analog front end that must meet the performance budget, a separate metering AFE, and a CT whose ALF is calculated for the fault current. The 12.8 kHz baseline rate may be marginal for arc or transient analysis, so fault recording may need a higher-rate path or a separate recorder. The 12.8 kHz rate is 256 points per cycle, which is high for an M4F, so the CPU and WCET budgets in ADR-022 are tight.

**Evidence level.** Architecture and element table [E]. Platform and chain separation [V hardware doc]. Test methods [A GB/T 7261-2016 per the matrix]. Complete-set standard [A DL/T 2859-2024, verified research update]. Accuracy limits [C]. Rates and ENOB [V performance doc].

**What evidence would overturn it.** A measured internal ADC that meets the phase and zero-sequence accuracy and the ENOB budget. A protection requirement that needs a different algorithm or a higher sample rate. A CT selection that cannot cover 20 kA without saturation.

**Verification.** COMTRADE playback. Injected waveforms at threshold, 1.1x, 1.5x, 5x, and 10x. Timing accuracy. Harmonic and DC-offset tests. CT saturation. Frequency sweep. Zero-sequence sensitivity. Directional angle sweep. Temperature drift. Calibration. Comparison against a reference relay. HIL.

**Open questions.** Does the 12.8 kHz, 256 points per cycle baseline hold for this hardware, or does the performance doc need a lower rate? Is the internal ADC sufficient for the ENOB target? What is the protection CT ALF and secondary burden? Is a dedicated zero-sequence CT present? Is 小电流接地选线 in scope? What fault-recording sample rate and record length are required?

### ADR-005 Power supply and energy budget

**Status.** proposed, 2026-11-23.

**Context.** A pole device has no reliable auxiliary supply. The hardware doc's baseline is dual harvesting, CT and PT or AC 220 V, with a 24 V control bus and 48 V as an alternative, a LiFePO4 battery, and a supercapacitor [V hardware doc sections 4.1 to 4.3]. The trip and close energy must not depend on line harvesting. The FTU must be alive when the breaker is open and the source side is energized. The hardware doc's backup targets are at least 24 h protection and standby, at least 4 h protection plus 4G periodic communication, and at least 3 trip or close operations [目标 in hardware doc section 4.6]. The performance document records the DL/T 721-2024 candidate of 8 h battery or 30 min supercap [V performance doc section 5.3; B], and it notes the conflict: the 4 h design target is below the 8 h standard candidate. The stricter combination for design is 24 h standby and 8 h communication. PRD must close the choice. The performance document also sets the hold-up and save path: hold-up ≥100 ms, and total save path ≤50 ms with an upper limit of 80 ms [V performance doc W-24 and W-25].

**Decision drivers.** Availability. Trip energy. Cold start. Backup duration. Battery life. Cost. Temperature. Safety.

**Options considered.**
1. CT harvesting only with a supercap. No battery. Works only when load current flows. The FTU dies when the breaker is open or the line is unloaded.
2. CT plus PT or AC 220 V plus LiFePO4 plus supercap. Recommended and aligned with the hardware doc.
3. Battery-primary. Long backup, but battery life, temperature, and transportation rules dominate.
4. External 24 V or 48 V DC or 220 V AC auxiliary supply. Only for a cabinet variant.

**Decision.** Option 2. The 24 V bus is the control and energy storage bus. The trip, close, and motor pulses draw from 24 V. The electronics use isolated DC/DC to 12 V, 5 V, and 3.3 V. A 48 V DC variant is allowed where the tender or the mechanism vendor requires it. The CT harvesting and the PT or AC 220 V input are OR-ed or ideal-diode isolated. The trip energy always comes from the supercap or battery, not from line harvesting. The battery is LiFePO4 with a BMS for cell balancing, overcurrent, overtemperature, undervoltage, and low-temperature charge inhibit. The supercap handles the trip and close peak and the power-fail hold-up. A heater is controlled independently of the main controller. Power-fail sequence: interrupt, save critical events and counters, notify the master, shut down non-critical loads, then park the outputs. The reset state has trip and close outputs de-energized. The exact thresholds, hold-up times, and backup durations are owned by the performance document. This ADR does not invent them.

**Consequences.** The device is available when the breaker is open and the source side is live. Protection does not depend on the battery. The trip path has local energy. The cost is more BOM, a battery maintenance item, a supercap aging item, a heater, and a power budget that must be measured. The 24 h standby and 8 h communication targets drive the battery size. A 7 Ah, 24 V battery is about 168 Wh. A 10 W load for 8 h needs 80 Wh, while a 21 W load for 8 h needs 168 Wh with no margin [V performance doc section 5.3 arithmetic]. The product requirement must settle the target before the battery is sized.

**Evidence level.** Topology and targets [V hardware doc]. Backup requirement [A DL/T 721-2024 and Q/GDW 10514-2018 per the matrix; B for the 8 h and 30 min values]. Energy numbers [E]. Hold-up and save path [V performance doc W-24 and W-25].

**What evidence would overturn it.** The PT cannot be source-side. The battery is not allowed on the pole. The CT cannot supply the required power at minimum current. The supercap cannot deliver the trip energy at end of life. The tender requires a backup time that LiFePO4 cannot meet at the temperature extremes.

**Verification.** Source-measure unit profile across idle, communication peak, and trip. Cold start at -40 ℃ and +70 ℃. CT and PT loss. Supercap end-of-life. Battery life and self-discharge. Brownout sweep. Trip at minimum rail. Heater test. EMC and surge. HIL.

**Open questions.** What backup time does the tender require, 24 h standby and 8 h communication, 4 h, or the DL/T 721-2024 8 h battery or 30 min supercap? Is the PT source-side or load-side? Is a battery allowed on the pole, and what is the replacement interval? What is the minimum CT current? Who owns the performance document's energy budget?

### ADR-006 Communication stack and protocols

**Status.** proposed, 2026-11-23.

**Context.** The FTU speaks to a distribution automation master. The hardware doc provides 4G Cat.1 as the cost baseline, Cat.4 when large files or waveforms require it, Ethernet 10/100, RS-485 channels, IR, and USB or serial maintenance [V hardware doc section 5.8]. The standards matrix says the main station side is primarily DL/T 634.5101 and 5104, 61850 is for model self-description and plug-and-play, and 698.45 is for the acquisition or fusion terminal channel when required [V standards matrix]. The repo has a DL/T 698.45 stack and a DL/T 645 stack for metering [V `docs/architecture.md`, `application/adapters`]. It does not have 104 or 61850.

**Decision drivers.** Conformance. Interoperability. FA latency. Security. CPU and RAM. Reuse. Product variants.

**Options considered.**
1. 104 only. Simple and widely deployed. No native GOOSE for distributed FA and no rich object model.
2. 104 plus 61850 MMS and GOOSE. Recommended. 104 for legacy SCADA, 61850 for the information model, reporting, and peer-to-peer FA, GOOSE for fast local messages.
3. 61850 only. Future-proof, but excludes utilities that still require 104.
4. 698.45 as the SCADA protocol. Rejected. It is a metering data-exchange protocol. It lacks the distribution automation control, reporting, and FA semantics.

**Decision.** Option 2. The canonical information model is protocol-neutral and lives in the application and domain layers. It covers measurements, statuses, events, controls, settings, records, and files. Adapters map it to 104 and 61850. The 61850 model is derived from an ICD or SCL file, not hand-written parallel types. The 104 point table is a configuration artifact. GOOSE is used for FA and interlock messages, not for SCADA polling. MMS is used for monitoring, control, reporting, and file transfer. 104 runs over 4G or Ethernet. 101 runs over RS-485 as the legacy serial path. DL/T 698.45 and DL/T 645 are used only on the metering or fusion channel. MQTT is an optional diagnostics or cloud channel, not a control path. It is outside the protection and FA critical path and must be authenticated and rate-limited. 4G is a transport, not a protocol. The product supports dual SIM or eSIM and a private APN or VPN where the utility requires it. The 61850 edition and the 90-6 or 90-12 distribution profile are TBD.

**Consequences.** One information model and two protocol adapters. The product fits legacy and new masters. GOOSE gives the FA latency. The cost is a heavy 61850 MMS, ASN.1, and TLS stack on the same MCU as protection, which raises the RAM and CPU budget and strengthens the case for the communication partition. The 104 and 61850 mappings must be kept consistent. A point-table change is a tested configuration change. MQTT adds a security surface and must not control the breaker.

**Evidence level.** Protocol identities [A DL/T 634.5101-2022 and DL/T 634.5104-2009, DL/T 860 series, DL/T 698.45-2017/2026, DL/T 1914-2018 per the matrix and the 2026-11-23 research update]. Edition and profile [C]. Architecture [E]. Hardware interfaces [V hardware doc].

**What evidence would overturn it.** A master that requires 698.45 as the SCADA protocol with a published utility profile. A 61850 edition or distribution profile that the available stack does not support. A CPU or memory measurement that makes MMS plus TLS infeasible on the chosen MCU.

**Verification.** 61850 conformance per the applicable DL/T 860 and IEC 61850-10 profile [A/B]. 104 conformance and interoperability with at least two master vendors. 101 conformance over RS-485. GOOSE latency and loss tests. Fuzz tests on every parser. TLS conformance. Long-run stability. CPU and RAM profile. A traceability matrix from the information model to both protocols.

**Open questions.** Which 61850 edition and which 90-6 or 90-12 profile? Does the master require MMS reporting or only polling? Is GOOSE allowed on the utility network? Is MQTT required by the utility's IoT platform? Which 4G bands and carrier certifications?

### ADR-007 Feeder automation architecture

**Status.** proposed, 2026-11-23.

**Context.** The slide claims fault detection, isolation, alarm, line monitoring, and automatic restoration of non-faulted sections [V image]. The product can implement this centrally, locally, or distributed. The choice changes the firmware, the communication, and the failure modes. The standards matrix points to Q/GDW 10514-2018 and DL/T 721-2024 for terminal functions [A]. The performance document sets the FA targets: intelligent distributed isolation about 100 ms, restoration 200 to 500 ms, GOOSE peer delay under 10 ms, and the terminal's own isolation and restoration paths at 50 ms each [V performance doc T-F12, T-F13, T-F8, T-F9, and C-16] plus the centralized and local system targets [V performance doc T-F10 and T-F11].

**Decision drivers.** Fault-clearing time. Dependence on the master. Communication failure tolerance. Topology flexibility. Certification. Cost.

**Options considered.**
1. Centralized only. The master detects and decides. The FTU reports and executes. Simple firmware, but restoration takes seconds to minutes and fails when the link fails.
2. Local voltage-time or current-time. No peer communication. Each FTU follows a time sequence after loss of voltage or fault current. Fast and robust, but settings depend on the topology and the scheme.
3. Distributed peer-to-peer GOOSE. Adjacent FTUs exchange fault, blocking, and interlock messages. The faulted section isolates in tens of milliseconds. Fast and selective, but needs a network, a topology model, and careful failure handling.
4. Hybrid. Local as the baseline, distributed as the premium, centralized for monitoring and optimization. Recommended.

**Decision.** Option 4. The product supports all three modes as configuration. Local voltage-time is the default autonomous mode. Distributed GOOSE is the premium mode. The FA logic is a domain state machine in `domain.feeder_automation` over a topology model, with upstream, downstream, tie, and normally open point. It is not a set of protocol handlers. The FTU never closes into a fault. Restoration is gated by voltage presence, a sync check where applicable, and a fault-cleared indication. Distributed mode has explicit failure handling for loss of a GOOSE peer, loss of time sync, false fault, multiple faults, breaker failure, and topology change. On loss of peer communication, the scheme falls back to local. On loss of both, it blocks automatic restoration and alarms. Centralized mode is always available for monitoring and manual control.

**Consequences.** One product covers multiple utility practices. Local mode works without a network. Distributed mode gives fast isolation. The cost is three modes in the test matrix, a topology model and settings burden, and a network design and security review for distributed mode. A bug in the FA state machine can cause a wrong restoration, so the state machine needs formal or exhaustive testing.

**Evidence level.** Product claim [V image]. Standard functions [A Q/GDW 10514-2018 and DL/T 721-2024 per the matrix]. Timing [V performance doc targets, C]. Scheme details [C].

**What evidence would overturn it.** A utility that forbids GOOSE or peer-to-peer communication. A topology where local voltage-time cannot be selective. A measured restoration time that fails the tender.

**Verification.** Multi-FTU HIL with a real-time simulator and a GOOSE network. Fault scenarios for each topology. Communication loss injection. Multiple faults. Breaker failure. Restoration into a fault. A TLA+ model of the state machine for no simultaneous close, no close into a fault, and no restoration without a fault-cleared indication. Field pilot on a test feeder.

**Open questions.** Which FA modes does the first utility require? Is a tie breaker and a normally open point present? What isolation time is required? Does the utility allow GOOSE over 4G, or only over fiber? Who owns the topology model and the settings?

### ADR-008 Storage and event records

**Status.** proposed, 2026-11-23.

**Context.** The FTU must record SOE, events, fault records, waveform records, settings, and calibration. The hardware doc provides EEPROM for calibration, parameters, and counters, SPI NOR for events, waveforms, and firmware staging, and FRAM as an option for power-fail critical writes [V hardware doc section 5.5]. The repo has a power-fail-safe sector store and record ring [V `docs/design/record-retrieval.md`]. The implementation plan defines `domain.fault_recording` as a bounded context [V implementation plan section 12].

**Decision drivers.** Power-fail atomicity. Wear. Retention. Retrieval speed. Record size. Cost. Temperature.

**Options considered.**
1. EEPROM only. Simple and byte-writable, but small, slow, and limited endurance. Unsuitable for waveforms.
2. EEPROM plus SPI NOR, with FRAM optional for power-fail critical writes. Recommended and aligned with the hardware doc.
3. eMMC or SD. Large and cheap, but filesystem corruption, wear, temperature, and vibration risk.
4. MCU internal flash only. Limited size and erase cycles. Blocks the CPU during erase. Not enough for waveforms.

**Decision.** Option 2. EEPROM holds calibration, parameters, and counters. SPI NOR, the existing MX25L3206E class, holds the SOE and event ring, the fault and waveform records, and the firmware staging area. FRAM is optional for the small, frequently written power-fail critical state if the EEPROM wear or write time fails the budget. The block store and record ring reuse the modular-001 design: sector ring, CRC, sequence numbers, snapshot reads, reset marker, and the TLA+ model [V `docs/design/record-retrieval.md`]. The record codec is new. COMTRADE is the waveform format, but it is not in the current standards matrix, so add it before citing a revision [B]. The event, SOE, and settings records are project-defined. The store is protocol-neutral. Retrieval adapters for 61850 file transfer, 104 file transfer, local USB or serial, and the metering channel read the same records.

The performance document sizes the storage: EEPROM 8 KiB target and 16 KiB limit for parameters, calibration, and counters; EEPROM or FRAM 2 to 4 KiB for the power-fail SOE journal; NOR 32 to 64 KiB for event logs; NOR 720 KiB target and 1 MiB limit for 64 waveform records; and NOR firmware staging at least twice the image size [V performance doc section 4.3]. A waveform record is 6 channels × 80 points per cycle × 12 cycles × 2 bytes = 11,520 bytes. Sixty-four records is about 720 KiB. At 128 points per cycle the requirement is about 1.44 MiB, so the NOR must be at least 2 MiB [V performance doc section 4.3]. These are the authoritative sizing inputs.

Atomicity follows the repo's design. A record is visible only after its header and CRC are committed. A power loss during erase leaves the old sector valid. The reset marker and the ring sequence handle partial resets. The boot scanner feeds the watchdog during long erase sequences. The performance document sets the hold-up at ≥100 ms and the save path at ≤50 ms, upper 80 ms, and notes that NOR erase is 45 to 400 ms, which exceeds the hold-up [V performance doc W-24, W-25, and section 5.5]. Therefore critical SOE, journal, and parameter writes go to EEPROM or FRAM, and NOR holds non-critical event logs and waveform records. The design does not rely on a filesystem.

**Consequences.** High endurance for calibration and parameters. Large space for records. Proven power-fail semantics. The cost is two media, two drivers, a new record codec, and wear and retention measurements for the chosen parts. FRAM is optional, not baseline, which keeps the BOM aligned with the hardware doc.

**Evidence level.** Hardware media [V hardware doc]. Repo design [V]. Sizing [V performance doc section 4.3]. Part behavior [E]. COMTRADE revision [B].

**What evidence would overturn it.** A measured NOR erase time that makes the ring unsafe without a write window. A retention requirement that needs a different medium. An EEPROM wear or write-time failure that forces FRAM into the baseline.

**Verification.** Power-fail injection at every write and erase step. CRC corruption. Ring wrap. Reset during write. Wear simulation. Retention at temperature. Retrieval conformance. COMTRADE validation. The TLA+ model. HIL.

**Open questions.** What fault-record length and sample rate are required? How many records must be retained? Is COMTRADE 1991, 1999, or 2013 required? Is FRAM needed in the baseline? What is the NOR endurance at the pole temperature?

### ADR-009 Local HMI and commissioning

**Status.** proposed, 2026-11-23.

**Context.** The device is on a pole, often inaccessible without a bucket truck. The local interface is for installation, calibration, phase check, topology and GOOSE configuration, and emergency operation. The hardware doc provides LEDs, buttons, a 38 kHz IR port, a USB or serial maintenance port, and a sealed cover with authentication [V hardware doc sections 6.5 and 6.6]. It does not include Bluetooth in the baseline BOM. A display and keypad add cost, sealing, and failure points.

**Decision drivers.** Cost. IP rating. Temperature. Usability. Security. Commissioning time. Safety.

**Options considered.**
1. Minimal LEDs and buttons, IR only. Cheapest and robust, with limited diagnostics.
2. LEDs and buttons, 38 kHz IR, USB or serial maintenance. Recommended and aligned with the hardware doc.
3. Add Bluetooth LE with a phone app. Rich UI, but a new radio, a new security surface, and a BOM change that the hardware doc does not include.
4. Local LCD and keypad. Best standalone usability, but cost, sealing, condensation, and temperature problems.

**Decision.** Option 2. The on-device HMI is LEDs for power, run, fault, communication, trip/close, and local/remote, and a small button set for local/remote, trip, close, and reset. A hardware local/remote keyswitch is the authoritative selector. The software reads it and never overrides it. The IR port is the isolated maintenance channel and the factory fallback. The USB or serial port is inside the sealed cover and is disabled by default after production. The cover opening is logged. There is no LCD or keypad in the pole variant. Bluetooth is a future option only if the tender allows it and the hardware adds the radio; it is not in the baseline.

Commissioning functions are parameter templates, CT and PT ratio and calibration, phase identification, topology and GOOSE or SCD configuration, time sync, self-test, fault injection and test mode, log retrieval, and firmware update. Every setting change is audit-logged with the operator identity and time. The session state machine covers local/remote, maintenance, select-before-operate, and timeout. Remote control requires select-before-operate. Local control requires the keyswitch in local and a guarded button sequence. The maintenance port and the IR parser follow the repo's fail-closed pattern with an abort on an incomplete frame [V `docs/design/record-retrieval.md`].

**Consequences.** Low cost, robust sealing, and an isolated fallback. The cost is limited field diagnostics without a tool, a maintenance port that must be physically protected, and no phone app in the baseline. A future Bluetooth option would require a new hardware revision and a security review.

**Evidence level.** Hardware HMI [V hardware doc]. IP and terminal requirements [A GB/T 4208-2017, Q/GDW 10514-2018, Q/GDW 11815-2023 per the matrix]. Architecture [E].

**What evidence would overturn it.** A utility that requires Bluetooth commissioning. A requirement for a local display. A maintenance port that cannot be sealed or authenticated.

**Verification.** HMI state machine tests. Button bounce and key sequences. LED patterns. IR frame fuzz and abort behavior. Maintenance port authentication and cover-open audit. Local/remote interlock. Select-before-operate timeout. Audit log. Commissioning acceptance with a PC tool.

**Open questions.** Is Bluetooth allowed by the utility? Is a cabinet variant with a display required? What is the commissioning credential lifecycle? Who owns the maintenance tool?

### ADR-010 Cybersecurity

**Status.** proposed, 2026-11-23.

**Context.** The FTU is on a 4G or utility network and can trip and close a breaker. A compromised device is a grid safety event. The hardware doc's baseline is 国网 ESAM plus secure boot, signed firmware, anti-rollback, debug lock, key injection, and audit logs, with an additional secure element or TrustZone if the MCU cannot protect the TLS keys [V hardware doc section 5.7]. The standards matrix lists the IEC 62351 series, GB/Z 25320, and the utility and energy regulator security rules [A/B].

**Decision drivers.** Confidentiality. Integrity. Availability. Authentication. Audit. Update. Key lifecycle. Cost. Performance.

**Options considered.**
1. Perimeter security only. The utility VPN and firewall protect the device. No secure boot and no signed firmware. Cheap, but a credential leak or a local port compromises the device.
2. Device hardening. Secure boot, signed images, ESAM or secure element, TLS, role-based access, audit, disabled debug, tamper detection. Recommended.
3. Full IEC 62351 profile plus a hardware trust root and measured boot. Strongest, but more cost and integration. Some parts, especially GOOSE security, may not fit the latency budget.
4. Air gap. Not compatible with 4G remote operation.

**Decision.** Option 2, with the option 3 parts that the utility profile requires. ESAM is the baseline for identity and key storage. If the main controller cannot protect the TLS keys or the secure boot root, add a secure element. The bootloader verifies the application image signature before jump. The algorithm is ECDSA P-256 or 国密 SM2, chosen by the utility. Keys are stored in ESAM or the secure element, never in plain flash. TLS 1.2 or 1.3 with mutual authentication protects 104 and 61850 MMS where the master supports it. The IEC 62351 part mapping is applied as the utility profile requires, with 62351-3 for TLS, 62351-4 for MMS, 62351-5 for 104, 62351-6 for GOOSE, and 62351-8 for role-based access [A standard set, C part details]. GOOSE security is applied only if the latency budget and the utility network allow it. Otherwise GOOSE is confined to a protected VLAN and the residual risk is documented. Roles are viewer, operator, engineer, and factory. Debug and JTAG are disabled in production. The case has a tamper switch or cover-open detection. Audit logs are integrity-protected. Firmware update is signed and anti-rollback.

**Consequences.** A compromised master or network cannot silently reflash the device. Local and remote access are authenticated. The product can pass a utility security review. The cost is boot time, CPU for TLS and signature verification, factory key provisioning and rotation, and a residual GOOSE risk that must be accepted and documented. ESAM is already in the hardware BOM, so the marginal cost is the secure element only if the MCU cannot protect the keys.

**Evidence level.** Hardware security baseline [V hardware doc]. Standard identities [A IEC 62351 series, B GB/Z 25320 and the utility rules per the matrix]. Part mapping and limits [C]. Architecture [E].

**What evidence would overturn it.** A measured boot or TLS cost that makes the main controller infeasible. A utility rule that GOOSE security is mandatory and cannot meet the latency. A secure element that fails the supply-chain or cost requirement. ESAM that cannot store the required keys.

**Verification.** Threat model. Secure-boot bypass tests. Key extraction attempts. TLS conformance and mutual authentication. Role and permission tests. Fuzzing of every network and local parser. Tamper response. Audit-log integrity. Penetration test. Utility security test. SBOM and CVE monitoring. A residual-risk register for GOOSE.

**Open questions.** 国密 or international algorithms? Which 62351 parts does the utility require? Is a secure element required, or is ESAM plus MCU protection sufficient? Who provisions and rotates keys? Is GOOSE security mandatory? What is the security certification path?

### ADR-011 Firmware update and rollback

**Status.** proposed, 2026-11-23.

**Context.** The device is remote. A failed update can brick a protection device or leave it on a vulnerable image. Power loss during update is likely on a pole. The hardware doc requires dual-bank or external staging with signature and rollback [V hardware doc section 5.5]. The implementation plan's provisional OTA budget is at most 5 min [V implementation plan section 5.1]. The product has one main controller, so an update resets the same MCU that runs protection.

**Decision drivers.** No brick. Power-loss safety. Authenticity. Anti-rollback. Protection availability. Update time. Bandwidth. Recovery.

**Options considered.**
1. Single slot, in-place update. Cheapest, but a power loss or a bad image can brick the device.
2. A/B slots or external staging with an immutable bootloader, signed images, trial boot, and rollback. Recommended and aligned with the hardware doc.
3. External staging only, copy to a single internal slot. Recovery is possible, but the copy window and the single slot still risk a brick.
4. Linux package update. Not applicable to the baremetal protection MCU. Possible only if a Linux communication module is added.

**Decision.** Option 2. Each image slot has a header with product ID, image type, version, size, hash, signature, minimum bootloader version, and an anti-rollback counter. The bootloader is immutable in the field, or updateable only with a signed bootloader image and a physical recovery jumper. The bootloader tries the new slot. The firmware runs a self-test and a health window, then commits. A watchdog reset or a failed self-test rolls back to the previous slot. The anti-rollback counter lives in OTP, ESAM, or the secure element. The update coordinator downloads to the inactive slot, verifies the signature, and only then switches. A protection-affecting update is allowed only with the breaker open and local or maintenance authorization, because the single MCU is unavailable during the slot switch and the boot. A communication-only change may be updated remotely if the product supports a partial update that leaves protection running; otherwise it uses the same maintenance window. A signed manifest covers the image and the configuration so the pair is compatible. Power loss at any point leaves at least one bootable slot. The hardware comparator path remains available even if the MCU is in the bootloader.

**Consequences.** No brick, rollback, and a clear protection-update policy. The cost is double flash for the image, a bootloader, key management, and a more complex release process. The health window and commit policy are product decisions that must be tested. The single-MCU architecture means a remote update of the protection image always has an availability cost.

**Evidence level.** Hardware update constraints [V hardware doc]. Provisional OTA budget [V implementation plan]. Architecture [E]. Security context [A/B].

**What evidence would overturn it.** A flash budget that makes A/B impossible. A certification rule that forbids field update of the protection image. A measured power-loss case that leaves both slots invalid. A product requirement for zero-downtime protection update, which would need a dual-MCU or dual-core architecture.

**Verification.** Power-loss injection at every write and at the slot switch. Signature failure. Corrupted image. Wrong product ID. Rollback. Anti-rollback. Interrupted download and resume. Image and configuration compatibility. Bootloader recovery. HIL.

**Open questions.** Is remote update of the protection image allowed? What is the maintenance window policy? How many versions must be retained? Is a recovery jumper acceptable on a sealed pole device? Who signs the images?

### ADR-012 Testability, HIL, and certification strategy

**Status.** proposed, 2026-11-23.

**Context.** Protection and FA behavior cannot be proven by unit tests alone. The product must pass utility type tests, protocol conformance, EMC, environmental, and safety reviews. The repo has a strong host, formal, and acceptance culture [V `AGENTS.md`, `docs/guides/verification.md`]. The standards matrix lists the test and certification basis, including Q/GDW 10639, Q/GDW 10514-2018, Q/GDW 11815-2023, Q/GDW 11813, Q/GDW 11253-2023, 国网设备〔2021〕151号, GB/T 7261-2016, and GB/T 13729-2019 [A/B]. The 2026-11-23 research update adds DL/T 2859-2024 as the complete-set main standard and confirms DL/T 634.5101-2022 as the current 101 edition; DL/T 634.5104-2009 remains the 104 basis [V research update]. The implementation plan lists HIL-only metrics and requires evidence per phase [V implementation plan sections 5.4 and 13].

**Decision drivers.** Evidence. Repeatability. Cost. Schedule. Coverage. Certification. Field confidence.

**Options considered.**
1. Unit and host tests only. Cheap, but no evidence for real-time, analog, EMC, or multi-device FA.
2. Device HIL with a relay test set, a breaker simulator or a real breaker, and a time source. Good for protection and control. Weak for multi-FTU FA.
3. System HIL with a real-time simulator, multiple FTUs, GOOSE, and time sync. Recommended for FA and protection integration.
4. Power hardware-in-the-loop. Strongest for the primary interface, but expensive and slow. Use only for the final validation if the lab requires it.

**Decision.** A four-layer test architecture. Layer 1 is host unit and property tests for algorithms, parsers, state machines, and record codecs, with ASan and UBSan and the repo's strict flags. Layer 2 is device HIL with a relay test set injecting currents and voltages into the FTU, a breaker simulator or a real breaker, and a time source. Layer 3 is system HIL with a real-time simulator and multiple FTUs over GOOSE, including communication loss, multiple faults, and topology changes. Layer 4 is certification and field pilot. The test harness is a product deliverable. It includes injection points, test mode, observable state, event logs, and a scripted acceptance runner. The HIL-only metrics are the protection trip delay, CT saturation and dynamic performance, temperature-point power, backup time, power-fail wake-up, 4G attach, GNSS or BeiDou time accuracy and SOE, mechanism timing, relay contact bounce, and EMC immunity [V implementation plan section 5.4]. The certification plan covers the utility's 一二次融合 入网检测, the terminal type tests, 61850 conformance, 104 conformance, EMC, environmental, and security. The exact scope and editions are assumptions until the tender and the lab confirm them.

**Consequences.** The repo's evidence model, acceptance matrix, `verdict=clean`, and formal checks extend to the FTU. Defects are found before the pole. The cost is HIL and simulator expense, a growing test matrix, and certification schedule risk. The harness must be maintained with the product.

**Evidence level.** Test standard identities [A/B per the matrix and the 2026-11-23 research update]. Scope and editions [C]. Test architecture and HIL-only list [V implementation plan]. Timing [V performance doc targets].

**What evidence would overturn it.** A lab that requires a different test sequence. A simulator that cannot model the target feeder. A certification scope that makes system HIL redundant.

**Verification.** Test reports, the acceptance matrix, HIL logs, formal models, and certificates. The formal set should include TLA+ for the FA and record-ring state machines, CBMC for the parsers and codecs, and Lean for the fixed-point protection arithmetic where it matters. Every safety goal in ADR-002 has a test or an analysis. Every protection element has a threshold and timing test. Every FA failure mode has an injected test. The repo assets are `tools/check_ftu_firmware.py`, the generalized acceptance matrix, `tools/check_formal.sh`, and the port-binding and mutable-globals baselines [V implementation plan section 13].

**Open questions.** Which lab and certification scope? Is a real-time simulator available, or is a relay test set plus a multi-FTU bench enough? What is the HIL budget and schedule? Which tests can run in CI?

### ADR-013 Reuse of the modular-001 monorepo versus a separate repository

**Status.** proposed, 2026-11-23.

**Context.** `modular-001` is a componentized C monorepo for meters and communication modules [V `docs/monorepo.md`]. It has layers, consumer-defined ports, component manifests, `product.yaml` selection, composition roots, strict host tests, formal checks, and a record-storage design [V `docs/platform-layout.md`, `docs/guides/port-binding.md`, `docs/guides/component-standard.md`, `docs/design/record-retrieval.md`]. The FTU is a different product line with hard real-time protection, 104 and 61850, and a safety case. The implementation plan recommends adding it to the same repo as `products/ftu_zw6b` and `targets/ftu_zw6b`, with named split conditions [V implementation plan section 12].

**Decision drivers.** Reuse. Tooling cost. Gate consistency. Certification isolation. Release cadence. Team ownership. CI time. Dependency management. Access control.

**Options considered.**
1. Same monorepo, new bounded contexts and target, with a qualification boundary around the protection partition. Recommended and aligned with the implementation plan.
2. Separate repository, copy the patterns and tools. Clean isolation, but duplicates the gates, the component model, and the review process. No shared versioning. The repo has no package manager to consume the shared components.
3. Hybrid. Protection core in a separate qualified repository, communication, management, and records in the monorepo. Strong isolation for the safety case, but two builds, an interface versioning problem, and integration overhead.
4. Fork the monorepo. Keeps history but creates a permanent merge burden and drift.

**Decision.** Option 1. Add `products/ftu_zw6b/product.yaml` and `targets/ftu_zw6b/{app,board,config,build,tests}`. Add the five bounded contexts: `domain.protection`, `domain.feeder_automation`, `domain.fault_recording`, `domain.power`, and `domain.device_control` [V implementation plan section 12]. Add `infra/soc/<vendor>/<soc>`, `infra/drivers/adc/<chip>`, `infra/drivers/comm/<module>`, `infra/drivers/security/<chip>`, and `infra/drivers/io/<do>`. Add a new protocol component for 104 under `components/protocol/iec60870`, depending only on domain, core, and services, and a new 61850 component whose name follows the component standard. Reuse the layer rules, the port pattern, the component standard, `product.yaml`, the gate scripts, the host test harness, the formal tooling, the FreeRTOS target pattern, the record-storage block and ring design, the maintenance and local-access pattern, and the ESAM integration pattern. Do not reuse the meter domain, the 698 and 645 main-protocol role, the metering aggregate, or the 103 KiB ROM and Cortex-M0-only budget. The FTU has its own Flash, RAM, and power budgets, its own acceptance matrix, and `tools/check_ftu_firmware.py` [V implementation plan section 12]. The protection path uses static allocation and does not depend on the heap or RTOS scheduling. The protection ISR uses a read-only root pointer initialized once, with a `mutable_globals_exceptions.txt` entry containing the reason, expiry, and approver [V implementation plan section 12].

| Asset | Verdict | Why |
| --- | --- | --- |
| Layer rules, ports, component manifests, `product.yaml`, composition root | Reuse | They match the FTU's structure and the project principles |
| Gate scripts, host strict, formal checks, acceptance matrix | Reuse, with a new FTU checker | The expensive part is the gates, and they are already proven |
| Record storage block and ring, TLA+ model | Reuse design, new codec | Power-fail semantics and wear are directly relevant |
| FreeRTOS target pattern | Reuse on the FTU main controller | Protection stays in the ADC ISR |
| Maintenance and local-access pattern | Reuse pattern | IR, local-only gate, watchdog feeding |
| ESAM integration pattern | Reuse pattern, new adapter | Identity and key storage |
| 698 and 645 stacks | Metering or fusion channel only | Not the SCADA protocol |
| Meter domain, metering aggregate | Do not reuse | Different bounded context and accuracy assumptions |
| 103 KiB ROM and Cortex-M0-only rule | Per product, not global | The FTU has a different MCU and memory budget |
| Hardware EDA, BOM, OEM certification | Not in the firmware repo | They stay in PLM or a controlled document library; the hardware doc is the interface summary |

**Consequences.** One toolchain, one review process, shared components, and one acceptance model. CI time and repo size grow. The safety case must explain why the shared components are suitable. The team must not let meter assumptions leak into the FTU. Mitigations are path-filtered CI, per-product gates, a qualification field in `component.yaml` as a follow-up, a protected branch for the protection partition, and a rule that the protection partition links only components tagged for it. The repo already needs a second SoC and board to validate the platform, and the FTU is that second product [V implementation plan section 12].

**Evidence level.** Repo facts [V]. Implementation plan recommendation [V]. Reuse judgment [E]. Safety-case suitability [E until the qualification review].

**What evidence would overturn it.** A non-ARM DSP or vendor-specific IDE such as C2000/CCS or Blackfin/CCES. A business unit that needs independent access control, release cadence, or customer delivery. A customer that requires the FTU firmware as a standalone deliverable [V implementation plan section 12]. A certification body that requires a frozen repository baseline the monorepo cannot provide.

**Verification.** A clean-clone build of the FTU product. The same gates as the meter products plus the FTU-specific gates. A dependency graph showing the protection partition does not link meter domain code. A traceability matrix from safety goals to components. A CI time budget. `[clean-clone] ok`.

**Open questions.** Who owns the FTU product line? Does the utility require a separate certified repository baseline? Is the repo's CI time acceptable with the new product? Should the protection partition be a component with a `qualification: safety` tag? Is a package manager needed if the repo grows further?

### ADR-014 Time synchronization and time base for SOE and fault records

**Status.** proposed, 2026-11-23.

**Context.** SOE needs a common time base across FTUs. Fault records need timestamps. Distributed FA needs ordered messages. 61850 SV, if used, needs tight sync. The hardware doc's baseline is an external low-power RTC with battery or supercap backup, temperature compensation, alarms, and timestamps, with time sync from the master 101 or 104 and optionally a 1PPS or network time source [V hardware doc section 5.6]. The implementation plan's communication board includes a 北斗/GPS option and HSI-5 covers it [V implementation plan sections 4.2 and 4.4]. The standards matrix lists the DL/T 860 series but does not list a dedicated time-sync standard, so PTP and IRIG-B details are [B/C].

**Decision drivers.** Accuracy. Holdover. Cost. Antenna and lightning. Network support. Standard conformance. Record integrity.

**Options considered.**
1. 101 or 104 time sync from the master. Baseline and lowest cost. Accuracy is typically seconds, which is not enough for 1 ms SOE.
2. GNSS with BeiDou and GPS, 1PPS, and a disciplined TCXO or OCXO. Best accuracy and holdover. Needs an antenna, sky view, surge protection, and a BOM option.
3. IEEE 1588 PTP with hardware timestamping. Sub-microsecond over Ethernet, good for 61850. Needs network support and hardware.
4. SNTP or NTP over 4G. Cheap, but tens to hundreds of milliseconds and network dependent.
5. RTC only. Drifts. Unacceptable for SOE over days.

**Decision.** A time hierarchy. The baseline is 101 or 104 time sync from the master plus the external temperature-compensated RTC. If the tender requires 1 ms SOE across devices, add GNSS or BeiDou 1PPS, with a disciplined oscillator for holdover. If the network supports PTP, use PTP for 61850 and GOOSE-adjacent timing; otherwise GNSS 1PPS is the common reference. SNTP is a fallback. The time base is 64-bit UTC microseconds plus a monotonic tick. Local time and timezone are display-only. Every record carries a time-quality flag, synced, holdover, or unsynced, and the estimated error. The device never claims 1 ms SOE when the quality flag says otherwise. Leap seconds are handled at the time source. The monotonic tick is immune. A TCXO with 1 ppm drift reaches 1 ms in about 1000 s. A 0.1 ppm oscillator reaches it in about 2.8 h [E]. The holdover requirement chooses the oscillator. If the utility requires 1 ms SOE for 24 h without GNSS, the oscillator and the holdover budget must be specified, not assumed.

**Consequences.** A common time base for SOE, records, and FA. Honest time quality. A low-cost baseline and an optional high-accuracy path. The cost of the high-accuracy path is GNSS hardware, an antenna, surge protection, a sky view on the pole, PTP switch support, and possibly an expensive holdover oscillator. The time architecture is a system issue, not only firmware.

**Evidence level.** Hardware baseline [V hardware doc]. 101/104 and 61850 references [A/B per the matrix; A DL/T 634.5101-2022 per the 2026-11-23 research update]. GNSS and PTP [B/C]. Drift numbers [E]. SOE target [V performance doc T-S2].

**What evidence would overturn it.** A measured holdover that misses the SOE requirement. A utility that forbids GNSS on the pole. A network that supports PTP and makes GNSS unnecessary. A tender that accepts 10 ms SOE and makes the high-accuracy path unnecessary.

**Verification.** GNSS or BeiDou simulator. Time-interval analyzer. 1PPS accuracy. Holdover drift at temperature. Leap-second test. Timezone and DST. 101 and 104 time sync. PTP conformance. SOE resolution with a pulse generator. Record timestamp consistency across two devices. Loss-of-sync behavior.

**Open questions.** Is GNSS allowed on the pole, and who maintains the antenna? Does the utility require PTP or IRIG-B? What is the holdover requirement? Is 1 ms SOE required, or 10 ms? Who provides the time source?

---

## Hardware ADRs

### ADR-015 Breaker body and operating mechanism

**Status.** proposed, 2026-11-23.

**Context.** ZW6B is a 12 kV outdoor vacuum breaker. The mechanism is inside the sealed pole. The software only commands trip and close and reads status. The mechanism choice determines trip and close energy, timing, diagnostics, and the safety case. The hardware doc's HW-001 is spring mechanism baseline, permanent magnet alternative [V hardware doc section 10]. The standards matrix points to GB/T 1984-2024, DL/T 402-2016, Q/GDW 11253-2023, and 国网设备〔2021〕151号 [A].

**Decision drivers.** Reliability. Trip energy. Mechanical life. Temperature. Anti-pump. Manual operation. Cost. Maintenance.

**Options considered.**
1. Spring operating mechanism with a 24 V DC gear motor for charging, 48 V as an alternative. Mature. Mechanical energy storage. Anti-pump is straightforward. Trip and close coils. Manual trip and close. Needs a charging motor and a spring-charge sensor.
2. Permanent magnet mechanism. Fewer parts. Capacitor-driven. Fast. Capacitor aging, temperature, and a depleted capacitor can prevent opening. Needs capacitor health monitoring and a backup path.
3. Contractor-supplied mechanism. The FTU adapts to whatever the primary team selects. The software interface is the same.

**Decision.** Option 1, aligned with HW-001. The 24 V DC gear motor charges the spring. The trip and close coils are 24 V DC short-time loads from the supercap or battery. A 48 V DC variant is an alternative where the mechanism vendor requires it. The FTU monitors spring charge, motor current and time, trip and close coil continuity, and mechanism travel. It interlocks close on spring charged, control voltage normal, no fault, and no blocking condition. Hardwired anti-pump and anti-slow-close logic carries the safety function; the MCU monitors and records but does not carry it alone [V hardware doc HW-005]. The hardware doc's design targets are charge time ≤15 s, trip time ≤60 ms from output to contact separation, close time ≤100 ms, pole discrepancy ≤5 ms, and contact bounce ≤2 ms [目标 in hardware doc section 2.6]. These are design targets, not standard limits or acceptance values. If the tender selects option 2, the FTU adds capacitor health monitoring and a backup energy path, and the safety case is re-run. Option 2 is not accepted without that.

**Consequences.** The spring mechanism is proven and the failure modes are understood. The cost is a charging motor, a spring-charge sensor, and more mechanical parts. The FTU must handle the charging motor as a monitored load. The design targets must be confirmed by the mechanism vendor and the type test.

**Evidence level.** Mechanism baseline and targets [V hardware doc]. Standard identities [A per the matrix; A DL/T 2859-2024 as the complete-set main standard, verified research update]. Limits [C]. Selection [E].

**What evidence would overturn it.** A life test or temperature test that shows the spring mechanism cannot meet the operation count. A tender that specifies the permanent magnet mechanism. A cost or size constraint that forces option 2 with a proven capacitor monitor.

**Verification.** Mechanical life test. Trip and close time. Anti-pump test. Anti-slow-close test. Manual operation. Spring-charge failure injection. Coil continuity. HIL with the real mechanism. One-second and two-second fusion drive tests.

**Open questions.** Who owns the mechanism? What mechanical life and operation sequence are required? Is manual trip and close required? What is the ambient temperature range? Does the tender require permanent magnet?

### ADR-016 Sensor front end hardware

**Status.** proposed, 2026-11-23.

**Context.** The FTU needs three-phase currents, three-phase voltages, zero-sequence current, and possibly zero-sequence voltage. The hardware doc's HW-002 is protection and metering CT separate, with independent sampling chains [V hardware doc section 10]. The deep-fusion design embeds the sensors in the primary body. The protection CT is 5P10 as a starting point, but the 20 kA fault current is about 31.7 times the 630 A rated current, so 5P10 may saturate and cause refusal or delay [V hardware doc section 3.2]. The metering CT is 0.5S. The zero-sequence CT is core-balance. The PT is 10/0.22 kV or 10/0.1 kV, or a capacitive divider. The CT harvesting winding is separate from the protection CT [V hardware doc section 3.1]. Accuracy limits are [C]. The ranges are 0.05 to 20 In for protection current, 0.05 to 1.2 In for metering current, and 0 to 1.2 Un for voltage [目标 in hardware doc section 3.4].

**Decision drivers.** Accuracy. Dynamic range. Zero-sequence sensitivity. Isolation. Cost. Size. Standardization. Calibration. Safety.

**Options considered.**
1. Conventional electromagnetic CT and PT, 5 A or 1 A and 100 V, with a separate analog front end. Mature and accurate, but bulky and costly. The deep-fusion design usually embeds sensors.
2. Embedded LPCT or Rogowski current sensors and capacitive or resistive voltage dividers with a low-level analog interface. Compact and safe. The standard deep-fusion approach. Needs factory calibration and temperature compensation.
3. Hybrid. LPCT for current and a conventional PT for voltage. More parts.
4. Computed zero-sequence from the three phase currents instead of a dedicated core-balance CT. Cheaper, but less sensitive for small earth currents.

**Decision.** Option 2, aligned with HW-002. Protection and metering have separate CT cores and separate sampling chains. The protection CT must have an ALF calculated for 20 kA, the secondary burden, the DC component, and remanence. A 5P10 core is only a starting point; 5P20, 5P30, or a different core may be required [V hardware doc section 3.2]. The metering CT is 0.5S with its own burden and cable error budget. The zero-sequence CT is core-balance, with a ratio and sensitivity set by the grounding system and the earth-fault requirement. The PT or capacitive divider provides voltage measurement and can contribute to harvesting. The CT harvesting winding is separate from the protection CT. The protection input has a dedicated shorting structure so the CT secondary is shorted before the connector is opened. The PT secondary has a fuse or current-limiting resistor so it is never shorted. Calibration is multi-point, with CRC-protected coefficients and a redundant copy in EEPROM [V hardware doc sections 3.5 and 3.6]. Accuracy classes and burden are not set here; they come from Q/GDW 11253-2023, the target province's 一二次融合 检测规范, and the vendor calculation.

**Consequences.** Compact and safe sensors, good zero-sequence sensitivity, and a protection CT that does not saturate at the fault current. The cost is factory calibration, temperature compensation, a larger protection CT core, and a controlled connector with CT shorting. The metering chain is independent, so a protection CT change does not disturb the 0.5S calibration.

**Evidence level.** Chain separation and CT warning [V hardware doc]. Standard identities [A per the matrix; A DL/T 2859-2024 as the complete-set main standard, verified research update]. Accuracy limits [C]. Sensor architecture [E]. Ranges [目标].

**What evidence would overturn it.** A vendor calculation that shows a 5P10 core is sufficient at 20 kA with the actual burden. A tender that fixes conventional CT and PT. A zero-sequence CT that cannot fit the pole. A metering accuracy that requires a different CT class.

**Verification.** Accuracy and phase error at 1%, 5%, 20%, 100%, and 120% of rated. Temperature drift. DC offset. CT saturation at 20 kA. Zero-sequence sensitivity. Isolation test. CT open-circuit and PT short-circuit safety test. EMC. Calibration transfer. Comparison with a reference meter and relay.

**Open questions.** What protection CT ALF and secondary burden are required? What is the zero-sequence CT ratio and sensitivity? What is the system grounding mode? Is 3U0 available? Who calibrates the metering chain? Does the tender require conventional CT and PT?

### ADR-017 Power supply hardware architecture

**Status.** proposed, 2026-11-23.

**Context.** The pole has no reliable auxiliary supply. The hardware doc's HW-003 is dual harvesting plus battery plus supercap, and HW-008 is LiFePO4 plus supercap with low-temperature heating [V hardware doc section 10]. The 24 V bus is the control and energy storage bus, with 48 V as an alternative. The trip, close, and motor pulses draw from 24 V, or 48 V in the alternative variant. The electronics use isolated DC/DC to 12 V, 5 V, and 3.3 V [V hardware doc section 4.1]. The trip energy always comes from the supercap or battery, not from line harvesting. The backup targets are 24 h protection and standby, 4 h protection plus 4G, and 3 operations [目标 in hardware doc section 4.6]. The performance document records the DL/T 721-2024 candidate of 8 h battery or 30 min supercap and flags the 4 h design target as below the 8 h candidate [V performance doc section 5.3; B]. The stricter design combination is 24 h standby and 8 h communication.

**Decision drivers.** Availability. Trip energy. Backup time. Cold start. Battery life. CT range. Cost. Isolation. Safety.

**Options considered.**
1. CT harvesting only with a supercap. Works only with load current. Dies when the breaker is open or the line is unloaded.
2. CT plus PT or AC 220 V plus LiFePO4 plus supercap. Recommended and aligned with HW-003 and HW-008.
3. Battery-primary. Long backup, but battery life, temperature, and maintenance dominate.
4. External auxiliary supply. Only for a cabinet variant.

**Decision.** Option 2. The 24 V bus is the main control and storage bus, with 48 V as an alternative variant. The CT harvesting and the PT or AC 220 V input are OR-ed or ideal-diode isolated. The battery is LiFePO4 with a BMS for cell balancing, overcurrent, overtemperature, undervoltage, and low-temperature charge inhibit. The supercap handles the trip and close peak and the power-fail hold-up. A heater is controlled independently of the main controller. The battery and supercap have independent fuses, reverse-polarity protection, overcurrent protection, and precharge. The power-fail sequence is interrupt, save critical events and counters, notify the master, shut down non-critical loads, then park the outputs. The reset state has trip and close outputs de-energized. The exact thresholds, hold-up times, and backup durations are owned by the performance document. This ADR does not invent them.

**Consequences.** The device is available when the breaker is open and the source side is live. Protection does not depend on the battery. The trip path has local energy. The cost is more BOM, a battery maintenance item, a supercap aging item, a heater, and a power budget that must be measured. The 24 h standby and 8 h communication targets drive the battery size. The DL/T 721-2024 candidate is 8 h battery or 30 min supercap, and the 4 h design target is below that candidate. The product requirement must settle this before the battery is selected.

**Evidence level.** Topology and targets [V hardware doc]. Backup requirement [A DL/T 721-2024 and Q/GDW 10514-2018 per the matrix; B for the 8 h and 30 min values]. Energy numbers [E]. Hold-up and save path [V performance doc W-24 and W-25].

**What evidence would overturn it.** The PT cannot be source-side. The battery is not allowed on the pole. The CT cannot supply the required power at minimum current. The supercap cannot deliver the trip energy at end of life. The tender requires a backup time that LiFePO4 cannot meet at the temperature extremes.

**Verification.** Source-measure unit profile. Cold start at -40 ℃ and +70 ℃. CT and PT loss. Supercap end-of-life. Battery life and self-discharge. Brownout sweep. Trip at minimum rail. Heater test. EMC and surge. HIL.

**Open questions.** What backup time does the tender require, 24 h standby and 8 h communication, 4 h, or the DL/T 721-2024 8 h battery or 30 min supercap? Is the PT source-side or load-side? Is a battery allowed on the pole, and what is the replacement interval? What is the minimum CT current? Who owns the performance document's energy budget?

### ADR-018 FTU compute platform and metering AFE

**Status.** proposed, 2026-11-23.

**Context.** The compute platform must run protection deterministically and communication concurrently. The hardware doc's HW-004 is a new main controller platform; the repo's Cortex-M0 V85xxP or V8500 is not the FTU main controller and can only be a metering or communication companion [V hardware doc section 5.3]. The baseline is a single Cortex-M4F or Cortex-M33 with FPU, at least two UARTs, SPI, I2C, Ethernet MAC, and enough ADC channels. A dual-core Cortex-A plus real-time core is only for Linux, containers, or a rich HMI [V hardware doc section 5.2]. The metering AFE is three-phase 0.5S, at least six channels, with an internal DSP, SPI or UART, and calibration registers. The protection ADC is the main controller's internal ADC or an external synchronous ADC [V hardware doc section 5.4].

**Decision drivers.** Determinism. Isolation. Performance. Memory. Cost. Power. Supply chain. Certification.

**Options considered.**
1. Repo Cortex-M0 V85xxP or V8500. Reuse is high, but the compute, memory, and interfaces are insufficient for protection, TLS, 4G, and records.
2. Single Cortex-M4F, no TrustZone. Lowest cost and a proven ecosystem. Weaker isolation.
3. Single Cortex-M33 with TrustZone. Better secure boot and partition isolation. Ecosystem and cost risk.
4. Dual-core Cortex-M7 plus M4, or Cortex-A plus real-time core. More headroom for Linux or a rich HMI, but higher power, cost, and software complexity. Not the baseline.
5. Lockstep safety MCU. Overkill for distribution-grade. Higher cost and toolchain burden.

**Decision.** Option 2 or option 3, selected by the security and ecosystem evaluation. The baseline is a single main controller. The metering AFE is a separate three-phase 0.5S device. The protection ADC is the main controller's internal ADC if it meets simultaneity and ENOB; otherwise an external synchronous ADC. The performance document sets the stage budgets: Phase 1 96 KiB Flash and 24 KiB RAM, Phase 2 192 KiB Flash and 64 KiB RAM, with the upper limits equal to those values [V performance doc section 4.3]. The repository's 100 KiB rule conflicts with the Phase 2 target, so Phase 2 needs an ADR exception or a scope cut [V performance doc section 4.3 and section 8]. The MCU must have enough Flash and RAM to leave headroom above the Phase 2 budget for the metering and communication buffers. The repo's Cortex-M0 is not the main controller. If the product later needs Linux, containers, or a rich HMI, reopen option 4 with a dual-core SoC, but keep the protection partition on the real-time core.

**Consequences.** A single MCU with a separate metering AFE matches the hardware doc and keeps cost and power low. The protection and communication partitions share the main controller, so the isolation evidence is mandatory. The metering AFE is independent, so the 0.5S calibration and the protection chain do not interfere. The cost is a main controller with enough Flash, RAM, and Ethernet, and a separate AFE board.

**Evidence level.** Platform baseline [V hardware doc]. Memory [V performance doc section 4.3]. Standard identity [A Q/GDW 11815-2023 per the matrix]. Metering certification [C]. Selection [E].

**What evidence would overturn it.** A measured single-M4F or M33 design that cannot meet the CPU, RAM, or Flash budget. A security profile that requires TrustZone and an ecosystem that cannot supply it. A product requirement for Linux, containers, or a rich HMI that forces a dual-core SoC. A metering AFE that cannot be certified as required.

**Verification.** WCET and CPU profile. RAM and Flash map. Isolation and fault injection. EMC. Temperature. 61850 and 104 throughput. Boot time. BOM cost. Metering accuracy and calibration.

**Open questions.** M4F or M33? Is TrustZone required? Which metering AFE? What are the final Flash and RAM budgets? Is a dual-core cost-down or cost-up planned?

### ADR-019 Isolation, EMC, and protection circuits

**Status.** proposed, 2026-11-23.

**Context.** The FTU sits on a 12 kV pole with lightning, switching transients, EMC, and a long sensor cable. The hardware doc partitions the FTU into four supply domains: the high-voltage sensor domain, the 24 V control and storage domain with 48 V as an alternative, the 3.3 V digital domain, and the communication and IO isolation domain [V hardware doc section 5.10]. The analog front end, the trip and close outputs, and the communication cable are entry points. The hardware doc sets EMC target levels: ESD 4, radiated immunity 3, surge 4, conducted immunity 3, and power-frequency magnetic field 4 [目标 in hardware doc section 7.4]. The standards matrix lists GB/T 17626.2-2018, .3-2023, .5-2019, .6-2017, .8-2006, GB/T 4208-2017, GB/T 2423, and GB/T 11022-2020 chapter 4 [A/B]. The limits are [C].

**Decision drivers.** Safety. Accuracy. Immunity. Emissions. Surge. Creepage and clearance. Cost. Size.

**Options considered.**
1. Non-isolated analog front end with a common ground. Cheapest, but dangerous and inaccurate on a pole.
2. Isolated analog front end with isolation amplifiers or an isolated ADC, separate analog and digital grounds, TVS and common-mode chokes, and a shielded connector. Recommended and aligned with the hardware doc.
3. Fully isolated sensor electronics at the breaker with a digital interface. Best immunity, but adds electronics in the primary enclosure and a different connector.

**Decision.** Option 2. The CT and PT inputs are isolated with reinforced insulation for the 12 kV side. The analog and digital grounds are separate and single-point connected to the chassis. The connector has surge arresters, common-mode chokes, and a shield bonded at the enclosure entry. The trip and close outputs are opto-isolated with coil-current monitoring and readback. The PCB partitions analog, digital, and power, with a complete ground plane and isolation slots. The EMC target levels are the hardware doc's levels, not standard limits. The exact limits are in the tender and the purchased standard texts. The overcurrent comparator and trip driver are in the protected domain from ADR-002. The external watchdog and voltage monitor are placed so the firmware cannot disable them [V hardware doc section 5.9].

**Consequences.** A safe and accurate front end with a defined EMC target. The cost is isolation components, board area, an EMC pre-scan, and a four-domain power design. The isolation barrier must not compromise the analog accuracy. EFT, voltage dip, and damped oscillation are not in the current standards matrix and are TBD [V hardware doc section 7.4].

**Evidence level.** Domains and target levels [V hardware doc]. Standard identities [A/B per the matrix]. Limits [C]. Topology [E].

**What evidence would overturn it.** An EMC pre-scan that fails with the isolated front end. Isolation that adds too much error or cost. A sensor vendor that provides a digital output and removes the analog front end. A target province that requires a different EMC level.

**Verification.** EMC pre-scan and type test. Surge. ESD. Isolation test. Creepage and clearance. Thermal. Accuracy under EMC stress. Trip output under surge. Watchdog and voltage monitor fault injection.

**Open questions.** Which EMC levels and IP rating does the tender require? Are EFT and voltage dip required? Is the sensor interface analog or digital? Who owns the EMC test?

### ADR-020 Aviation connector and wiring interface

**Status.** proposed, 2026-11-23.

**Context.** The FTU connects to the breaker through the standardized 一二次融合 aviation connector. The 2026-11-23 research update fixes the baseline: the 26-pin connector defined by Q/GDW 11815-2023 Annex B/C, 国网设备〔2021〕151号, and the China EPRI 2026 入网 requirement Annex 4. The 37-pin variant is an early proposal and is not the baseline. Q/GDW 1519 is the distribution network operation and maintenance regulation, not the connector standard, and must not be cited as such [V research update, V hardware doc section 6.1]. The hardware doc still separates control, measurement, power, communication, and maintenance functions at board and cable level, with different shells and keying to prevent misconnection [V hardware doc section 6.2]. The official 26-pin pinout defines which signals cross the primary interface. The pin definitions are C until the official drawing is attached.

**Decision drivers.** Interoperability. Safety. Misconnection. Current rating. IP. Mating cycles. Serviceability. CT and PT safety.

**Options considered.**
1. Proprietary connector and pinout. Cheapest, but fails the standardized design and interoperability requirement.
2. Standardized 26-pin 一二次融合 connector per Q/GDW 11815-2023 Annex B/C, 151号, and CEPRI 2026 Annex 4. Recommended. The board-level control, measurement, power, and communication partition remains.
3. Separate terminal blocks. Easier to wire, but not sealed and not standardized.
4. The 37-pin variant. Early proposal, not the baseline. Only if the target province requires it with a written justification.

**Decision.** Option 2. The primary 一二次融合 interface is the 26-pin connector. The 26-pin pinout is controlled by Q/GDW 11815-2023 Annex B/C, 国网设备〔2021〕151号, and the CEPRI 2026 入网 requirement Annex 4, not invented here. The 37-pin variant is not the baseline; a deviation to 37-pin or to a province-specific pinout requires a written justification and the target province's approval. The board-level and cable-level partition into control, measurement, power, communication, and maintenance remains, but the official 26-pin pinout decides which signals cross the primary interface. The measurement path has a CT shorting feature so the secondary is shorted before the connector is opened. The power path separates AC and DC contacts, with independent battery and supercap circuits. The communication path separates Ethernet, RS-485, and antenna. The connectors are keyed, sealed, current-rated, and have a defined shield and service loop. The DI and DO counts are 8 to 16 DI and 4 to 8 DO, with isolated inputs and relay outputs [V hardware doc section 6.5]. The product does not use 4-20 mA. The pinout is configuration-controlled and tested for continuity in production.

**Consequences.** Interoperability with the standardized design and a safer service disconnect. The cost is a controlled pinout, multiple connector families, a cable assembly, and a production continuity test. A wrong pinout is a safety event, so the pinout is configuration-controlled. Until the official drawing is available, prototype connectors use temporary definitions with a clear label, and prototype hardware must not interconnect with field equipment [V hardware doc section 6.3].

**Evidence level.** Connector baseline [V 2026-11-23 research update: 26-pin, Q/GDW 11815-2023 Annex B/C, 151号, CEPRI 2026 Annex 4]. Complete-set standard [A DL/T 2859-2024, verified research update]. Hardware partition [V hardware doc]. Pinout [C until the official drawing]. Connector part [E until the hardware drawing].

**What evidence would overturn it.** The target province publishes a 37-pin or a different pinout with a written requirement. The 26-pin connector cannot meet the current, IP, or mating-cycle requirement. The primary team uses a different interface.

**Verification.** Mating and demating cycles. IP test. Current and temperature rise. Misconnection test. CT shorting test. Shield and EMC. Salt spray. Vibration. A pinout continuity test in production.

**Open questions.** Which target province's 26-pin variant applies? What is the connector part number and current rating? Does any province require the 37-pin early proposal, and what is the written justification? Is a service loop required? Who owns the cable assembly?

### ADR-021 Enclosure, thermal, and environment

**Status.** proposed, 2026-11-23.

**Context.** The device is outdoor, pole-mounted, and subject to sun, rain, salt spray, vibration, and temperature extremes. The hardware doc targets a 304 stainless steel or aluminum enclosure with a sun shield and a lock, FTU IP65, body and mated connector IP67, -40 to +70 ℃ operating, ≤95% humidity with condensation allowed, ≤2000 m altitude, and salt spray and damp heat per the target province [目标 in hardware doc section 7.1 and 7.2]. The standards matrix lists GB/T 4208-2017, GB/T 11022-2020 chapter 4, and GB/T 2423 [A/B]. The limits are [C].

**Decision drivers.** IP. UV. Salt spray. Temperature. Thermal. Condensation. Serviceability. Cost.

**Options considered.**
1. Sealed plastic enclosure. Cheap and good RF windows, but thermal and UV limitations.
2. Sealed 304 stainless steel or aluminum enclosure with a sun shield and external fins. Recommended and aligned with the hardware doc. Good thermal and robust, but heavier and needs a grounding point.
3. Ventilated enclosure with a fan. Better thermal, but not sealed. Contamination and condensation risk.

**Decision.** Option 2. The enclosure is 304 stainless steel or aluminum with a sun shield and a lock. The FTU is IP65, and the body and mated connector are IP67. The operating temperature target is -40 to +70 ℃. The enclosure is UV-resistant and salt-spray tested to the target province's level. The PCB has conformal coating and a controlled condensation path, with a breather, desiccant, or pressure-equalization element and a drain path. The thermal design covers the 4G peak, the power conversion, the main controller, the relays and coils, the heater, and solar radiation, including the worst-case combination [V hardware doc section 7.3]. The antenna has a sky view and surge protection. The thermal budget, internal ambient temperature, and junction margins are owned by the performance document. This ADR does not invent them.

**Consequences.** A robust outdoor enclosure with a defined IP and temperature target. The cost is weight, a grounding point, a thermal design, a chamber test, and a salt-spray test. The antenna placement and the battery compartment temperature are system issues. The battery is more sensitive than the electronics at low and high temperature.

**Evidence level.** Enclosure and targets [V hardware doc]. Standard identities [A/B per the matrix; A DL/T 2859-2024 as the complete-set main standard, verified research update]. Limits [C]. Thermal design [E].

**What evidence would overturn it.** A chamber test that exceeds the internal ambient or junction temperature. An aluminum enclosure that blocks RF or GNSS. A cost or weight target that forces plastic. A target province that requires a different IP or salt-spray level.

**Verification.** IP test. Thermal chamber at -40 ℃ and +70 ℃ with 4G peak, charging, and heating. Solar load. Salt spray. UV. Vibration and shock. Condensation. RF and GNSS performance. Drop and transport.

**Open questions.** What IP level, temperature range, and salt-spray duration? Is a heater needed in the FTU or only in the battery compartment? Where are the antennas mounted? Who owns the enclosure and thermal design?

---

## Performance ADRs

The performance document `docs/plans/zw6b-ftu-performance.md` (Draft, 2026-11-23) is the single source of truth for every number in this section. It has zero measured FTU values; its targets are P or L and must be filled by bench and HIL. The ADRs below cite its IDs. Where a value is not in that document, the ADR says so. No value here is an acceptance value without the stage evidence.

### ADR-022 Sampling and protection path performance

**Status.** proposed, 2026-11-23.

**Context.** The protection path must be deterministic. The choice between an interrupt and an RTOS task changes jitter, latency, and WCET. The performance document sets the chain: sample period 78.125 µs at 12.8 kHz, sampling ISR WCET ≤39 µs, sampling-to-criterion entry ≤78 µs, instantaneous criterion window ≤10 ms with a 20 ms limit, criterion computation ≤1 ms with a 5 ms limit, trip output ≤1 ms with a 2 ms limit, and sampling-to-coil ≤10 ms with a 20 ms limit [V performance doc T-P1, T-P3, T-P4, T-P5, T-P6, and T-P9]. It also sets interrupt latency ≤50 µs with a 100 µs limit, maximum interrupt-disable time ≤20 µs with a 50 µs limit, and worst-case CPU ≤70% with a 90% limit [V performance doc M-C2, M-C6, and M-C7].

**Decision drivers.** Determinism. Latency. Jitter. CPU. Simplicity. Testability.

**Options considered.**
1. ADC end-of-conversion interrupt runs sampling, DFT, protection, and trip decision. Recommended. Fixed priority, no scheduler, smallest jitter.
2. DMA block interrupt runs the protection task. Lower interrupt rate, but the block latency and DMA jitter are added.
3. RTOS task woken by the ADC. Easiest to integrate, but scheduling jitter and priority inversion risk.
4. Hardware protection only. Deterministic, but inflexible settings and no fault recording.

**Decision.** Option 1. The ADC conversion is timer-driven. The end-of-conversion interrupt reads the samples, runs the DFT and the protection element table, and sets the trip output. The ISR is highest priority and never blocks. The main loop handles self-test and housekeeping. The trip output is written at the end of the ISR. There is no dynamic allocation, no flash, no protocol code, and no RTOS call in the path.

**Performance budget.** From the performance document: sample period 78.125 µs, sampling ISR WCET ≤39 µs, sampling-to-criterion entry ≤78 µs, sampling-to-coil ≤10 ms target and ≤20 ms limit, criterion computation ≤1 ms with a 5 ms limit, trip output ≤1 ms with a 2 ms limit, interrupt latency ≤50 µs with a 100 µs limit, maximum interrupt-disable time ≤20 µs with a 50 µs limit, and worst-case CPU ≤70% with a 90% limit [V performance doc T-P1, T-P3 to T-P9, M-C2, M-C6, and M-C7]. The measurement method is a GPIO pulse, a logic analyzer, DWT CYCCNT, and a relay test set in S1 and S2 [V performance doc sections 5.2 and 7.1]. The regression rule is that a measured value above the baseline plus 10% fails even if it is still within the limit [V performance doc section 7.6].

**Consequences.** The lowest jitter and the simplest WCET argument. The cost is a hard real-time ISR that must stay small, and a main loop that must not block the ISR. The 12.8 kHz rate is 256 points per cycle and is high for an M4F. The 39 µs ISR WCET and the 20 µs maximum interrupt-disable time are the tightest constraints in the design and must be measured early.

**Evidence level.** Architecture [E]. Budgets [V performance doc T-P1 to T-P9 and M-C2, M-C6, M-C7]. Protection test methods [A GB/T 7261-2016 per the matrix]. Zero FTU measurements [V performance doc section 0].

**What evidence would overturn it.** A measured ISR WCET above 39 µs or a trip path above the 20 ms limit. A measured interrupt-disable time above 20 µs. A DMA path that adds less latency than the ISR. A sample rate or feature that cannot run in the ISR and the main loop.

**Verification.** GPIO toggle at ISR entry and exit. Logic analyzer. Injected waveform. DWT CYCCNT. WCET measurement. CPU load. Trip-time measurement. Long-run jitter histogram. HIL with a relay test set.

**Open questions.** Does the 39 µs ISR WCET hold on the chosen MCU at 12.8 kHz? Is 12.8 kHz per channel or aggregate? Who owns the WCET evidence? What is the fallback if the ISR budget is missed, lower the rate or move to a faster MCU?

### ADR-023 RTOS scheduling and priority performance

**Status.** proposed, 2026-11-23.

**Context.** The main controller runs FreeRTOS with 104, 61850, GOOSE, TLS, records, HMI, update, and 4G, while the protection path runs in the ADC ISR. Priorities and queue depths determine latency and failure modes. The performance document sets CPU average ≤50% with a 90% limit, worst-case CPU ≤70% with a 90% limit, protection task WCET ≤5 ms per 10 ms tick with a 10 ms limit, communication task WCET ≤5 ms per tick with a 10 ms limit, and task jitter ≤100 µs with a 1 ms limit [V performance doc M-C1, M-C2, M-C4, M-C5, and M-C8]. The 10 ms tick is the repository FreeRTOS model value; changing it requires the same document and the `check:rtos-model` change [V performance doc section 2].

**Decision drivers.** Deadline meeting. Priority inversion. Starvation. Simplicity. Testability.

**Options considered.**
1. All tasks at the same priority with time slicing. Simple, but unpredictable latency.
2. Fixed-priority preemptive with rate-monotonic assignment and priority inheritance on shared buses. Recommended.
3. Cooperative scheduling. Deterministic but fragile under blocking I/O.

**Decision.** Option 2. Priorities from high to low are the protection ISR, protocol receive and transmit, GOOSE, record worker, HMI and commissioning, and housekeeping. The protection ISR is above every task. The watchdog task is highest among tasks but only kicks when the health flags are set. Shared buses use mutexes with priority inheritance. Queues are bounded. The record worker drops with an event when full and never blocks the protocol. One task owns each shared object. The protection partition shares only immutable snapshots and bounded queues. The task latency, queue depth, and CPU budgets are the performance document values in M-C1, M-C2, M-C4, M-C5, and M-C8.

**Consequences.** Predictable latency and a clear ownership model. The cost is a priority and queue design that must be tested, a trace tool for deadline evidence, and a strict rule that no task disables interrupts long enough to delay the ADC ISR.

**Evidence level.** Architecture [E]. Budgets [V performance doc M-C1, M-C2, M-C4, M-C5, M-C8]. 10 ms tick [V `docs/design/freertos-thread-model.md`].

**What evidence would overturn it.** Measured deadline misses, priority inversion, or a simpler cooperative design that meets the deadlines. A task that blocks the ADC ISR beyond the jitter budget. A measured task WCET above 5 ms per tick or a task jitter above 1 ms.

**Verification.** Trace with SystemView, Percepio, or an equivalent. DWT CYCCNT. Stack high-water. Queue high-water. Deadline-miss counter. Soak test. Fault injection.

**Open questions.** Which RTOS trace tool? Does the 10 ms tick hold for the FTU? May 61850 MMS block the record worker? What is the maximum interrupt-disable time? What happens if the protection task WCET exceeds 5 ms per tick?

### ADR-024 Static memory and stack budget

**Status.** proposed, 2026-11-23.

**Context.** The protection partition must have no heap and a bounded stack. The communication partition uses fixed pools. The performance document sets the stage budgets: Phase 1 96 KiB Flash and 24 KiB RAM, Phase 2 192 KiB Flash and 64 KiB RAM, with the upper limits equal to those values [V performance doc section 4.3]. It gives an initial stack allocation of 11.5 KiB: protection 3 KiB, communication 3 KiB, application 2 KiB, storage 2 KiB, idle 0.5 KiB, and ISR and MSP 1 KiB [V performance doc section 4.2]. It requires no dynamic allocation after init; no `malloc` or `free` in the protection, interrupt, and communication paths; FreeRTOS heap only for tasks and queues; stack free ≥25%; and a canary failure is a test failure [V performance doc section 4.2]. The repo's allocator contract is user-allocated, opaque, and no mutable globals [V `docs/architecture.md` section 5, `AGENTS.md`]. The hardware doc says the repo's 24 KiB stategrid arena cannot be copied directly to the FTU [V hardware doc section 9.4].

**Decision drivers.** Determinism. Safety. Fragmentation. Stack overflow. ROM and RAM. Certification.

**Options considered.**
1. `malloc` and `free` everywhere. Convenient, but fragmentation and unbounded latency.
2. Static allocation and fixed pools, with a startup arena. Recommended. The protection partition has zero heap. The communication partition uses fixed-size pools and a startup arena.
3. Stack-only allocation. Simple, but large stacks and no dynamic records.

**Decision.** Option 2. The protection partition uses static objects and a startup arena. No `malloc`, no `free`, no variable-length arrays, no recursion. The communication partition uses a startup arena and fixed pools. No general-purpose heap. All stacks are sized from measured high-water marks plus margin. The linker map is checked. The protection ISR uses a read-only root pointer initialized once, with a `mutable_globals_exceptions.txt` entry containing the reason, expiry, and approver [V implementation plan section 12]. The repository's allocator pattern is reused, not the meter-specific sizes.

**Performance budget.** Phase 1 96 KiB Flash and 24 KiB RAM; Phase 2 192 KiB Flash and 64 KiB RAM; upper limits equal to the targets [V performance doc section 4.3]. The initial stack budget is 11.5 KiB, and the stack free must be ≥25% [V performance doc section 4.2]. The Phase 2 192 KiB budget needs the ADR-028 product-class approval or a scope cut. Measurement is ELF section, linker map, `-fstack-usage`, high-water marks, and canaries. The regression rule is that any growth over the frozen baseline fails [V performance doc section 7.6].

**Consequences.** No fragmentation and a bounded worst case. The cost is larger static footprints and a stack budget that must be measured. A library that requires a heap is rejected or wrapped in a fixed pool. The Phase 1 24 KiB RAM budget is tight: the 11.5 KiB stack plus the 2 KiB SOE ring leaves about 10.5 KiB for the application, protocol, and buffers [V performance doc section 4.2].

**Evidence level.** Repo pattern [V]. Stage and stack budgets [V performance doc sections 4.2 and 4.3]. Phase 2 approval [TBD, ADR-028].

**What evidence would overturn it.** A measured stack or pool high-water that exceeds the budget. A required protocol library that forces a heap. A Flash or RAM increase that needs a different MCU. A Phase 2 scope that does not fit 192 KiB even after the ADR-028 approval, or a Phase 1 stack high-water that leaves less than 25% free.

**Verification.** ELF section and linker map. `-fstack-usage`. Stack canaries. High-water marks. Allocation failure injection. Soak. Static analysis for recursion and VLA. `tools/check_ftu_firmware.py`. The S0 gate checks sizes and static frames; the S1 gate checks high-water marks on the target [V performance doc section 4.4].

**Open questions.** Which libraries require a heap? Does the Phase 2 192 KiB budget get the ADR-028 approval or a scope cut? Does the Phase 1 11.5 KiB stack plus 2 KiB SOE ring leave enough room? Who signs the stack budget?

### ADR-025 Communication stack performance

**Status.** proposed, 2026-11-23.

**Context.** The communication stack must meet the master's response and reporting deadlines. It shares the main controller with protection. The performance document sets: 101 9600 8N1 with a 32 B frame ≤33 ms, 104 over 4G Cat.1 with Cat.4 optional and SOE and command ≤1 s, Ethernet local ≤10 ms, GOOSE <10 ms, three connections target and four maximum, each ≤2 KiB and total ≤8 KiB, SOE event to master ≤1 s, command to master acknowledgment ≤1 s, link failure detection ≤3× timeout, reconnect ≤5 s, and TLS handshake CPU and latency TBD [V performance doc C-01 to C-18 and section 6.3]. OTA ≤5 min is an implementation plan target, not a performance document value [V implementation plan section 5.1]. The hardware doc sets 4G Cat.1 as the cost baseline and Cat.4 when large files or waveforms require it [V hardware doc section 5.8].

**Decision drivers.** Latency. Throughput. CPU. Memory. TLS cost. GOOSE determinism. FA latency.

**Options considered.**
1. Polling 104 only. Simple, but slow reporting and no GOOSE.
2. 104 plus 61850 MMS plus GOOSE with an RTOS and bounded queues. Recommended.
3. 61850 only. Cleaner model, but excludes 104 masters.

**Decision.** Option 2. The budget covers 104 response time, 101 response time, MMS report latency, GOOSE publish latency, TLS handshake time, 4G attach and reconnect time, FA end-to-end time, OTA time, and CPU and memory per protocol. The design rule is that GOOSE has a dedicated high-priority path and is not delayed by MMS, TLS, or record work. 104 and MMS share the lower-priority stack. The 4G link is treated as an unreliable transport with reconnect and buffering. The performance document values C-01 to C-18 are the authoritative targets. OTA ≤5 min is an implementation plan target and must be added to the performance document if it is a release gate.

**Consequences.** 104 and 61850 in one product, with GOOSE protected from lower-priority work. The cost is a larger main controller budget, a more complex priority design, and a performance test harness. The performance document's CPU average ≤50% and worst-case ≤70% cover protection and communication together; the communication task WCET ≤5 ms per tick is the binding constraint. TLS and MMS concurrency must be measured, not assumed.

**Evidence level.** Standard identities [A per the matrix]. Hardware interfaces [V hardware doc]. Budgets [V performance doc C-01 to C-18]. OTA [V implementation plan].

**What evidence would overturn it.** A measured GOOSE latency above 10 ms. A 104 SOE or command path above 1 s. A communication task WCET above 5 ms per tick. A 4G attach above 30 s. A master that does not require one of the protocols.

**Verification.** Traffic generator. Protocol analyzer. GOOSE latency histogram. 104 and 101 response time. MMS report time. TLS handshake. 4G attach and reconnect. FA end-to-end HIL. OTA time. CPU profile. Soak.

**Open questions.** Does the 104 SOE ≤1 s target hold on Cat.1? Is GOOSE over 4G allowed? What is the TLS handshake budget? Which 61850 edition and profile? Which 4G category does the first tender require? Is OTA ≤5 min a release gate?

### ADR-026 Power management and CT harvesting wake-up

**Status.** proposed, 2026-11-23.

**Context.** The device is CT and PT powered and must survive low load, loss of line, and long communication idle. Power management determines whether the device is awake when a fault occurs. The hardware doc baseline is a 24 V control bus, with 48 V as an alternative. The performance document W-01 to W-03 are 24 V DC: terminal body ≤100 mA, which is 2.4 W; idle with communication online ≤10 mA; sleep ≤200 µA. The rest of the performance document sets: whole machine ≤10 W target and ≤30 VA limit, wake from sleep to sampling ready ≤2 ms, wake source detection ≤100 µs, enter sleep ≤1 ms, hold-up ≥100 ms, save path ≤50 ms with an 80 ms limit, undervoltage detect and stop output ≤1 ms each, flush SOE ≤10 ms, and journal commit ≤20 ms [V performance doc W-01 to W-03, W-08, W-10 to W-12, and W-20 to W-25]. The hardware doc's backup targets are 24 h protection and standby, 4 h protection plus 4G, and 3 operations [目标 hardware doc section 4.6]. The performance document records the DL/T 721-2024 candidate of 8 h battery or 30 min supercap and the conflict with the 4 h design target [V performance doc section 5.3; B]. The stricter design combination is 24 h standby and 8 h communication.

**Decision drivers.** Availability. Wake-up latency. Energy. Battery life. False wake. Fault detection.

**Options considered.**
1. Always on. Simplest and highest energy. May not survive low CT current or the backup target.
2. Duty-cycled with CT and PT wake-up. Recommended. The protection front end stays alive. Communication and HMI sleep. A current or voltage threshold wakes the full system.
3. Deep sleep with a hardware comparator only. Lowest energy, but the protection algorithms are off and only the comparator trips.

**Decision.** Option 2. The protection MCU and the analog front end stay in a low-power armed state. The communication partition and 4G sleep or duty-cycle. A CT or PT threshold and the hardware comparator wake the protection path. The wake-up latency is bounded so a fault is not missed. The sleep current, wake-up time, and energy per report are the performance document values W-01 to W-03, W-08, and W-10 to W-12. The trip path remains hardware-backed in sleep. The backup-time requirement must be settled before the battery and supercap are sized. The 24 h standby and 8 h communication design targets, the 4 h hardware target, and the 8 h battery or 30 min supercap standard candidate are not the same requirement. PRD must close the choice.

**Consequences.** A low average power with protection armed. The cost is a wake-up design that must be tested, a false-wake budget, and a 4G duty cycle that must not miss a master command. The battery and supercap sizing depends on the final backup requirement. The 2 ms wake to sampling ready and the 100 ms hold-up are the binding power numbers.

**Evidence level.** Architecture [E]. Hardware targets [V hardware doc]. Budgets [V performance doc W-01 to W-03, W-08, W-10 to W-12, W-20 to W-25]. Backup conflict [V performance doc section 5.3].

**What evidence would overturn it.** A wake from sleep to sampling above 2 ms. A sleep current above 200 µA. A hold-up below 100 ms or a save path above 50 ms. A battery that cannot support the chosen backup target. A 24 h backup that makes the supercap-only path impossible.

**Verification.** Power analyzer. Source-measure unit. Wake-up latency with a current step. Low-load CT test. Battery life. False-wake count. Backup-time test. HIL.

**Open questions.** What is the minimum CT current? Does the 2 ms wake to sampling ready hold at 12.8 kHz? Is 4G always-on required? What is the final backup requirement, 24 h standby and 8 h communication, 4 h, or 8 h battery or 30 min supercap? What is the battery replacement interval?

### ADR-027 Boot time

**Status.** proposed, 2026-11-23.

**Context.** After a power loss or a watchdog reset, the device must be able to protect before a close command and must restore communication quickly. The performance document sets the boot chain: safety boot ≤100 ms with a 200 ms limit, firmware init ≤50 ms with a 100 ms limit, protection ready ≤150 ms with a 250 ms limit, local communication ready ≤250 ms with a 500 ms limit, module attach ≤30 s, master session ≤5 s after link, terminal-owned communication available ≤500 ms, and cold start to link ≤30 s with a <60 s limit [V performance doc T-B2 to T-B9]. The order invariant is protection ready before local communication ready before remote link. The hardware comparator path remains available even if the main controller is in reset or in the bootloader.

**Decision drivers.** Protection availability. Close interlock. Communication recovery. User experience. Flash size.

**Options considered.**
1. Boot the full communication stack before protection. Slow and unsafe.
2. Boot protection first, then communication. Recommended. The protection self-test completes before the close interlock is released. The communication stack starts in parallel or after.
3. Keep the protection MCU always powered from the battery. Avoids boot, but drains the battery and does not remove the reset case.

**Decision.** Option 2. The main controller boots from reset, runs a power-on self-test for the ADC, comparator, trip coil continuity, memory, watchdog, and voltage monitor, and arms protection before the close interlock is released. The communication stack starts after or in parallel with protection arming. The 4G attach and the protocol stack follow. The boot-time budget is the performance document's chain: 150 ms protection ready and 250 ms local communication ready, with 250 ms and 500 ms limits [V performance doc T-B4 and T-B5]. The design rule is that no close command is accepted until the protection self-test passes. The hardware comparator path covers failure to trip on high current during the boot window.

**Consequences.** Protection is available before close, and communication recovery is independent. The cost is a boot sequence that must be measured and a close interlock that must be tested under reset and power-fail. The 150 ms protection-ready target is tight for a full self-test and must be measured before it is accepted.

**Evidence level.** Architecture [E]. Budgets [V performance doc T-B2 to T-B9]. Hardware comparator [V hardware doc].

**What evidence would overturn it.** A measured protection-ready time that exceeds the 250 ms limit and the close interlock requirement. A communication boot that blocks the protection path. A self-test that cannot complete before the close command in the target sequence.

**Verification.** GPIO markers. Oscilloscope. Cold start at -40 ℃ and +70 ℃. Watchdog reset. Power-fail. Close interlock test. 4G attach time. `tools/check_ftu_firmware.py` for the image layout.

**Open questions.** Does the 150 ms protection-ready target hold with the full self-test? May a close command be accepted during boot? What is the 4G attach budget? Does the performance document keep the 30 s cold-start target?

### ADR-028 Platform ROM budget and the 100K rule conflict

**Status.** proposed, 2026-11-23. Not approved.

**Context.** The platform rule in `CLAUDE.local.md` says all targets must have ROM under 100K [V `CLAUDE.local.md`]. `AGENTS.md` states the IDIS ROM budget as 103 KiB [V `AGENTS.md`]. The repository's current stategrid ARM ELF already reports text 207,656 B after R2, about 202.8 KiB [V `docs/guides/port-binding.md` section 8.2]. The performance document sets the FTU Phase 1 Flash at 96 KiB and Phase 2 at 192 KiB, and it explicitly notes that Phase 2 exceeds the repository's 100 KiB rule and needs an ADR [V performance doc section 4.3 and section 8]. The implementation plan sets Phase 4 Flash at ≤480 KiB of a 1 MiB MCU and RAM at ≤160 KiB of 256 KiB [V implementation plan section 5.1]. The hardware document leaves the main controller Flash and RAM TBD but rejects the repo's Cortex-M0 as the FTU main controller [V hardware doc sections 5.2 and 5.3]. The 100K rule is therefore already not true for stategrid, and its scope is unclear. This is a platform and portfolio budget decision, not an FTU-only decision.

**Decision drivers.** Product feasibility. The 61850, waveform-recording, FA, and security feature set. Certification. BOM and power. CI enforceability. Consistency across products. Clarity of the rule. Existing obligations to IDIS and stategrid.

**Options considered.**
1. Keep the 100K rule for every target and cut FTU features: remove 61850/860, remove waveform recording, shrink the protocol stack, and keep only protection and 104. This may fit, but it changes the product. The 860 model, waveform records, and FA are differentiators in the slide's claim [V image].
2. Replace the single 100K rule with per-product-class budgets: IDIS 103 KiB, stategrid grandfathered at its measured size, FTU Phase 1 96 KiB, Phase 2 192 KiB, Phase 4 480 KiB, on a 1 MiB/256 KiB MCU. Each product has a budget file and a CI gate, and the default for new small products remains 100K. Requires product owner and architecture approval. Recommended.
3. Dual-chip or Linux plus MCU: keep the protection MCU under 100K and move 61850, records, and 4G to a second MCU or a Linux module. This changes the architecture, BOM, power, update, and security surface. The communication side still needs more than 100K, so the 100K rule applies only to the protection MCU. It is valid if the protection core must be separately qualified or the communication load cannot coexist, but it is not a budget workaround.
4. Keep the 100K rule and do not ship the FTU. Not acceptable for the product.

**Decision.** Option 2. The 100K rule is re-scoped to "per-product approved budget, default 100K for new small products." The IDIS class keeps 103 KiB. The existing stategrid text size is grandfathered at its measured value, with no growth without a budget update. The FTU is a new product class with Phase 1 96 KiB, Phase 2 192 KiB, and Phase 4 480 KiB targets on a 1 MiB/256 KiB MCU. The per-product budget is enforced by `tools/check_ftu_firmware.py` and a CI gate, not by a single global number. The rule change must be recorded in `CLAUDE.local.md` and `AGENTS.md` or in a checked-in platform budget policy, with the approval record. This decision is not approved yet. Until approval, the FTU may use the 1 MiB MCU for prototyping, but it may not claim the 192 KiB or 480 KiB budgets as accepted, and Phase 2 may not exit on the 192 KiB budget without the approval record or a scope cut.

**Consequences.** The rule matches the repository's actual state and the FTU can be built with its required feature set. Per-product budgets are enforceable and visible. The cost is a more complex platform rule, one budget file and approval per product, the 1 MiB MCU's cost and power, and a grandfather record for stategrid. If option 2 is rejected, the FTU must either cut 860, waveform recording, and the protocol stack under option 1, or move communication off the main controller under option 3. Both are product or architecture changes, not budget tweaks.

**Approval.** Product owner, architecture review, and platform owner. Names are TBD. The approval must be recorded before Phase 2 exit or before the 192 KiB budget is used, whichever comes first. The record must state the product, the phase budgets, the MCU memory, the stategrid grandfather baseline, and the expiry or review date. This ADR is not an approval.

**Evidence level.** Current rule and measured sizes [V `CLAUDE.local.md`, `AGENTS.md`, `docs/guides/port-binding.md` section 8.2]. FTU phase budgets [V performance doc section 4.3, V implementation plan section 5.1]. Recommendation [E]. Approval [TBD, not approved].

**What evidence would overturn it.** A measured FTU image that fits under 100K with the required 860, waveform, FA, and security features. A product decision to cut 860, waveform recording, and FA, which makes option 1 viable. A certification or security requirement for a separate protection MCU, which makes option 3 necessary. An already-approved product-class budget policy, which makes option 2 documentation rather than a new decision. A BOM or cost constraint that rules out the 1 MiB/256 KiB MCU. A measured stategrid text size that differs materially from the documented 207,656 B.

**Verification.** A per-product budget file. `tools/check_ftu_firmware.py` reads the ELF sections and checks the Phase 1 and Phase 2 budgets. A CI gate fails on budget growth. A clean-clone build of each product against its budget. The stategrid grandfather baseline is recorded as the measured text size. The approval record has a date, approver, scope, and review date. Phase 2 and Phase 4 exit evidence includes the budget verdict.

**Open questions.** Who approves the product-class budget: product owner, architecture review, platform owner, or all three? Is the 480 KiB Phase 4 value a target or a hard limit? Is the 1 MiB/256 KiB MCU approved by sourcing and cost? Does the 100K default apply only to new IDIS-class products? Is option 3 needed for certification or security independent of the budget? What is the stategrid growth policy under the grandfather record?

---

## Open decisions

The following are not closed by this ADR set and must be resolved before the design freezes. They are tracked in the hardware and implementation plans.

1. M4F or M33, and whether TrustZone is required [ADR-002, ADR-018].
2. Protection CT ALF and secondary burden at 20 kA [ADR-016].
3. Zero-sequence CT ratio and system grounding mode [ADR-016].
4. Final backup requirement: 24 h standby and 8 h communication, 4 h, or the DL/T 721-2024 8 h battery or 30 min supercap [ADR-005, ADR-017, ADR-026].
5. 61850 edition and 90-6 or 90-12 distribution profile [ADR-006].
6. Target province's 26-pin connector pinout and any approved 37-pin deviation [ADR-020].
7. Performance document owner, version, and freeze date; add the Phase 4 budget and the OTA budget [ADR-022 to ADR-028].
8. Final Flash, RAM, power, trip-time, SOE, and boot budgets; Phase 4 480 KiB and 1 MiB MCU approval [ADR-022 to ADR-028].
9. Billing certification scope [ADR-001].
10. Whether the protection partition needs a separate qualification repository [ADR-013].
11. Platform ROM budget approval under ADR-028, not yet approved.

## Residual risks

The standards matrix is a scope and identity check, not a clause-level compliance review. Every limit cited as [C] must be verified against the purchased standard text and the target province's tender. DL/T 2859-2024 and the DL/T 634.5101-2022 edition come from the 2026-11-23 research update and should be added to the standards matrix. The hardware document is a design input, not a frozen specification. The performance document is a draft with zero measured FTU values; its targets are P or L and must be filled by bench and HIL. Its W-01 to W-03 are 24 V DC, matching the hardware doc baseline. No timing, power, accuracy, thermal, or EMC number has been measured. The image is a product poster, not a datasheet. The two largest architecture risks are the single-MCU partition's freedom from interference and the protection CT saturation at 20 kA. The third is the 26-pin connector pinout, which is a safety contract and must come from the official controlled document. The fourth is the power topology when the breaker is open; if the PT is on the load side, the FTU cannot be alive to receive a close command. The fifth is the platform ROM budget: the 100K rule is already exceeded by stategrid, and the FTU 192 KiB and 480 KiB budgets are not approved.
