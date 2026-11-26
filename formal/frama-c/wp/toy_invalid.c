/* WP wiring probe: WP must report the goal invalid. No memory, no
 * quantifiers. */
/*@ ensures \result == 1; */
int toy_invalid(void)
{
    return 0;
}
