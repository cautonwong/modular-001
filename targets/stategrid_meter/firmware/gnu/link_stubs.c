/*
 * Link-only stubs for symbols normally supplied by the IAR runtime.
 * They are intentionally no-ops: the GNU image is a link check, not a
 * validated replacement runtime.
 */
void __CALL(int value)
{
    (void)value;
}
