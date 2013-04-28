#pragma once
#include "json_spirit.h"

class RpcServiceSearch
{
public:
    static std::string result(int count = 500);
    static bool simpleSearch(const std::string &query, bool isHash);
    static bool command(const json_spirit::Object &data);

private:
    /**
     * Экрнирование и устранение нежелательных символов.
     */
    static std::string safeString(std::string str);

    RpcServiceSearch(void){};
    ~RpcServiceSearch(void){};
};

