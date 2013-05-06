#pragma once
#include <RCF/RCF.hpp>

namespace RpcServicesTypes
{
    namespace ErrorCodes
    {
        enum ServiceErrorCodes
        {
            ERR_PARAM_TYPE_INCORRECT        = 100,
            ERR_PARAM_COUNT_DIFFERENT       = 101,
            ERR_PARAM_UNKNOWN_ARG           = 102,
            ERR_PARAM_KEYARG_NOTFOUND       = 103,
            ERR_OPERATION_TYPE_INCORRECT    = 200,
            ERR_UNKNOWN_ERROR               = 500,
            ERR_UNSUPPORTED_FUNCTION        = 700
        };
    };

    namespace ServiceSearch
    {
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

        namespace LabelFile
        {
            enum
            {
                /* default no label */
                WITHOUT_LABEL,
                WAITING_DOWNLOAD,
                DOWNLOADED_PART,
                DOWNLOADED_COMPLETE,
                ALREADY_IN_SHARE,
                /* manually of user */
                EXCLUDED
            };
        };

        namespace LabelUser
        {
            enum{};
        };
    };

    namespace ServiceHub
    {
        enum TypeAllow
        {
            /* list fav */
            LIST,
            /* current links */
            USED,
            /* quick / fav. */
            CONNECT,
            CLOSE,
            CREATE, UPDATE, REMOVE,
            /* getting custom menu from the connected hub */
            MENU
        };
    };
};


/**
 * Удволетворение основных потребностей.
 */
class RpcServices
{
public:
  /* display of current transfers -> similar to the queue */
    void transfers(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);

  /* hub operation: CRUD of fav & list of connected with main operation */
    void hub(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);

  /* search operation */
    void search(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);

  /* queue operation: Uploads/Downloads, Complete/incomplete, CRUD op,  */
    void queue(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);
  
  /* viewing from users and sharing operation */
    void share(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);

  /* available settings: speed limit, connection settings, etc., */
    void settings(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);

  /* loader applications and libraries */
    void execute(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);

  /* hashing operation: calculate, status */
    void hashing(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);

    //INFO: RCF скуп на привязывания, поэтому лучше сводить схожие операции
    //void uploads(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);
    //void downloads(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);
    //// combination of uploads / downloads
    //void transfers(const RCF::JsonRpcRequest &request,  RCF::JsonRpcResponse &response);

    RpcServices(void);
    ~RpcServices(void);

private:
    void prepareSuccess(const std::string &result, RCF::JsonRpcResponse &response);
    void prepareFailure(int error, RCF::JsonRpcResponse &response);
    void handlerBooleanResult(bool result, RCF::JsonRpcResponse &response);
    void handlerStringResult(const std::string &result, RCF::JsonRpcResponse &response);
};