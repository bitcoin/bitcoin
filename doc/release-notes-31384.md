- Node and Mining

---

- PR #31384 fixed an issue where the coinbase transaction weight was being reserved in two different places.
 The fix ensures the reservation happens in one place, but does not change the previous behaviour by setting the default
 to the previous value of `8,000`.
 However, this PR offers the advantage of overriding the default with a user-desired value and allow creating blocks
 with weights more than `3,992,000`.

- Bitcoin Core will now **fail to start** if the `-blockmaxweight` parameter is set higher than the consensus limit of `4,000,000` weight units.