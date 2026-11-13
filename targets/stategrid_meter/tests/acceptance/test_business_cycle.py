from __future__ import annotations

from cases import case

EXPECTED_STAGES = [
    "SecTick_Task",
    "Rtc_Prossecc",
    "TouFeePro",
    "Electrical_measurement",
    "Energy_Pluse_Count",
    "Energy_Meter",
    "freeze_pro",
    "event_pro",
    "Key_Display",
    "Display",
    "backlight_pro",
    "do_relay",
    "relay_led_ctl",
    "IOStateCheck",
]


@case(
    "ACC-CYCLE-001",
    "one MSG_SECOND reaches the meter application and runs every business stage",
    ("business", "smoke"),
)
def one_business_cycle(ctx):
    run = ctx.run(cycles=1, verbose=True)

    assert run.exit_code == 0, f"firmware_sim exited {run.exit_code}"
    missing = [stage for stage in EXPECTED_STAGES if stage not in run.stages]
    assert not missing, f"missing business stages: {missing}"
    duplicated = [stage for stage in EXPECTED_STAGES if run.stages.count(stage) != 1]
    assert not duplicated, f"business stages did not run exactly once: {duplicated}"
    assert any(
        (hit.pipeline, hit.filter_id, hit.ret) == ("app_meter_base", 0, 0)
        for hit in run.filters
    ), f"no successful app_meter_base filter in {run.filters}"
    assert any(
        (message.source, message.target, message.control, message.length)
        == (0x0D, 0x16, 0x01, 0)
        for message in run.messages
    ), f"no RTC message in {run.messages}"
    assert not any("NVIC_SystemReset" in line for line in run.trace)
