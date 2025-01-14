- Node and Mining

---

- **PR #31384** fixed an issue where block reserved weight for fixed-size block header, transactions count,
  and coinbase transaction was done in two separate places.
  Before this pull request, the policy default for the maximum block weight was `3,996,000` WU, calculated by
  subtracting `4,000 WU` from the `4,000,000 WU` consensus limit to account for the fixed-size block header,
  transactions count, and coinbase transaction. During block assembly, Bitcoin Core clamped custom `-blockmaxweight`
  value to not be more than the policy default.

  Additionally, the mining code added another `4,000 WU` to the initial reservation, reducing the effective block template
  size to `3,992,000 WU`.

  Due to this issue, the total reserved weight was always `8,000 WU`, meaning that even when specifying a `-blockmaxweight`
  higher than the policy default, the actual block size never exceeded `3,992,000 WU`.

  The fix consolidates the reservation into a single place and introduces a new startup option,
  `-blockreservedweight` (default: `8,000 WU`). This startup option specifies the reserved weight for
  the fixed-size block header, transactions count, and coinbase transaction.
  The default value of `-blockreservedweight` was chosen to preserve the previous behavior.

  **Upgrade Note:** The default `-blockreservedweight` ensures backward compatibility for users who relied on the previous behavior.

  Users who manually set `-blockmaxweight` to its maximum value of `4,000,000 WU` should be aware that this
  value previously had no effect since it was clamped to `3,996,000 WU`.

  Users lowering `-blockreservedweight` should ensure that the total weight (for the block header, transaction count, and coinbase transaction)
   does not exceed the reduced value.

  As a safety check, Bitcoin core will **fail to start** when `-blockreservedweight` init parameter value is lower than `2000` weight units.

  Bitcoin Core will also **fail to start** if the `-blockmaxweight` or `-blockreservedweight` init parameter exceeds
  consensus limit of `4,000,000` weight units.
