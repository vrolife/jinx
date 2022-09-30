#include <jinx/cjson/cjson.hpp>

using namespace jinx::cjson;

int main(int argc, char *argv[])
{
    JSON_Object root{cJSON_Parse("{}")};
    return 0;
}
