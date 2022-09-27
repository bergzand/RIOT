/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_coreconf CORECONF CoAP management interface
 * @ingroup     net
 * @brief       CORECONF definitions
 * @{
 *
 * @file
 * @brief   CORECONF definitions
 *
 * @author  Koen Zandberg <koen@bergzand.net>
 *
 * @experimental
 *
 * The coreconf module allows for interacting with a RIOT instance via the
 * CORECONF protocol. It consists of a number of CoAP endpoints with CBOR-based
 * payloads. Data models are defined in YANG.
 *
 * Requirements:
 *  - Separate different parts of the data store over different files.
 *  - Allow for extending containers by end users.
 *
 * The implementation leans heavily on the XFA subsystem, each YANG container is
 * a separate XFA, containing the nodes under the container.
 *
 * Using XFA for the coreconf allows for extending YANG containers from external
 * code, supporting application-specific extensions.
 */
#ifndef NET_CORECONF_H
#define NET_CORECONF_H

#include "macros/xtstr.h"
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include "net/gcoap.h"
#include "checksum/fletcher32.h"
#include "nanocbor/nanocbor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief URI under which the data store is provided
 */
#ifndef CONFIG_CORECONF_STORE_SUBTREE
#define CONFIG_CORECONF_STORE_SUBTREE "/c"
#endif

/**
 * @brief Maximum allowed length of the k query parameter
 */
#ifndef CONFIG_CORECONF_COAP_ARGS_LEN
#define CONFIG_CORECONF_COAP_ARGS_LEN     64
#endif

/**
 * @brief max number of args in the query supported
 */
#ifndef CONFIG_CORECONF_COAP_ARGS_NUM
#define CONFIG_CORECONF_COAP_ARGS_NUM    4
#endif

/**
 * @brief max number of concurrent requests allocated
 */
#ifndef CONFIG_CORECONF_COAP_STATE_NUM
#define CONFIG_CORECONF_COAP_STATE_NUM    4
#endif

/**
 * @brief Timeout at which a request state is cleared
 */
#ifndef CONFIG_CORECONF_COAP_STATE_TIMEOUT_SEC
#define CONFIG_CORECONF_COAP_STATE_TIMEOUT_SEC  5
#endif

/**
 * @brief Use 64 bit integers for SID instead of 32 bit integers
 *
 * Needed when YANG models use SIDs above 4G
 */
#ifndef CONFIG_CORECONF_64BIT_SID
#define CONFIG_CORECONF_64BIT_SID           0
#endif

/**
 * @brief CoAP format code used in the response.
 * application/yang-data+cbor;id=sid
 */
#ifndef CORECONF_COAP_FORMAT
#define CORECONF_COAP_FORMAT   140
#endif

/**
 * @name CORECONF config query content
 * @{
 */
#define CORECONF_QUERY_CONFIG_ONLY  'c' /**< Return only configuration data nodes */
#define CORECONF_QUERY_CONFIG_NONE  'n' /**< Return only non-config data nodes */
#define CORECONF_QUERY_CONFIG_ALL   'a' /**< Return all data nodes */
/** @} */


#if CONFIG_CORECONF_64BIT_SID
typedef uint64_t coreconf_sid_t;
#else
typedef uint32_t coreconf_sid_t;
#endif

/**
 * @brief CORECONF error tag numbers
 *
 * Used in CORECONF error replies
 */
typedef enum {
    CORECONF_ERROR              = 1005,
    CORECONF_OPERATION_FAILED   = 1019,
    CORECONF_INVALID_VALUE      = 1011,
    CORECONF_MISSING_ELEMENT    = 1014,
    CORECONF_UNKNOWN_ELEMENT    = 1023,
    CORECONF_BAD_ELEMENT        = 1001,
    CORECONF_DATA_MISSING       = 1002,
} coreconf_error_tag_t;

/**
 * @brief CORECONF app tag numbers
 *
 * Used in CORECONF error replies
 */
typedef enum {
    CORECONF_MALFORMED_MESSAGE          = 1012,
    CORECONF_DATA_NOT_UNIQUE            = 1003,
    CORECONF_TOO_MANY_ELEMENTS          = 1022,
    CORECONF_TOO_FEW_ELEMENTS           = 1021,
    CORECONF_MUST_VIOLATION             = 1017,
    CORECONF_DUPLICATE                  = 1004,
    CORECONF_INVALID_DATA_TYPE          = 1009,
    CORECONF_NOT_IN_RANGE               = 1018,
    CORECONF_INVALID_LENGTH             = 1010,
    CORECONF_PATTERN_TEST_FAILED        = 1020,
    CORECONF_MISSING_KEY                = 1016,
    CORECONF_MISSING_INPUT_PARAMETER    = 1015,
    CORECONF_INSTANCE_REQUIRED          = 1008,
    CORECONF_MISSING_CHOICE             = 1013,
} coreconf_app_tag_t;

/**
 * @brief CORECONF handler function error codes
 */
enum {
    CORECONF_ERR_NOT_FOUND          = -1,   /**< Resource requested not found */
    CORECONF_ERR_INTERNAL_SERVER    = -2,   /**< Internal server error */
};

/**
 * @brief CORECONF XFA node forward declaration
 */
typedef struct coreconf_node coreconf_node_t;

/**
 * @brief CORECONF coap block2 slicer and etag helper
 */
typedef struct {
    coap_block_slicer_t slicer; /**< coap block slicer struct */
    size_t sliced_length;       /**< Total length sliced */
    uint8_t *buf;               /**< Buffer to slice into */
    size_t resp_len;            /**< Total length of the response */
    uint16_t fletcher_tmp;      /**< Temporary storage for fletcher32 words */
    fletcher32_ctx_t fltchr;    /**< Fletcher32 context for etag */
} coreconf_slicer_helper_t;

/**
 * @brief CORECONF request context
 *
 * Used to store request data between block2 requests
 */
typedef struct {
    event_t ev;                 /**< event to flush the request */
    event_timeout_t timeout;    /**< Timeout to flush the request */
    uint16_t message_id;        /**< message ID to correlate with */
    uint8_t method;             /**< CoAP method */
    /**
     * @brief request data for get (uri query string) or fetch (payload)
     */
    union {
        char uri_query[CONFIG_CORECONF_COAP_ARGS_LEN];          /**< Uri query */
        uint8_t request_data[CONFIG_CORECONF_COAP_ARGS_LEN];    /**< fetch payload */
    };
} coreconf_state_t;

/**
 * @brief CORECONF request context
 *
 * Used with all types of requests
 */
typedef struct {
    coap_pkt_t *pdu;            /**< Pointer to the request/response PDU */
    size_t pdu_len;             /**< Length of the pdu */
    uint32_t etag;              /**< Etag of the pdu */
    uint32_t blocknum2;         /**< block2 number requested*/
    uint8_t szx2;               /**< Size exponent of the block number */
    bool etag_sent;             /**< Signals whether there is an etag */
    char config;                /**< Config argument */
    char with_default;          /**< defaults argument */

    coreconf_state_t *state;    /**< Ptr to the @ref coreconf_state_t */
    nanocbor_value_t decoder;   /**< CBOR decoder state */
} coreconf_ctx_t;

/**
 * @brief Response encoder specific struct
 */
typedef struct {
    coreconf_ctx_t ctx;                 /**< Generic context */
    coreconf_slicer_helper_t slicer;    /**< Response slicer */
    nanocbor_encoder_t encoder;         /**< Encoder context */
} coreconf_encoder_t;

/**
 * @brief POST/PUT/DELETE/PATCH Request context
 */
typedef struct {
    coreconf_ctx_t ctx; /**< Generic context */
} coreconf_decoder_t;

/**
 * @brief CORECONF node read function.
 *
 * Read functions must be idempotent and can be called multiple times to
 * generate the output.
 *
 * @param   encoder encoder context structure
 * @param   node    node to read
 * @param   argv    Already parsed array of arguments
 */
typedef ssize_t (*coreconf_node_read_handler_t)
    (coreconf_encoder_t *encoder,
    const coreconf_node_t *node,
    void **argv);

/**
 * @brief CORECONF node write function.
 *
 * Are called for all types of writes: PUT, POST, DELETE and IPATCH
 *
 * @param   decoder decoder context structure
 * @param   node    node to write
 * @param   argv    Already parsed array of arguments
 */
typedef ssize_t (*coreconf_node_write_handler_t)
    (coreconf_decoder_t *decoder,
    const coreconf_node_t *node,
    void **argv);

/**
 * @brief Argument parser for YANG container list types.
 *
 * @param   ctx     context structure
 * @param   node    node to write
 * @param   argv    Already parsed array of arguments
 */
typedef ssize_t (*coreconf_node_arg_handler_t)
    (coreconf_ctx_t *ctx,
    const coreconf_node_t *node,
    void **argv);

/**
 * @brief CORECONF XFA node types
 */
typedef enum {
    CORECONF_NODE_LEAF          = 1,    /**< Leaf type */
    CORECONF_NODE_LEAF_LIST     = 2,    /**< Leaf list type */
    CORECONF_NODE_CONTAINER     = 3,    /**< Container node */
    CORECONF_NODE_LIST          = 4,    /**< List type container */
    CORECONF_NODE_DATA_STORE    = 5,    /**< Also a container */
} coreconf_node_type_t;

/**
 * @brief CORECONF node flags
 */
#define CORECONF_NODE_CONFIG        0x80 /**< Node is a config type */
#define CORECONF_NODE_STATE         0    /**< Node is a state type */

/**
 * @brief optional extra data required with some containers and types
 */
typedef struct {
    coreconf_node_arg_handler_t parse;      /**< argument parser for container lists */
    coreconf_node_read_handler_t read;      /**< get or fetch for containers lists */
    coreconf_node_write_handler_t write;    /**< put, post or ipatch handler */
} coreconf_node_container_aux_t;

/**
 * @brief Functions and data for YANG leaf nodes
 */
typedef struct {
    coreconf_sid_t num;                     /**< SID value of the node */
    coreconf_node_read_handler_t read;      /**< get or fetch handler */
    coreconf_node_write_handler_t write;    /**< put, post or ipatch handler */
} coreconf_node_leaf_t;

/**
 * @brief Functions and data for YANG containers and container list nodes
 */
typedef struct {
    coreconf_sid_t num;                                 /**< SID value of the container node */
    const volatile coreconf_node_t *target;             /**< First element of the container XFA */
    const volatile coreconf_node_t *end;                /**< End marker of the container XFA */
    const volatile coreconf_node_container_aux_t *aux;  /**< Ptr to extra data */
} coreconf_node_container_t;

/**
 * @brief CORECONF XFA node
 */
struct coreconf_node {
    uint16_t type;                              /**< Node type and flags */
    union {
        coreconf_node_leaf_t leaf;              /**< Leaf type members */
        coreconf_node_container_t container;    /**< Container type members */
    };
};

/**
 * @brief Get the method used with the current CoAP request
 *
 * @param   ctx CORECONF request context
 *
 * @returns     CoAP request method
 */
static inline uint8_t coreconf_ctx_get_method(const coreconf_ctx_t *ctx)
{
    return ctx->state->method;
}

/**
 * @brief Get the nanocbor encoder
 *
 * @param   enc Encoder context
 *
 * @return      Nanocbor encoder
 */
static inline nanocbor_encoder_t *coreconf_encoder_cbor(coreconf_encoder_t *enc)
{
    return &enc->encoder;
}

/**
 * @brief Check if any argument is supplied with the current request
 *
 * TODO: implement all request types, only GET and FETCH implemented now
 *
 * @param   ctx Context struct
 *
 * @return  True if no arguments are supplied
 */
static inline bool coreconf_args_empty(const coreconf_ctx_t *ctx)
{
    assert(ctx->state);
    return (coreconf_ctx_get_method(ctx) == COAP_METHOD_GET && ctx->state->uri_query[0] == '\0') ||
           (coreconf_ctx_get_method(ctx) == COAP_METHOD_FETCH && nanocbor_at_end(&ctx->decoder));
}

/**
 * @brief Check if any argument is supplied with the current request
 *
 * TODO: implement all request types, only GET and FETCH implemented now
 *
 * @param   enc Encoder context struct
 *
 * @return  True if no arguments are supplied
 */
static inline bool coreconf_enc_args_empty(const coreconf_encoder_t *enc)
{
    return coreconf_args_empty(&enc->ctx);
}

/**
 * @brief Check if any argument is supplied with the current request
 *
 * TODO: implement all request types, only GET and FETCH implemented now
 *
 * @param   dec Decoder context struct
 *
 * @return  True if no arguments are supplied
 */
static inline bool coreconf_dec_args_empty(const coreconf_decoder_t *dec)
{
    return coreconf_args_empty(&dec->ctx);
}

/**
 * @brief Get the difference between two SIDs, @p second - @p first
 *
 * @pre second >= first
 *
 * @param first First SID, usually the container
 * @param second Second SID, usually the leaf in the container
 *
 * @returns The difference between the SIDs
 */
static inline coreconf_sid_t coreconf_sid_diff(coreconf_sid_t first, coreconf_sid_t second)
{
    return second - first;
}

/**
 * @brief Format the SID as CBOR unsigned integer
 *
 * @param   enc     Encoder struct
 * @param   base    Base SID, usually the base container
 * @param   sid     SID of the YANG node to format
 */
static inline void coreconf_cbor_sid(coreconf_encoder_t *enc, coreconf_sid_t base,
                                     coreconf_sid_t sid)
{
    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), coreconf_sid_diff(base, sid));
}

/**
 * @brief Get the node type
 *
 * @param   node    Node to get the type for
 *
 * @return          The type of the coreconf node
 */
static inline coreconf_node_type_t coreconf_node_type(const coreconf_node_t *node)
{
    return node->type & ~CORECONF_NODE_CONFIG;
}

/**
 * @brief Get the config status of the node
 *
 * @param   node    Node to get the config status for
 *
 * @return          The config status
 */
static inline bool coreconf_node_config(const coreconf_node_t *node)
{
    return node->type & CORECONF_NODE_CONFIG;
}

/**
 * @brief Get the SID of the node
 *
 * @param   node    Node to get the SID for
 *
 * @return          The SID, 0 if not applicable
 */
static inline coreconf_sid_t coreconf_node_sid(const coreconf_node_t *node)
{
    switch (coreconf_node_type(node)) {
    case CORECONF_NODE_LEAF:
        return node->leaf.num;
    case CORECONF_NODE_CONTAINER:
        return node->container.num;
    default:
        return 0;
    }
}

/**
 * @brief Read and encode all members of a container node
 *
 * Skips members based on the configuration parameter with the request.
 *
 * @param   enc     Encoder context struct
 * @param   node    Container type node
 * @param   argv    List of decoded arguments
 */
ssize_t coreconf_read_container(coreconf_encoder_t *enc, const coreconf_node_t *node, void **argv);

/**
 * @brief Checks if this is the first block part requested
 *
 * Can be used to cache data structures to ensure consistency between parts of
 * the requests
 *
 * @param   ctx     Context struct
 *
 * @returns True if it is the first part
 */
static inline bool coreconf_first_request_part(coreconf_ctx_t *ctx)
{
    return ctx->blocknum2 == 0;
}

int coreconf_arg_as_str(const coreconf_ctx_t *ctx, int offset, const char **val);

/**
 * @brief Return a CORECONF error to the client as response
 *
 * @param   ctx     CORECONF context
 * @param   node    Node to generate the error for, may be NULL for none
 * @param   error_tag   Error tag to include
 * @param   app_tag     Application tag to include
 * @param   error_msg   Error message string to add, may be NULL
 *
 * @returns Size of the reply
 */
ssize_t coreconf_reply_error(const coreconf_ctx_t *ctx,
                             const coreconf_node_t *node, coreconf_error_tag_t error_tag,
                             coreconf_app_tag_t app_tag, const char *error_msg);

#ifndef DOXYGEN
/* Convenience and macro name expansion macros */
#define _CORECONF_CONCAT_HELPER(a, b)    a ## b
#define _CORECONF_CONCAT(a, b)  _CORECONF_CONCAT_HELPER(a, b)
#define _CORECONF_XFA_AUX_NAME(aux)   _CORECONF_CONCAT(coreconf_aux_xfa_, aux)
#define _CORECONF_XFA_NAME(container)   _CORECONF_CONCAT(coreconf_node_xfa_, container)
#define _CORECONF_XFA_NAME__(container)   _CORECONF_CONCAT(_CORECONF_XFA_NAME(container), _)
#define _CORECONF_XFA_CONST_WRAPPER(name, prio)   XFA_CONST(name, prio)
#define _CORECONF_XFA_INIT_CONST_WRAPPER(name, prio)   XFA_INIT_CONST(name, prio)

#define CORECONF_CONTAINER_AUX(name, _parse, _read, _write) \
    _CORECONF_XFA_CONST_WRAPPER(_CORECONF_XFA_AUX_NAME(aux_), name)  \
    coreconf_node_container_aux_t _CORECONF_CONCAT(_CORECONF_XFA_AUX_NAME(aux_), name) \
        = { \
        .parse = _parse, \
        .read = _read, \
        .write = _write }

#define CORECONF_CONTAINER_SUBTYPE(_container, name, _num, _config, _type, _aux) \
    _CORECONF_XFA_INIT_CONST_WRAPPER(coreconf_node_t, _CORECONF_XFA_NAME(name)); \
    _CORECONF_XFA_CONST_WRAPPER(_CORECONF_XFA_NAME(_container), _num)  \
    coreconf_node_t _CORECONF_CONCAT(_CORECONF_XFA_NAME__(_container), _num) \
        = { \
        .type = _type | _config, \
        .container = { \
            .num = _num, \
            .target = _CORECONF_XFA_NAME(name), \
            .end = _CORECONF_CONCAT(_CORECONF_XFA_NAME(name), _end), \
            .aux = _aux, \
            } \
        }

#define CORECONF_CONTAINER_SUBTYPE_AUX(_container, name, _num, _config, _type, \
                                       _parse, _read, _write) \
    CORECONF_CONTAINER_AUX(name, _parse, _read, _write); \
    CORECONF_CONTAINER_SUBTYPE(_container, name, _num, _config, _type, \
                               &_CORECONF_CONCAT(_CORECONF_XFA_AUX_NAME(aux_), name))
#endif

/**
 * @brief   Define a CORECONF leaf endpoint
 *
 * This macro is a helper for defining a CORECONF endpoint and adding it to the
 * CORECONF XFA (cross file array).
 *
 * @param _container    Container name it is part of
 * @param _num          SID value
 * @param _config       @ref CORECONF_NODE_CONFIG or @ref CORECONF_NODE_STATE
 * @param _read         Node read function, see @ref coreconf_node_read_handler_t
 * @param _write        Node write function, see @ref coreconf_node_write_handler_t
 */
#define CORECONF_LEAF(_container, _num, _config, _read, _write)   \
    _CORECONF_XFA_CONST_WRAPPER(_CORECONF_XFA_NAME(_container), _num)  \
    coreconf_node_t _CORECONF_CONCAT(_CORECONF_XFA_NAME__(_container), _num) \
        = { .type = CORECONF_NODE_LEAF | _config, \
            .leaf = { \
                .num = _num, \
                .read = _read, \
                .write = _write } \
        }

#define CORECONF_RPC(_num, _write)   \
    _CORECONF_XFA_CONST_WRAPPER(_CORECONF_XFA_NAME(_container), _num)  \
    coreconf_node_t _CORECONF_CONCAT(coreconf_rpc_xfa, _num) \
        = { .type = CORECONF_NODE_LEAF, \
            .leaf = { \
                .num = _num, \
                .write = _write } \
        }

/**
 * @brief   Define a simple container without parse, read and write functions
 *
 * @param container   Parent container name
 * @param name        Container name
 * @param _num        SID value
 * @param _config       @ref CORECONF_NODE_CONFIG or @ref CORECONF_NODE_STATE
 */
#define CORECONF_CONTAINER(container, name, _num, _config)     \
    CORECONF_CONTAINER_SUBTYPE(container, name, _num, _config, \
                               CORECONF_NODE_CONTAINER, NULL)

/**
 * @brief   Define a simple container with parse, read and write functions
 *
 * @param container   Parent container name
 * @param name        Container name
 * @param _num        SID value
 * @param _config     @ref CORECONF_NODE_CONFIG or @ref CORECONF_NODE_STATE
 * @param _parse      Node parse function, see @ref coreconf_node_arg_handler_t
 * @param _read       Node read function, see @ref coreconf_node_read_handler_t
 * @param _write      Node write function, see @ref coreconf_node_write_handler_t
 */
#define CORECONF_CONTAINER_ADV(container, name, _num, _config, _parse, \
                               _read, _write) \
    CORECONF_CONTAINER_SUBTYPE_AUX(container, name, _num, \
                                   _config, CORECONF_NODE_CONTAINER, _parse, _read, _write)

/**
 * @brief   Define a container list with parse, read and write functions
 *
 * @param container   Parent container name
 * @param name        Container name
 * @param _num        SID value
 * @param _config     @ref CORECONF_NODE_CONFIG or @ref CORECONF_NODE_STATE
 * @param _parse      Node parse function, see @ref coreconf_node_arg_handler_t
 * @param _read       Node read function, see @ref coreconf_node_read_handler_t
 * @param _write      Node write function, see @ref coreconf_node_write_handler_t
 */
#define CORECONF_CONTAINER_LIST(container, name, _num, _config, _parse, _read, _write)  \
    CORECONF_CONTAINER_SUBTYPE_AUX(container, name, _num, _config, \
                                   CORECONF_NODE_CONTAINER, _parse, _read, _write)
#ifdef __cplusplus
}
#endif

#endif /* NET_CORECONF_H */
/** @} */
