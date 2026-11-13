# ESAM 兼容性验证

状态：主机侧策略验证通过，未接真实 ESAM，未做硬件运行验证。

## 验证矩阵

| 场景 | 输入条件 | 期望结果 | 当前验证 |
|---|---|---|---|
| ESAM 状态错误 | `esam_err_no != 0x9000` | 拒绝 | `test_esam_result` |
| SID / SID_MAC | `verification_type = 0` | 由 ESAM 内部校验，应用层不比较 MAC | 主机策略 |
| RN_MAC 正确 | 双方 MAC 长度 >= 4，前 4 字节相同 | 接受 | `test_esam_result` |
| RN_MAC 错误 | 双方 MAC 长度 >= 4，前 4 字节不同 | 拒绝 | `test_esam_result` |
| 纯 RN | 双方都没有 MAC 字段 | 接受，不误判 | `test_esam_result` |
| MAC 缺失或过短 | 一方有 MAC 字段，另一方没有或长度 < 4 | 拒绝 | `test_esam_result` |

## 已运行

```sh
cmake -S tests -B tests/build
cmake --build tests/build -j4
ctest --test-dir tests/build --output-on-failure
```

结果：48/48 通过。

## 仍需硬件完成

1. 真实 ESAM 返回正确 MAC 时，RN_MAC 和 SID_MAC 请求应通过。
2. 人为篡改一字节 MAC 时，请求应被拒绝并返回 `DAR_UNAUTHORIZED`。
3. 纯 RN 请求在无 MAC 字段时仍应通过。
4. 上报确认使用 `check_upreport_esam_rn_mac` 的路径应保持原有行为。
5. 连续认证、断线重连、掉电重启后的重试状态应保持一致。

硬件验证前不要把 `enforce_mac` 再关掉；若现场报文确实不带 MAC，应先确认是协议版本问题，而不是直接放宽校验。
