#include "stdafx.h"
#include "RpcServiceHub.h"
#include "json_spirit_utils.h"
#include "../client/FavoriteManager.h"
#include "HubFrame.h"
#include "MainFrm.h" //onSpeaker., TODO:~

// TODO: create/update/delate - place to new thread

bool RpcServiceHub::create(const json_spirit::Object &data)
{
    const std::string &server = json_spirit::find_value(data, "server").get_str();
    if(FavoriteManager::getInstance()->getFavoriteHubEntry(server)){
        return false; //already exist
    }

    FavoriteHubEntry entry;
    entry.setServer(server);
    prepareHubFields(data, entry);
    
    FavoriteManager::getInstance()->addFavorite(entry);
    return true;
}

bool RpcServiceHub::update(const json_spirit::Object &data)
{
    const std::string &server   = json_spirit::find_value(data, "server").get_str();
    FavoriteHubEntry *entry     = FavoriteManager::getInstance()->getFavoriteHubEntry(server);
    if(!entry){
        return false;
    }

    json_spirit::Pair_impl<json_spirit::Config_vector<std::string>>::Value_type addr = json_spirit::find_value(data, "serverNew");
    if(addr.type() != json_spirit::null_type){
        if(addr.get_str().length() > 0){
            entry->setServer(addr.get_str());
        }
    }
    prepareHubFields(data, *entry);

    FavoriteManager::getInstance()->save();
    return true;
}

bool RpcServiceHub::remove(const json_spirit::Object &data)
{
    const std::string &server       = json_spirit::find_value(data, "server").get_str();
    const FavoriteHubEntry *entry   = FavoriteManager::getInstance()->getFavoriteHubEntry(server);
    if(!entry){
        return false;
    }
    FavoriteManager::getInstance()->removeFavorite(entry);
    return true;
}

bool RpcServiceHub::connect(const json_spirit::Object &data)
{
    json_spirit::Pair_impl<json_spirit::Config_vector<std::string>>::Value_type addr = json_spirit::find_value(data, "server");
    if(addr.type() != json_spirit::null_type && addr.get_str().length() > 0){
        //TODO: лучше разделить на отдельные потоки, если клиент посылает множетсвенные запросы
        //     Автостарт у них также отрабатывает в едином - см. MainFrm::onSpeaker(AUTO_CONNECT) autoConnect(...); !
        PostMessage(MainFrame::getMainFrame()->m_hWnd, 
                    WM_SPEAKER, MainFrame::QUICK_CONNECT_HUB, (WPARAM)new std::string(addr.get_str()));
        return true;
    }
    return false;
}

bool RpcServiceHub::close(const json_spirit::Object &data)
{
    const std::string &server = json_spirit::find_value(data, "server").get_str();
    return HubFrame::closeHubByAddr(Text::toT(server));
}

/**
 * @response: array(object, object,..)
 */
std::string RpcServiceHub::list(const json_spirit::Object &data)
{
    std::string ret = "[";
    const FavoriteHubEntry::List& iHubs = FavoriteManager::getInstance()->getFavoriteHubs();
    for(auto idx = iHubs.cbegin(); idx != iHubs.cend(); ++idx){
		FavoriteHubEntry* entry = *idx;
        ret += "{server:'" + entry->getServer() + "',name:'" + entry->getName() + "',description:'" + entry->getDescription() + "'},";
    }
    //return ret.substr(0, ret.length()) + "]";
    return ret += "]"; //yep
}

std::string RpcServiceHub::used(const json_spirit::Object &data)
{
    std::map<std::string, bool> list;
    HubFrame::listOpenedHubs(list);
        
    std::string ret = "{";
    for(auto it = list.cbegin(); it != list.end(); it++){
        std::string server = it->first;
        size_t pos = server.find("://");
        if(pos != std::string::npos){
            server = server.substr(pos + 3);
        }
        ret += "'" + server + "':" + ((it->second)? "1":"0") + ",";
        //TODO: адреса fav возвращать в виде ID
    }
    //return ret.substr(0, ret.length()) + "}";
    return ret += "}"; //yep
}

void RpcServiceHub::prepareHubFields(const json_spirit::Object &data, FavoriteHubEntry &entry)
{
    const std::string &caption      = json_spirit::find_value(data, "caption").get_str();
    const std::string &description  = json_spirit::find_value(data, "description").get_str();

    entry.setName(caption);
    entry.setDescription(description);
    entry.setEncoding("Russian_Russia.1251");
    entry.setGroup(Util::emptyString);
    entry.setMode(0);
    //entry.setExclusiveHub(false); // send H:1/0/0 or similar
}