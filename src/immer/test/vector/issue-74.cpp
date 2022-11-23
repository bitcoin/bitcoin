#include "immer/box.hpp"
#include "immer/set.hpp"
#include "immer/vector.hpp"

#include <functional>

struct my_type
{
    using container_t = immer::vector<immer::box<my_type>>;
    using func_t      = std::function<int(int)>;

    int ival;
    double dval;
    func_t func;
    container_t children;
};

int main()
{
    my_type::container_t items = {my_type()};
    immer::set<int> items2;
    auto items3 = items2.insert(10);
    return 0;
}
