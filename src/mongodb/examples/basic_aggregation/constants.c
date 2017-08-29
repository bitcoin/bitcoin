const char *const COLLECTION_NAME = "things";

/* Our map function just emits a single (key, 1) pair for each tag
   in the array: */
const char *const MAPPER = "function () {"
                           "this.tags.forEach(function(z) {"
                           "emit(z, 1);"
                           "});"
                           "}";

/* The reduce function sums over all of the emitted values for a
   given key: */
const char *const REDUCER = "function (key, values) {"
                            "var total = 0;"
                            "for (var i = 0; i < values.length; i++) {"
                            "total += values[i];"
                            "}"
                            "return total;"
                            "}";
/* Note We can't just return values.length as the reduce function
   might be called iteratively on the results of other reduce
   steps. */
