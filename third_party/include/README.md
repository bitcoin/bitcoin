1. To prevent displaying warnings/errors by compiler, add
    ```
    #pragma clang system_header
    #pragma GCC system_header
    ```

2. Rename Verify macro inside fakeit.hpp, as it clashes with BTC's Verify function.