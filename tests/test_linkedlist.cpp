#include <jinx/assert.hpp>
#include <jinx/linkedlist.hpp>

using namespace jinx;

struct mynode : public LinkedList<mynode>::Node
{
    int _i;
    explicit mynode(int val) : _i(val) { }
};

int main(int argc, const char* argv[])
{
    LinkedList<mynode> list;
    mynode node1{0};
    mynode node2{1};
    mynode node3{2};

    jinx_assert(list.push_back(&node1).is(Successfu1));
    jinx_assert(list.push_back(&node2).is(Successfu1));
    jinx_assert(list.push_back(&node3).is(Successfu1));

    jinx_assert(list.erase(&node2).is(Successfu1));
    jinx_assert(list.size() == 2);
    
    jinx_assert(list.erase(&node1).is(Successfu1));
    jinx_assert(list.erase(&node3).is(Successfu1));
    jinx_assert(list.empty());

    jinx_assert(list.push_front(&node3).is(Successfu1));
    jinx_assert(list.push_front(&node2).is(Successfu1));
    jinx_assert(list.push_front(&node1).is(Successfu1));
    jinx_assert(list.size() == 3);

    int idx = 0;
    list.for_each([&](mynode* node) {
        jinx_assert(node->_i == idx);
        ++idx;
    });

    jinx_assert(list.erase(&node1).is(Successfu1));
    jinx_assert(list.erase(&node2).is(Successfu1));
    jinx_assert(list.erase(&node3).is(Successfu1));
    jinx_assert(list.empty());
    return 0;
}
