#include "stdafx.h"
#include "RpcServices.h"
#include <RCF/JsonRpc.hpp>

#include "RpcServiceHub.h"
#include "RpcServiceSearch.h"

void RpcServices::transfers(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response)
{

}

/**
 * -
 * Request params: array("type", {object})
 */
void RpcServices::hub(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response)
{
    prepareFailure(ERR_UNKNOWN_ERROR, response);
    const json_spirit::Array &params = request.getJsonParams();    
    
    if(params.size() != 2){
        //throw Exception("..."); - catch in RcfServer:938
        prepareFailure(ERR_PARAM_COUNT_DIFFERENT, response);
        return;
    }

    if(params[0].type() != json_spirit::int_type || params[1].type() != json_spirit::obj_type){
        prepareFailure(ERR_PARAM_TYPE_INCORRECT, response);
        return;
    }

    const int type                  = params[0].get_int();
    const json_spirit::Object &data = params[1].get_obj();

    enum TypeAllow
    {
        /* list fav */
        TYPE_LIST,
        /* current links */
        TYPE_USED,
        /* quick / fav. */
        TYPE_CONNECT,
        TYPE_CLOSE,
        TYPE_CREATE,
        TYPE_UPDATE,
        TYPE_DELETE,
        /* getting custom menu from the connected hub */
        TYPE_MENU
    };
    
    switch(type)
    {
        case TYPE_USED:
        {
            handlerStringResult(RpcServiceHub::used(data), response);
            return;
        }
        case TYPE_CONNECT:
        {
            handlerBooleanResult(RpcServiceHub::connect(data), response);
            return;
        }
        case TYPE_CLOSE:
        {
            handlerBooleanResult(RpcServiceHub::close(data), response);
            return;
        }
        case TYPE_LIST:
        {
            handlerStringResult(RpcServiceHub::list(data), response);
            return;
        }
        case TYPE_CREATE:
        {
            handlerBooleanResult(RpcServiceHub::create(data), response);
            return;
        }
        case TYPE_UPDATE:
        {
            handlerBooleanResult(RpcServiceHub::update(data), response);
            return;
        }
        case TYPE_DELETE:
        {
            handlerBooleanResult(RpcServiceHub::remove(data), response);
            return;
        }
        case TYPE_MENU:
        {
            prepareFailure(ERR_UNSUPPORTED_FUNCTION, response); //TODO:
            return;
        }
        default:{
            prepareFailure(ERR_OPERATION_TYPE_INCORRECT, response);
            return;
        }
    }
}

/**
 * -
 * Request params by type: 
 *       - {object} array(RESPONSE [, count])
 *       - bool     array(DEFAULT, "query string")
 *       - bool     array(TTH, "base32-encoded string")
 *       - bool     array(COMMAND, {object})
 */
void RpcServices::search(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response)
{
    prepareFailure(ERR_UNKNOWN_ERROR, response);
    const json_spirit::Array &params = request.getJsonParams();

    if(params[0].type() != json_spirit::int_type){
        prepareFailure(ERR_PARAM_TYPE_INCORRECT, response);
        return;
    }

    enum TypeAllow
    {
        /* listen answer */
        RESPONSE,
        /*  simple query */
        DEFAULT,
        /* request by tth */
        TTH,
        /* commands after start */
        COMMAND
        /* additional features: ADC - SCH {AN, NO, EX}, separation of words & etc., */
        //SPECIFIC
    };

    const int type = params[0].get_int();
    switch(type)
    {
        case RESPONSE:
        {
            if(params.size() == 2){
                if(params[1].type() == json_spirit::int_type){
                    handlerStringResult(RpcServiceSearch::result(params[1].get_int()), response);
                    return;
                }
                prepareFailure(ERR_PARAM_TYPE_INCORRECT, response);
                return;
            }
            handlerStringResult(RpcServiceSearch::result(), response);
            return;
        }
        case DEFAULT: case TTH:
        {
            if(params[1].type() != json_spirit::str_type){
                prepareFailure(ERR_PARAM_TYPE_INCORRECT, response);
                return;
            }
            handlerBooleanResult(RpcServiceSearch::simpleSearch(params[1].get_str(), (type == TTH)? true : false), response);
            return;
        }
        case COMMAND:
        {
            if(params[1].type() != json_spirit::obj_type){
                prepareFailure(ERR_PARAM_TYPE_INCORRECT, response);
                return;
            }
            handlerBooleanResult(RpcServiceSearch::command(params[1].get_obj()), response);
            return;
        }
    }
    prepareFailure(ERR_OPERATION_TYPE_INCORRECT, response);
    return;
}

void RpcServices::queue(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response)
{

}

void RpcServices::share(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response)
{

}

void RpcServices::settings(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response)
{

}

void RpcServices::execute(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response)
{

}

void RpcServices::hashing(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response)
{
    
}

inline void RpcServices::prepareSuccess(const std::string &result, RCF::JsonRpcResponse &response)
{
    json_spirit::mObject &ret = response.getJsonResponse();
    ret["error"]  = json_spirit::mValue();
    if(!result.empty()){
        ret["result"] = result;
    }
    else{
        ret["result"] = json_spirit::mValue();
    }
}

inline void RpcServices::prepareFailure(int error, RCF::JsonRpcResponse &response)
{
    json_spirit::mObject &ret = response.getJsonResponse();
    ret["error"]  = error;
    ret["result"] = json_spirit::mValue();
}

inline void RpcServices::handlerBooleanResult(bool result, RCF::JsonRpcResponse &response)
{
    if(!result){
        prepareSuccess("false", response);
        return;
    }
    prepareSuccess("true", response);
    return;
}

inline void RpcServices::handlerStringResult(const std::string &result, RCF::JsonRpcResponse &response)
{
    prepareSuccess(result, response);
}

RpcServices::RpcServices(void){}
RpcServices::~RpcServices(void){}