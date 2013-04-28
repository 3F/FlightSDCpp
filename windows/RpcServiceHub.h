#pragma once
#include "json_spirit.h"

class RpcServiceHub
{
public:
    static bool create(const json_spirit::Object &data);
    static bool update(const json_spirit::Object &data);
    static bool remove(const json_spirit::Object &data);
    static bool connect(const json_spirit::Object &data);
    static bool close(const json_spirit::Object &data);
    static std::string used(const json_spirit::Object &data);
    static std::string list(const json_spirit::Object &data);
    
private:
    static void prepareHubFields(const json_spirit::Object &data, FavoriteHubEntry &entry);

    RpcServiceHub(void){};
    ~RpcServiceHub(void){};
};