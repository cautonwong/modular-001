/* WP wiring probe: Qed alone does not prove it, so the pinned external solver
 * must return valid. No memory, no quantifiers. */
/*@ requires x >= 0;
    ensures \result >= 0; */
int toy_valid(int x)
{
    return x * x;
}
