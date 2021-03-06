/*
 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * Copyright (c) 2012, YumaWorks, Inc., All Rights Reserved.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */
/*  FILE: agt_state.c

   NETCONF State Data Model implementation: Agent Side Support

From draft-13:

identifiers:
container /netconf-state
container /netconf-state/capabilities
leaf-list /netconf-state/capabilities/capability
container /netconf-state/datastores
list /netconf-state/datastores/datastore
leaf /netconf-state/datastores/datastore/name
container /netconf-state/datastores/datastore/locks
choice /netconf-state/datastores/datastore/locks/lock-type
case /netconf-state/datastores/datastore/locks/lock-type/global-lock
container /netconf-state/datastores/datastore/locks/lock-type/global-lock/
    global-lock
leaf /netconf-state/datastores/datastore/locks/lock-type/global-lock/
    global-lock/locked-by-session
leaf /netconf-state/datastores/datastore/locks/lock-type/global-lock/
    global-lock/locked-time
case /netconf-state/datastores/datastore/locks/lock-type/partial-locks
list /netconf-state/datastores/datastore/locks/lock-type/partial-locks/
    partial-locks
leaf /netconf-state/datastores/datastore/locks/lock-type/partial-locks/
    partial-locks/lock-id
leaf /netconf-state/datastores/datastore/locks/lock-type/partial-locks/
    partial-locks/locked-by-session
leaf /netconf-state/datastores/datastore/locks/lock-type/partial-locks/
    partial-locks/locked-time
leaf-list /netconf-state/datastores/datastore/locks/lock-type/partial-locks/
    partial-locks/select
leaf-list /netconf-state/datastores/datastore/locks/lock-type/partial-locks/
    partial-locks/locked-nodes
container /netconf-state/schemas
list /netconf-state/schemas/schema
leaf /netconf-state/schemas/schema/identifier
leaf /netconf-state/schemas/schema/version
leaf /netconf-state/schemas/schema/format
leaf /netconf-state/schemas/schema/namespace
leaf-list /netconf-state/schemas/schema/location
container /netconf-state/sessions
list /netconf-state/sessions/session
leaf /netconf-state/sessions/session/session-id
leaf /netconf-state/sessions/session/transport
leaf /netconf-state/sessions/session/username
leaf /netconf-state/sessions/session/source-host
leaf /netconf-state/sessions/session/login-time
leaf /netconf-state/sessions/session/in-rpcs
leaf /netconf-state/sessions/session/in-bad-rpcs
leaf /netconf-state/sessions/session/out-rpc-errors
leaf /netconf-state/sessions/session/out-notifications
container /netconf-state/statistics
leaf /netconf-state/statistics/netconf-start-time
leaf /netconf-state/statistics/in-bad-hellos
leaf /netconf-state/statistics/in-sessions
leaf /netconf-state/statistics/dropped-sessions
leaf /netconf-state/statistics/in-rpcs
leaf /netconf-state/statistics/in-bad-rpcs
leaf /netconf-state/statistics/out-rpc-errors
leaf /netconf-state/statistics/out-notifications
rpc /get-schema
container /get-schema/input
leaf /get-schema/input/identifier
leaf /get-schema/input/version
leaf /get-schema/input/format
container /get-schema/output
anyxml /get-schema/output/data

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
24feb09      abb      begun
02dec09      abb      redo with new names

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_cap.h"
#include "agt_cb.h"
#include "agt_rpc.h"
#include "agt_ses.h"
#include "agt_state.h"
#include "agt_sys.h"
#include "agt_time_filter.h"
#include "agt_util.h"
#include "cfg.h"
#include "getcb.h"
#include "log.h"
#include "ncxmod.h"
#include "ncxtypes.h"
#include "rpc.h"
#include "ses.h"
#include "ses_msg.h"
#include "status.h"
#include "tstamp.h"
#include "val.h"
#include "val_util.h"
#include "xmlns.h"
#include "xml_util.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define AGT_STATE_TOP_CONTAINER (const xmlChar *)"netconf-state"

#define AGT_STATE_GET_SCHEMA    (const xmlChar *)"get-schema"

#define AGT_STATE_OBJ_CAPABILITIES  (const xmlChar *)"capabilities"

#define AGT_STATE_OBJ_DATASTORE   (const xmlChar *)"datastore"
#define AGT_STATE_OBJ_DATASTORES  (const xmlChar *)"datastores"
#define AGT_STATE_OBJ_LOCKS           (const xmlChar *)"locks"

#define AGT_STATE_OBJ_SCHEMA          (const xmlChar *)"schema"
#define AGT_STATE_OBJ_SCHEMAS         (const xmlChar *)"schemas"

#define AGT_STATE_OBJ_IDENTIFIER      (const xmlChar *)"identifier"
#define AGT_STATE_OBJ_FORMAT          (const xmlChar *)"format"
#define AGT_STATE_OBJ_VERSION         (const xmlChar *)"version"
#define AGT_STATE_OBJ_NAMESPACE       (const xmlChar *)"namespace"
#define AGT_STATE_OBJ_LOCATION        (const xmlChar *)"location"

#define AGT_STATE_OBJ_SESSION         (const xmlChar *)"session"
#define AGT_STATE_OBJ_SESSIONS        (const xmlChar *)"sessions"

#define AGT_STATE_OBJ_SESSIONID       (const xmlChar *)"session-id"
#define AGT_STATE_OBJ_TRANSPORT       (const xmlChar *)"transport"
#define AGT_STATE_OBJ_USERNAME        (const xmlChar *)"username"
#define AGT_STATE_OBJ_SOURCEHOST      (const xmlChar *)"source-host"
#define AGT_STATE_OBJ_LOGINTIME       (const xmlChar *)"login-time"

#define AGT_STATE_OBJ_IN_RPCS         (const xmlChar *)"in-rpcs"
#define AGT_STATE_OBJ_IN_SESSIONS     (const xmlChar *)"in-sessions"
#define AGT_STATE_OBJ_NETCONF_START_TIME \
    (const xmlChar *)"netconf-start-time"
#define AGT_STATE_OBJ_IN_BAD_RPCS     (const xmlChar *)"in-bad-rpcs"
#define AGT_STATE_OBJ_IN_BAD_HELLOS   (const xmlChar *)"in-bad-hellos"
#define AGT_STATE_OBJ_OUT_RPC_ERRORS  (const xmlChar *)"out-rpc-errors"
#define AGT_STATE_OBJ_OUT_NOTIFICATIONS  (const xmlChar *)"out-notifications"
#define AGT_STATE_OBJ_DROPPED_SESSIONS  (const xmlChar *)"dropped-sessions"
#define AGT_STATE_OBJ_STATISTICS      (const xmlChar *)"statistics"

#define AGT_STATE_OBJ_GLOBAL_LOCK     (const xmlChar *)"global-lock"
#define AGT_STATE_OBJ_NAME            (const xmlChar *)"name"
#define AGT_STATE_OBJ_LOCKED_BY_SESSION (const xmlChar *)"locked-by-session"
#define AGT_STATE_OBJ_LOCKED_TIME     (const xmlChar *)"locked-time"

#define AGT_STATE_OBJ_PARTIAL_LOCK    (const xmlChar *)"partial-lock"
#define AGT_STATE_OBJ_LOCK_ID         (const xmlChar *)"lock-id"
#define AGT_STATE_OBJ_SELECT          (const xmlChar *)"select"
#define AGT_STATE_OBJ_LOCKED_NODES    (const xmlChar *)"locked-nodes"

#define AGT_STATE_FORMAT_YANG         (const xmlChar *)"yang"
#define AGT_STATE_FORMAT_YIN          (const xmlChar *)"yin"

#define AGT_STATE_ENUM_NETCONF        (const xmlChar *)"NETCONF"


/* yumaworks-system extensions */
#define AGT_STATE_OBJ_BACKUP_FILE     (const xmlChar *)"backup-file"
#define AGT_STATE_OBJ_BACKUP_TIME     (const xmlChar *)"backup-time"
#define AGT_STATE_OBJ_BACKUP_FILES    (const xmlChar *)"backup-files"
#define AGT_STATE_OBJ_CONFORMANCE     (const xmlChar *)"conformance"



/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

static boolean agt_state_init_done = FALSE;

static ncx_module_t *statemod;

static val_value_t *mysessionsval;

static val_value_t *myschemasval;

static val_value_t *mybackupfilesval;

static obj_template_t *mysessionobj;

static obj_template_t *myschemaobj;

static xmlChar *ietf_netconf_path;

/**
 * \fn get_caps
 * \brief <get> operation for the capabilities NP container
 * \param scb session that issued the get (may be NULL)
 * \param cbmode reason for the callback
 * \param virval place-holder node in data model for this virtual
 * value node
 * \param dstval pointer to value output struct
 * \return status
 */
static status_t 
    get_caps (ses_cb_t *scb,
              getcb_mode_t cbmode,
              val_value_t *virval,
              val_value_t  *dstval)
{
    val_value_t       *capsval;
    status_t           res;

    (void)scb;
    (void)virval;
    res = NO_ERR;

    if (cbmode == GETCB_GET_VALUE) {
        capsval = val_clone(agt_cap_get_capsval());
        if (!capsval) {
            return ERR_INTERNAL_MEM;
        }
        /* change the namespace to this module, 
         * and get rid of the netconf NSID 
         */
        val_change_nsid(capsval, statemod->nsid);
        val_move_children(capsval, dstval);
        val_free_value(capsval);
    } else {
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    }
    return res;

}  /* get_caps */

// ----------------------------------------------------------------------------!

/**
 * \fn make_plock_entry
 * \brief Make a partial-lock list entry
 * \param plcb partial lock control block to use
 * \param plockobj object template to use for the list struct
 * \param res address of return status
 * \return pointer to malloc value struct for this data; NULL if
 * malloc error
 */
static val_value_t *
    make_plock_entry (plock_cb_t *plcb,
                      obj_template_t *plockobj,
                      status_t *res)
{
    val_value_t  *plockval, *leafval, *valptr;
    obj_template_t *selectobj, *lockednodeobj;
    xpath_pcb_t *xpathpcb;
    xpath_resnode_t *resnode;
    xpath_result_t *result;
    xmlChar *pathbuff;


    *res = NO_ERR;

    selectobj = obj_find_child(plockobj,
                               AGT_STATE_MODULE,
                               AGT_STATE_OBJ_SELECT);
    if (selectobj == NULL) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    lockednodeobj = obj_find_child(plockobj,
                                   AGT_STATE_MODULE,
                                   AGT_STATE_OBJ_LOCKED_NODES);
    if (lockednodeobj == NULL) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
                               
    plockval = val_new_value();
    if (plockval == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    val_init_from_template(plockval, plockobj);

    /* add the list key: lock-id */
    leafval = agt_make_uint_leaf(plockobj,
                                 AGT_STATE_OBJ_LOCK_ID,
                                 plock_get_id(plcb),
                                 res);
    if (leafval == NULL) {
        val_free_value(plockval);
        return NULL;
    }
    val_add_child(leafval, plockval);

    /* add the session ID of the lock-owner */
    leafval = agt_make_uint_leaf(plockobj,
                                 AGT_STATE_OBJ_LOCKED_BY_SESSION,
                                 plock_get_sid(plcb),
                                 res);
    if (leafval == NULL) {
        val_free_value(plockval);
        return NULL;
    }
    val_add_child(leafval, plockval);

    /* add the lock start timestamp */
    leafval = agt_make_leaf(plockobj,
                            AGT_STATE_OBJ_LOCKED_TIME,
                            plock_get_timestamp(plcb),
                            res);
    if (leafval == NULL) {
        val_free_value(plockval);
        return NULL;
    }
    val_add_child(leafval, plockval);

    /* add a 'select' leaf for each parm in the request */
    for (xpathpcb = plock_get_first_select(plcb);
         xpathpcb != NULL;
         xpathpcb = plock_get_next_select(xpathpcb)) {

        /* cobble an XPath string together making sure to
         * preserve the XML prefix mappings originally parsed
         */
        leafval = val_new_value();
        if (leafval == NULL) {
            val_free_value(plockval);
            *res = ERR_INTERNAL_MEM;
            return NULL;
        }
        val_init_from_template(leafval, selectobj);
        VAL_STR(leafval) = xml_strdup(xpathpcb->exprstr);
        if (VAL_STR(leafval) == NULL) {
            val_free_value(plockval);
            *res = ERR_INTERNAL_MEM;
            return NULL;
        }
        leafval->xpathpcb = xpath_clone_pcb(xpathpcb);
        if (leafval->xpathpcb == NULL) {
            val_free_value(plockval);
            *res = ERR_INTERNAL_MEM;
            return NULL;
        }
        val_add_child(leafval, plockval);
    }

    /* add a 'locked-nodes' leaf for each valptr in the 
     * final node-set result
     */
    pathbuff = NULL;
    result = plock_get_final_result(plcb);
    for (resnode = xpath_get_first_resnode(result);
         resnode != NULL;
         resnode = xpath_get_next_resnode(resnode)) {

        /* cobble an i-i string together making sure to
         * preserve the XML prefix mappings originally parsed
         */
        leafval = val_new_value();
        if (leafval == NULL) {
            val_free_value(plockval);
            *res = ERR_INTERNAL_MEM;
            return NULL;
        }
        val_init_from_template(leafval, lockednodeobj);

        valptr = xpath_get_resnode_valptr(resnode);
        *res = val_gen_instance_id(NULL, 
                                   valptr,
                                   NCX_IFMT_XPATH1,
                                   &pathbuff);
        if (*res != NO_ERR) {
            if (pathbuff != NULL) {
                m__free(pathbuff);
            }
            val_free_value(leafval);
            val_free_value(plockval);
            return NULL;
        }

        *res = val_set_simval_obj(leafval,
                                  lockednodeobj,
                                  pathbuff);
        if (*res != NO_ERR) {
            m__free(pathbuff);
            val_free_value(leafval);
            val_free_value(plockval);
            return NULL;
        }
        val_add_child(leafval, plockval);
        m__free(pathbuff);
        pathbuff = NULL;
    }

    *res = val_gen_index_chain(plockobj, plockval);
    if (*res != NO_ERR) {
        val_free_value(plockval);
        return NULL;
    }


    *res = NO_ERR;
    return plockval;

}  /* make_plock_entry */

// ----------------------------------------------------------------------------!

/**
 * \fn get_locks
 * \brief <get> operation handler for the locks NP container
 * \param scb session that issued the get (may be NULL)
 * \param cbmode reason for the callback
 * \param virval place-holder node in data model for this virtual
 * value node
 * \param dstval pointer to value output struct
 * \return status
 */   
static status_t 
    get_locks (ses_cb_t *scb,
               getcb_mode_t cbmode,
               val_value_t *virval,
               val_value_t  *dstval)
{
    (void)scb;
    status_t res = NO_ERR;

    if (cbmode == GETCB_GET_VALUE) {
        obj_template_t *globallock =
            obj_find_child(virval->obj, AGT_STATE_MODULE,
                           AGT_STATE_OBJ_GLOBAL_LOCK);
        if (!globallock) {
            return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        }

        obj_template_t *plockobj =
            obj_find_child(virval->obj, AGT_STATE_MODULE,
                           AGT_STATE_OBJ_PARTIAL_LOCK);
        if (!plockobj) {
            return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        }

        val_value_t *nameval =
            val_find_child(virval->parent, AGT_STATE_MODULE,
                           AGT_STATE_OBJ_NAME);
        if (!nameval) {
            return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        }

        cfg_template_t *cfg = cfg_get_config(VAL_ENUM_NAME(nameval));
        if (!cfg) {
            return SET_ERROR(ERR_NCX_CFG_NOT_FOUND);            
        }

        if (cfg_is_global_locked(cfg)) {
            /* make the 2 return value leafs to put into
             * the retval container
             */
            ses_id_t sid = 0;
            const xmlChar *locktime = NULL;
            res = cfg_get_global_lock_info(cfg, &sid, &locktime);
            if (res == NO_ERR) {
                /* add locks/global-lock */
                val_value_t *globallockval = val_new_value();
                if (!globallockval) {
                    return ERR_INTERNAL_MEM;
                }
                val_init_from_template(globallockval, globallock);
                val_add_child(globallockval, dstval);

                /* add locks/global-lock/locked-by-session */ 
                val_value_t *newval = 
                    agt_make_uint_leaf(globallock,
                                       AGT_STATE_OBJ_LOCKED_BY_SESSION,
                                       sid, &res);
                if (newval) {
                    val_add_child(newval, globallockval);
                }

                /* add locks/global-lock/locked-time */ 
                newval = agt_make_leaf(globallock, AGT_STATE_OBJ_LOCKED_TIME,
                                       locktime, &res);
                if (newval) {
                    val_add_child(newval, globallockval);
                }
            }
        } else if (cfg_is_partial_locked(cfg)) {
            plock_cb_t *plcb = cfg_first_partial_lock(cfg);
            for (; plcb != NULL && res == NO_ERR;
                 plcb = cfg_next_partial_lock(plcb)) {
                val_value_t *plockval = make_plock_entry(plcb, plockobj, &res);
                if (plockval) {
                    val_add_child(plockval, dstval);
                }
            }
        } else {
            res = ERR_NCX_SKIPPED;
        }
    } else {
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    }
    return res;

} /* get_locks */

// ----------------------------------------------------------------------------!

/**
 * \fn get_datastore_name
 * \brief Get the datastore name for the virtual timestamp entry that
 * was called from a <get> operation
 * \param virval virtual value
 * \param retname address of return name string pointer
 * \return status
 */
static status_t 
    get_datastore_name (const val_value_t *virval,
                        const xmlChar **retname)
{
    const val_value_t  *parentval, *nameval;

    parentval = virval->parent;
    if (parentval == NULL) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    nameval = val_find_child(parentval,
                             obj_get_mod_name(parentval->obj),
                             (const xmlChar *)"name");

    if (nameval == NULL) {
        return ERR_NCX_DEF_NOT_FOUND;        
    }

    *retname = VAL_STR(nameval);
    return NO_ERR;

} /* get_datastore_name */

// ----------------------------------------------------------------------------!

/**
 * \fn get_last_modified
 * \brief <get> operation handler for the datastore/last-modified timestamp
 * \param scb session that issued the get (may be NULL)
 * \param cbmode reason for the callback
 * \param virval place-holder node in data model for this virtual
 * value node
 * \param dstval pointer to value output struct
 * \return status
 */
static status_t 
    get_last_modified (ses_cb_t *scb,
                       getcb_mode_t cbmode,
                       const val_value_t *virval,
                       val_value_t  *dstval)
{
    const xmlChar  *name = NULL;
    cfg_template_t *cfg;
    status_t        res;

    (void)scb;

    if (cbmode != GETCB_GET_VALUE) {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    res = get_datastore_name(virval, &name);
    if (res != NO_ERR) {
        return res;
    }

    cfg = cfg_get_config(name);
    if (cfg == NULL) {
        return ERR_DB_NOT_FOUND;
    }

    VAL_STR(dstval) = xml_strdup(cfg->last_ch_time);
    if (VAL_STR(dstval) == NULL) {
        return ERR_INTERNAL_MEM;
    }
    return NO_ERR;

} /* get_last_modified */

// ----------------------------------------------------------------------------!

/**
 * \fn make_datastore_val
 * \brief make a val_value_t struct for a specified configuration
 * \param confname config name
 * \param confobj config object to use
 * \param res address of return status
 * \return malloced value struct or NULL if some error
 */
static val_value_t *
    make_datastore_val (const xmlChar *confname,
                        obj_template_t *confobj,
                        status_t *res)
{
    *res = NO_ERR;

    /* create datastore node */
    val_value_t *confval = val_new_value();
    if (!confval) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    val_init_from_template(confval, confobj);

    val_value_t *nameval = agt_make_leaf(confobj, NCX_EL_NAME, confname, res);
    if (*res != NO_ERR) {
        if (nameval != NULL) {
            val_free_value(nameval);
        }
        val_free_value(confval);
        return NULL;
    }
    val_add_child(nameval, confval);
    
    /* create datastore/locks */
    obj_template_t *testobj =
        obj_find_child(confobj, AGT_STATE_MODULE, AGT_STATE_OBJ_LOCKS);
    if (!testobj) {
        val_free_value(confval);
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    val_value_t *leafval = val_new_value();
    if (!leafval) {
        val_free_value(confval);
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    val_init_virtual(leafval, get_locks, testobj);
    val_add_child(leafval, confval);

    /* create yuma extension datastore/last-modified; only if object found */
    testobj = obj_find_child(confobj, 
                             y_yuma_time_filter_M_yuma_time_filter, 
                             NCX_EL_LAST_MODIFIED);
    if (testobj) {
        leafval = val_new_value();
        if (!leafval) {
            val_free_value(confval);
            *res = ERR_INTERNAL_MEM;
            return NULL;
        }
        val_init_virtual(leafval, get_last_modified, testobj);
        val_add_child(leafval, confval);
    }

    *res = NO_ERR;
    return confval;

} /* make_datastore_val */

// ----------------------------------------------------------------------------!


/**
 * \fn make_schema_val
 * \brief make a val_value_t struct for a specified module
 * \param mod module control block to use
 * \param schemaobj <schema> object to use
 * \param res address of return status
 * \return malloced value struct or NULL if some error
 */
static val_value_t *
    make_schema_val (ncx_module_t *mod,
                     obj_template_t *schemaobj,
                     status_t *res)
{
    *res = NO_ERR;

    /* create schema node */
    val_value_t *schemaval = val_new_value();
    if (!schemaval) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    val_init_from_template(schemaval, schemaobj);

    /* create schema/identifier */
    val_value_t *childval = 
        agt_make_leaf(schemaobj, AGT_STATE_OBJ_IDENTIFIER,
                      ncx_get_modname(mod), res);
    if (!childval) {
        val_free_value(schemaval);
        return NULL;
    }
    val_add_child(childval, schemaval);

    /* create schema/version */
    childval = agt_make_leaf(schemaobj, AGT_STATE_OBJ_VERSION,
                             ncx_get_modversion(mod), res);
    if (!childval) {
        val_free_value(schemaval);
        return NULL;
    }
    val_add_child(childval, schemaval);

    /* create schema/format */
    childval = agt_make_leaf(schemaobj, AGT_STATE_OBJ_FORMAT,
                             AGT_STATE_FORMAT_YANG, res);
    if (!childval) {
        val_free_value(schemaval);
        return NULL;
    }
    val_add_child(childval, schemaval);

    /* create schema/namespace */
    childval = agt_make_leaf(schemaobj, AGT_STATE_OBJ_NAMESPACE,
                             ncx_get_modnamespace(mod), res);
    if (!childval) {
        val_free_value(schemaval);
        return NULL;
    }
    val_add_child(childval, schemaval);

    /* create schema/location */
    childval = agt_make_leaf(schemaobj, AGT_STATE_OBJ_LOCATION,
                             AGT_STATE_ENUM_NETCONF, res);
    if (!childval) {
        val_free_value(schemaval);
        return NULL;
    }
    val_add_child(childval, schemaval);

    /* create yuma extension schema/conformance; only if object found */
    obj_template_t *leafobj =
        obj_find_child(schemaobj, AGT_YWSYS_MODULE, AGT_STATE_OBJ_CONFORMANCE);
    if (leafobj) {
        const xmlChar *strval = mod->supported ? NCX_EL_TRUE : NCX_EL_FALSE;
        childval = val_make_simval_obj(leafobj, strval, res);
        if (!childval) {
            val_free_value(schemaval);
            return NULL;
        }
        val_add_child(childval, schemaval);
    }

    *res = val_gen_index_chain(schemaobj, schemaval);
    if (*res != NO_ERR) {
        val_free_value(schemaval);
        return NULL;
    }

    return schemaval;

} /* make_schema_val */


/**
 * \fn make_netconf_schema_val
 * \brief make a val_value_t struct for a specified module
 * \param mod module control block to use
 * \param schemaobj <schema> object to use
 * \param res address of return status
 * \return malloced value struct or NULL if some error
 */
static val_value_t *
    make_netconf_schema_val (obj_template_t *schemaobj,
                             status_t *res)
{
    *res = NO_ERR;

    /* create schema node */
    val_value_t *schemaval = val_new_value();
    if (!schemaval) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    val_init_from_template(schemaval, schemaobj);

    /* create schema/identifier */
    val_value_t *childval = 
        agt_make_leaf(schemaobj, AGT_STATE_OBJ_IDENTIFIER,
                      NCXMOD_IETF_NETCONF, res);
    if (!childval) {
        val_free_value(schemaval);
        return NULL;
    }
    val_add_child(childval, schemaval);

    /* create schema/version */
    childval = agt_make_leaf(schemaobj, AGT_STATE_OBJ_VERSION,
                             NCXMOD_IETF_NETCONF_REV, res);
    if (!childval) {
        val_free_value(schemaval);
        return NULL;
    }
    val_add_child(childval, schemaval);

    /* create schema/format */
    childval = agt_make_leaf(schemaobj, AGT_STATE_OBJ_FORMAT,
                             AGT_STATE_FORMAT_YANG, res);
    if (!childval) {
        val_free_value(schemaval);
        return NULL;
    }
    val_add_child(childval, schemaval);

    /* create schema/namespace */
    childval = agt_make_leaf(schemaobj, AGT_STATE_OBJ_NAMESPACE,
                             NC_URN, res);
    if (!childval) {
        val_free_value(schemaval);
        return NULL;
    }
    val_add_child(childval, schemaval);

    /* create schema/location */
    childval = agt_make_leaf(schemaobj, AGT_STATE_OBJ_LOCATION,
                             AGT_STATE_ENUM_NETCONF, res);
    if (!childval) {
        val_free_value(schemaval);
        return NULL;
    }
    val_add_child(childval, schemaval);

    /* create yuma extension schema/conformance */
    obj_template_t *leafobj =
        obj_find_child(schemaobj, AGT_YWSYS_MODULE, AGT_STATE_OBJ_CONFORMANCE);
    if (leafobj) {
        childval = val_make_simval_obj(leafobj, NCX_EL_TRUE, res);
        if (!childval) {
            val_free_value(schemaval);
            return NULL;
        }
        val_add_child(childval, schemaval);
    }

    *res = val_gen_index_chain(schemaobj, schemaval);
    if (*res != NO_ERR) {
        val_free_value(schemaval);
        return NULL;
    }

    return schemaval;

} /* make_netconf_schema_val */


// ----------------------------------------------------------------------------!

/**
 * \fn make_session_val
 * \brief make a val_value_t struct for a specified session
 * \param scb session control block to use
 * \param sessionobj <session> object to use
 * \param res address of return status
 * \return malloced value struct or NULL if some error
 */
static val_value_t *
    make_session_val (ses_cb_t *scb,
                      obj_template_t *sessionobj,
                      status_t *res)
{
    *res = NO_ERR;

    /* create session node */
    val_value_t *sessionval = val_new_value();
    if (!sessionval) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    val_init_from_template(sessionval, sessionobj);

    /* create session/sessionId */
    xmlChar numbuff[NCX_MAX_NUMLEN];
    snprintf((char *)numbuff, sizeof(numbuff), "%u", scb->sid);
    val_value_t *childval =
        agt_make_leaf(sessionobj, AGT_STATE_OBJ_SESSIONID, numbuff, res);
    if (!childval) {
        val_free_value(sessionval);
        return NULL;
    }
    val_add_child(childval, sessionval);

    /* create session/transport */
    /* hack: check protocol because very first message is always sent
     * looking as if it is SSH       */
    const xmlChar *name = ses_get_transport_name(scb->transport);
    if (ses_get_protocol(scb) == NCX_PROTO_YUMA_YANGAPI) {
        name = ses_get_transport_name(SES_TRANSPORT_HTTP);
    }

    childval = agt_make_leaf(sessionobj, AGT_STATE_OBJ_TRANSPORT, name, res);
    if (!childval) {
        val_free_value(sessionval);
        return NULL;
    }
    val_add_child(childval, sessionval);
    
    /* create session/username */
    childval = agt_make_leaf(sessionobj, AGT_STATE_OBJ_USERNAME,
                             scb->username, res);
    if (!childval) {
        val_free_value(sessionval);
        return NULL;
    }
    val_add_child(childval, sessionval);

    /* create session/sourceHost */
    childval = agt_make_leaf(sessionobj, AGT_STATE_OBJ_SOURCEHOST,
                             scb->peeraddr, res);
    if (!childval) {
        val_free_value(sessionval);
        return NULL;
    }
    val_add_child(childval, sessionval);

    /* create session/loginTime */
    childval = agt_make_leaf(sessionobj, AGT_STATE_OBJ_LOGINTIME,
                             scb->start_time, res);
    if (!childval) {
        val_free_value(sessionval);
        return NULL;
    }
    val_add_child(childval, sessionval);

    /* create session/inRpcs */
    childval = agt_make_virtual_leaf(sessionobj, AGT_STATE_OBJ_IN_RPCS,
                                     agt_ses_get_session_inRpcs, res);
    if (!childval) {
        val_free_value(sessionval);
        return NULL;
    }
    val_add_child(childval, sessionval);

    /* create session/inBadRpcs */
    childval = agt_make_virtual_leaf(sessionobj, AGT_STATE_OBJ_IN_BAD_RPCS,
                                     agt_ses_get_session_inBadRpcs, res);
    if (!childval) {
        val_free_value(sessionval);
        return NULL;
    }
    val_add_child(childval, sessionval);

    /* create session/outRpcErrors */
    childval = agt_make_virtual_leaf(sessionobj, AGT_STATE_OBJ_OUT_RPC_ERRORS,
                                     agt_ses_get_session_outRpcErrors, res);
    if (!childval) {
        val_free_value(sessionval);
        return NULL;
    }
    val_add_child(childval, sessionval);

    /* create session/outNotifications */
    childval = agt_make_virtual_leaf(sessionobj,
                                     AGT_STATE_OBJ_OUT_NOTIFICATIONS,
                                     agt_ses_get_session_outNotifications,
                                     res);
    if (!childval) {
        val_free_value(sessionval);
        return NULL;
    }
    val_add_child(childval, sessionval);

    *res = val_gen_index_chain(sessionobj, sessionval);
    if (*res != NO_ERR) {
        val_free_value(sessionval);
        return NULL;
    }

    return sessionval;

} /* make_session_val */

// ----------------------------------------------------------------------------!

/**
 * \fn make_statistics_val
 * \brief statisticsobj <statistics> object to use
 * \param res address of return status
 * \return malloced value struct or NULL if some error
 */
static val_value_t *
    make_statistics_val (obj_template_t *statisticsobj,
                         status_t *res)
{
    val_value_t            *statsval, *childval;
    xmlChar                tbuff[TSTAMP_MIN_SIZE+1];

    /* create statistics node */
    statsval = val_new_value();
    if (!statsval) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    val_init_from_template(statsval, statisticsobj);

    /* add statistics/netconfStartTime static leaf */
    tstamp_datetime(tbuff);
    childval = agt_make_leaf(statisticsobj,
                             AGT_STATE_OBJ_NETCONF_START_TIME,
                             tbuff,
                             res);
    if (!childval) {
        val_free_value(statsval);
        return NULL;
    }
    val_add_child(childval, statsval);

    /* add statistics/inBadHellos virtual leaf */
    childval = agt_make_virtual_leaf(statisticsobj,
                                     AGT_STATE_OBJ_IN_BAD_HELLOS,
                                     agt_ses_get_inBadHellos,
                                     res);
    if (!childval) {
        val_free_value(statsval);
        return NULL;
    }
    val_add_child(childval, statsval);

    /* add statistics/inSessions virtual leaf */
    childval = agt_make_virtual_leaf(statisticsobj,
                                     AGT_STATE_OBJ_IN_SESSIONS,
                                     agt_ses_get_inSessions,
                                     res);
    if (!childval) {
        val_free_value(statsval);
        return NULL;
    }
    val_add_child(childval, statsval);

    /* add statistics/droppedSessions virtual leaf */
    childval = agt_make_virtual_leaf(statisticsobj,
                                     AGT_STATE_OBJ_DROPPED_SESSIONS,
                                     agt_ses_get_droppedSessions,
                                     res);
    if (!childval) {
        val_free_value(statsval);
        return NULL;
    }
    val_add_child(childval, statsval);

    /* add statistics/inRpcs virtual leaf */
    childval = agt_make_virtual_leaf(statisticsobj,
                                     AGT_STATE_OBJ_IN_RPCS,
                                     agt_ses_get_inRpcs,
                                     res);
    if (!childval) {
        val_free_value(statsval);
        return NULL;
    }
    val_add_child(childval, statsval);

    /* add statistics/inBadRpcs virtual leaf */
    childval = agt_make_virtual_leaf(statisticsobj,
                                     AGT_STATE_OBJ_IN_BAD_RPCS,
                                     agt_ses_get_inBadRpcs,
                                     res);
    if (!childval) {
        val_free_value(statsval);
        return NULL;
    }
    val_add_child(childval, statsval);

    /* add statistics/outRpcErrors virtual leaf */
    childval = agt_make_virtual_leaf(statisticsobj,
                                     AGT_STATE_OBJ_OUT_RPC_ERRORS,
                                     agt_ses_get_outRpcErrors,
                                     res);
    if (!childval) {
        val_free_value(statsval);
        return NULL;
    }
    val_add_child(childval, statsval);

    /* add statistics/outNotifications virtual leaf */
    childval = agt_make_virtual_leaf(statisticsobj,
                                     AGT_STATE_OBJ_OUT_NOTIFICATIONS,
                                     agt_ses_get_outNotifications,
                                     res);
    if (!childval) {
        val_free_value(statsval);
        return NULL;
    }
    val_add_child(childval, statsval);

    return statsval;

}  /* make_statistics_val */


// ----------------------------------------------------------------------------!

/**
 * \fn get_schema_validate
 * \brief get-schema : validate params callback
 * \param scb session control block to use
 * \param see agt_rpc.h
 * \param see agt_rpc.h
 * \return status
 ******************************************************************************/
static status_t get_schema_validate( ses_cb_t *scb, 
                                     rpc_msg_t *msg, 
                                     xml_node_t *methnode)
{
    assert( scb && "scb is NULL" );
    assert( msg && "msg is NULL" );
    assert( methnode  && "methnode  is NULL" );

    const xmlChar *identifier = NULL;
    const xmlChar *version = NULL;
    const xmlChar *format = NULL;
    char *path = NULL;
    xmlns_id_t nsid = 0;
    status_t res = NO_ERR;
    ncx_modformat_t modformat = NCX_MODFORMAT_NONE;
    boolean isnetconf = FALSE;

    /* get the identifier parameter */
    val_value_t *validentifier = 
        val_find_child(msg->rpc_input, AGT_STATE_MODULE,
                       AGT_STATE_OBJ_IDENTIFIER);
    if (validentifier && validentifier->res == NO_ERR) {
        identifier = VAL_STR(validentifier);
        isnetconf = !xml_strcmp(identifier, NCXMOD_IETF_NETCONF);
    }

    /* get the version parameter */
    val_value_t *valversion = 
        val_find_child(msg->rpc_input, AGT_STATE_MODULE,
                       AGT_STATE_OBJ_VERSION);
    if (valversion && valversion->res == NO_ERR) {
        version = VAL_STR(valversion);
    }

    /* get the format parameter */
    val_value_t *valformat = 
        val_find_child(msg->rpc_input, AGT_STATE_MODULE,
                       AGT_STATE_OBJ_FORMAT);
    if (valformat && valformat->res == NO_ERR) {
        format = VAL_IDREF_NAME(valformat);
        nsid = VAL_IDREF_NSID(valformat);
        if (nsid != statemod->nsid) {
            res = ERR_NCX_INVALID_VALUE;
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res, 
                             methnode, 
                             NCX_NT_VAL, 
                             valformat,
                             NCX_NT_VAL, 
                             valformat);
        }
    }

    if (identifier == NULL) {
        /* should already be reported */
        return ERR_NCX_MISSING_PARM;
    }

    /* set default format to YANG if needed */
    if (format == NULL) {
        format = AGT_STATE_FORMAT_YANG;
        nsid = statemod->nsid;
    }

    /* check format parameter: only YANG and YIN supported for now */
    if (!xml_strcmp(format, AGT_STATE_FORMAT_YANG)) {
        modformat = NCX_MODFORMAT_YANG;
    } else if (!xml_strcmp(format, AGT_STATE_FORMAT_YIN)) {
        if (isnetconf) {
            modformat = NCX_MODFORMAT_NONE;  // force error below
        } else {
            modformat = NCX_MODFORMAT_YIN;
        }
    } else {
        modformat = NCX_MODFORMAT_NONE;
    }
    if (nsid != statemod->nsid) {
        res = ERR_NCX_WRONG_NS;
        agt_record_error(scb,
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res, 
                         methnode, 
                         NCX_NT_VAL, 
                         valformat,
                         NCX_NT_VAL, 
                         valformat);
    } else if (modformat == NCX_MODFORMAT_NONE) {
        res = ERR_NCX_VALUE_NOT_SUPPORTED;
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res, 
                         methnode, 
                         NCX_NT_STRING, 
                         format,
                         NCX_NT_VAL, 
                         valformat);
    }

    /* check the identifier: must be valid module name */
    if (!ncx_valid_name2(identifier)) {
        res = ERR_NCX_NO_MATCHES;
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res, 
                         methnode, 
                         NCX_NT_STRING, 
                         identifier,
                         NCX_NT_VAL, 
                         validentifier);
    }

    /* do not check the revision now 
     * it must be an exact match if non-empty string
     */
    if (version && !*version) {
        /* send NULL instead of empty string */
        version = NULL;
    }

    /* client must specify a revision if the server has more than 1 */
    if (version == NULL && !isnetconf) {
        uint32 revcount = ncx_mod_revision_count(identifier);
        if (revcount > 1) {
            res = ERR_NCX_GET_SCHEMA_DUPLICATES;
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res, 
                             methnode, 
                             NCX_NT_NONE, 
                             NULL,
                             NCX_NT_NONE, 
                             NULL);
        }
    }

    /* find the module that is loaded; this is the only format
     * that is supported at this time; TBD: registry of filespecs
     * or URLs that are supported for each [sub]module 
     * instead of just returning the format loaded into memory
     */
    if (res == NO_ERR && isnetconf) {
        /* special hack for now since yuma-netconf is loaded,
         * not ietf-netconf; ietf-netconf is advertised in<capabilities>
         */
        if (ietf_netconf_path == NULL) {
            res = ERR_NCX_OPERATION_FAILED;
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION,
                             res, 
                             methnode, 
                             NCX_NT_VAL, 
                             validentifier,
                             NCX_NT_VAL, 
                             validentifier);
        } else {
            path = (char *)ietf_netconf_path;
        }
    } else if (res == NO_ERR) {
        ncx_module_t *findmod = ncx_find_module(identifier, version);
        if (findmod == NULL) {
            val_value_t *errval = validentifier;

            res = ERR_NCX_NO_MATCHES;
            if (version != NULL) {
                findmod = ncx_find_module(identifier, NULL);
                if (findmod != NULL) {
                    errval = valversion;
                }
            }
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION,
                             res, 
                             methnode, 
                             NCX_NT_VAL, 
                             errval,
                             NCX_NT_VAL, 
                             errval);
        } else {
            path = (char *)findmod->source;

            /* check if format is correct for request */
            switch (modformat) {
            case NCX_MODFORMAT_YANG:
                if (yang_fileext_is_yin(findmod->source)) {
                    res = ERR_NCX_VALUE_NOT_SUPPORTED;
                }
                break;
            case NCX_MODFORMAT_YIN:
                if (yang_fileext_is_yang(findmod->source)) {
                    res = ERR_NCX_VALUE_NOT_SUPPORTED;
                }
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
            if (res != NO_ERR) {
                agt_record_error(scb, 
                                 &msg->mhdr, 
                                 NCX_LAYER_OPERATION, 
                                 res, 
                                 methnode, 
                                 NCX_NT_STRING, 
                                 format,
                                 (valformat) ? NCX_NT_VAL : NCX_NT_NONE,
                                 valformat);
            }
        }
    }

    if (res == NO_ERR) {
        /* save the found module filespec
         * setup the automatic output using the callback
         * function in agt_util.c
         */
        msg->rpc_user1 = (void *)path;
        msg->rpc_data_type = RPC_DATA_STD;
        msg->rpc_datacb = agt_output_schema;
    }

    return res;
} /* get_schema_validate */


/**********************************************************************
 * add_backup_file
 *
 * make a backup-file value node and add it to the parent val
 *
 * INPUTS:
 *   parentval == parent value to add child list node to
 *   fullspec == absolute or relative path spec, with filename and ext.
 *               this regular file exists
 *   mtime == last-modified time for the backup file
 *
 * RETURNS:
 *    status
 *
 *    Return fatal error to stop the traversal or NO_ERR to
 *    keep the traversal going.  Do not return any warning or
 *    recoverable error, just log and move on
 *********************************************************************/
static status_t 
    add_backup_file (val_value_t *parentval,
                     const char *fullspec,
                     const xmlChar *mtime)
{
    /* get the parentval /netconf-state/backup-files */
    if (parentval == NULL || parentval->obj == NULL) {
        return ERR_NCX_OPERATION_FAILED;
    }

    /* make a backup-file list entry and add it to the parentval */
    obj_template_t *listobj =
        obj_find_child(parentval->obj, obj_get_mod_name(parentval->obj),
                       AGT_STATE_OBJ_BACKUP_FILE);
    if (listobj == NULL) {
        return ERR_NCX_OPERATION_FAILED;
    }

    val_value_t *listval = val_new_value();
    if (listval == NULL) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(listval, listobj);

    /* add the backup-file/name key leaf */
    status_t res = NO_ERR;
    val_value_t *leafval =
        agt_make_leaf(listobj, NCX_EL_NAME, (const xmlChar *)fullspec, &res);
    if (leafval) {
        val_add_child(leafval, listval);
    } else {
        val_free_value(listval);
        return res;
    }

    /* add the backup-file/backup-time leaf */
    leafval = agt_make_leaf(listobj, AGT_STATE_OBJ_BACKUP_TIME, mtime, &res);
    if (leafval) {
        val_add_child(leafval, listval);
    } else {
        val_free_value(listval);
        return res;
    }

    /* generate the internal index Q */
    res = val_gen_index_chain(listobj, listval);
    if (res != NO_ERR) {
        val_free_value(listval);
        return res;
    }

    val_add_child_sorted(listval, parentval);
    return NO_ERR;

}  /* add_backup_file */


/**********************************************************************
 * ncxmod_backup_cbfn_t
 *
 * callback for backup file walk
 *
 * INPUTS:
 *   fullspec == absolute or relative path spec, with filename and ext.
 *               this regular file exists
 *   mtime == last-modified time for the backup file
 *   cookie == opaque handle passed from start of callbacks
 *
 * RETURNS:
 *    status
 *
 *    Return fatal error to stop the traversal or NO_ERR to
 *    keep the traversal going.  Do not return any warning or
 *    recoverable error, just log and move on
 *********************************************************************/
static status_t 
    backup_file_cbfn (const char *fullspec,
                      const xmlChar *mtime,
                      void *cookie)
{
    /* get the parentval /netconf-state/backup-files */
    val_value_t *parentval = (val_value_t *)cookie;
    if (parentval == NULL || parentval->obj == NULL) {
        return ERR_NCX_OPERATION_FAILED;
    }

    status_t res = add_backup_file(parentval, fullspec, mtime);

    return res;

}  /* backup_file_cbfn */


/************* E X T E R N A L    F U N C T I O N S ***************/

/**
 * \fn agt_state_init
 * \brief Initialize the agent state monitor module data structures
 * \return status
 */
status_t agt_state_init (void)
{
    if (agt_state_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    log_debug2("\nagt: Loading netconf-state module");

    agt_profile_t *agt_profile = agt_get_profile();

    /* ietf-netconf-monitoring support objects */
    statemod = NULL;
    mysessionsval = NULL;
    myschemasval = NULL;
    mybackupfilesval = NULL;
    mysessionobj = NULL;
    myschemaobj = NULL;
    ietf_netconf_path = NULL;
    agt_state_init_done = TRUE;

    /* load the netconf-state module */
    status_t res =
        ncxmod_load_module(AGT_STATE_MODULE, NULL, 
                           &agt_profile->agt_savedevQ, &statemod);

    if (res == NO_ERR) {
        /* load the yumaworks-system if needed. */
        agt_profile_t *profile = agt_get_profile();
        if (profile->agt_yumaworks_system) {
            res = ncxmod_load_module(AGT_YWSYS_MODULE, NULL, 
                                     &profile->agt_savedevQ, NULL);
        }
    }

    if (res == NO_ERR) {
        /* find the ietf-netconf module */
        ncxmod_search_result_t *sr = 
            ncxmod_find_module(NCXMOD_IETF_NETCONF, NULL);
        if (sr == NULL) {
            log_error("\nError: cannot find %s module for <get-schema> "
                      "support", NCXMOD_IETF_NETCONF);
        } else {
            /* steal the source string memory and transfer it
             * to the ietf-netconf path strnig   */
            ietf_netconf_path = sr->source;
            sr->source = NULL;
            ncxmod_free_search_result(sr);
        }
    }

    return res;

}  /* agt_state_init */

// ----------------------------------------------------------------------------!

/**
 * \fn agt_state_init2
 * \brief Initialize the monitoring data structures. This must be done
 * after the <running> config is loaded
 * \return status 
 */

status_t
    agt_state_init2 (void)
{
    if (!agt_state_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    /* set up get-schema RPC operation */
    status_t res =
        agt_rpc_register_method(AGT_STATE_MODULE, AGT_STATE_GET_SCHEMA,
                                AGT_RPC_PH_VALIDATE, get_schema_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    cfg_template_t *runningcfg = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (!runningcfg || !runningcfg->root) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* get all the object nodes first */
    obj_template_t  *topobj = 
        obj_find_template_top(statemod, AGT_STATE_MODULE,
                              AGT_STATE_TOP_CONTAINER);
    if (!topobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    obj_template_t *capsobj =
        obj_find_child(topobj, AGT_STATE_MODULE,
                       AGT_STATE_OBJ_CAPABILITIES);
    if (!capsobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    obj_template_t *confsobj =
        obj_find_child(topobj, AGT_STATE_MODULE,
                       AGT_STATE_OBJ_DATASTORES);
    if (!confsobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    obj_template_t *confobj =
        obj_find_child(confsobj, AGT_STATE_MODULE,
                       AGT_STATE_OBJ_DATASTORE);
    if (!confobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    obj_template_t *schemasobj = obj_find_child(topobj,
                                AGT_STATE_MODULE,
                                AGT_STATE_OBJ_SCHEMAS);
    if (!schemasobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    myschemaobj = obj_find_child(schemasobj, AGT_STATE_MODULE,
                                 AGT_STATE_OBJ_SCHEMA);
    if (!myschemaobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    obj_template_t *sessionsobj =
        obj_find_child(topobj, AGT_STATE_MODULE, AGT_STATE_OBJ_SESSIONS);
    if (!sessionsobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    mysessionobj = obj_find_child(sessionsobj, AGT_STATE_MODULE,
                                  AGT_STATE_OBJ_SESSION);
    if (!mysessionobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    obj_template_t *statisticsobj =
        obj_find_child(topobj, AGT_STATE_MODULE, AGT_STATE_OBJ_STATISTICS);
    if (!statisticsobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    obj_template_t *bkupobj = NULL;
    if (agt_backups_enabled()) {
        bkupobj = obj_find_child(topobj, AGT_YWSYS_MODULE,
                                 AGT_STATE_OBJ_BACKUP_FILES);
        if (!bkupobj) {
            return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        }
    }

    /* add /ietf-netconf-state */
    val_value_t *topval = val_new_value();
    if (!topval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(topval, topobj);

    /* handing off the malloced memory here */
    val_add_child_sorted(topval, runningcfg->root);

    /* add /ietf-netconf-state/capabilities virtual node */
    val_value_t *capsval = val_new_value();
    if (!capsval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_virtual(capsval, get_caps, capsobj);
    val_add_child(capsval, topval);

    /* add /ietf-netconf-state/datastores */
    val_value_t *confsval = val_new_value();
    if (!confsval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(confsval, confsobj);
    val_add_child(confsval, topval);

    /* add /ietf-netconf-state/datastores/datastore[1] */
    val_value_t *confval = NULL;
    if (agt_cap_std_set(CAP_STDID_CANDIDATE)) {
        confval = make_datastore_val(NCX_EL_CANDIDATE, confobj, &res);
        if (!confval) {
            return res;
        }
        val_add_child(confval, confsval);
    }

    /* add /ietf-netconf-state/datastores/datastore[2] */
    confval = make_datastore_val(NCX_EL_RUNNING, confobj, &res);
    if (!confval) {
        return res;
    }
    val_add_child(confval, confsval);

    /* add /ietf-netconf-state/datastores/datastore[3] */
    if (agt_cap_std_set(CAP_STDID_STARTUP)) {
        confval = make_datastore_val(NCX_EL_STARTUP, confobj, &res);
        if (!confval) {
            return res;
        }
        val_add_child(confval, confsval);
    }

    /* add /ietf-netconf-state/schemas */
    myschemasval = val_new_value();
    if (!myschemasval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(myschemasval, schemasobj);
    val_add_child(myschemasval, topval);

    /* add all the /ietf-netconf-state/schemas/schema nodes */
    ncx_module_t *mod;
    for (mod = ncx_get_first_module();
         mod != NULL;
         mod = ncx_get_next_module(mod)) {
        res = agt_state_add_module_schema(mod);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* add /ietf-netconf-state/sessions */
    val_value_t *sessionsval = val_new_value();
    if (!sessionsval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(sessionsval, sessionsobj);
    val_add_child(sessionsval, topval);
    mysessionsval = sessionsval;

    /* add /ietf-netconf-state/statistics */
    val_value_t *statisticsval = make_statistics_val(statisticsobj, &res);
    if (!statisticsval) {
        return ERR_INTERNAL_MEM;
    }
    val_add_child(statisticsval, topval);

    if (agt_backups_enabled()) {
        /* add yumaworks-system extension:
         * /ietf-netconf-state/backup-files virtual node */
        val_value_t *bkupval = val_new_value();
        if (!bkupval) {
            return ERR_INTERNAL_MEM;
        }
        val_init_from_template(bkupval, bkupobj);
        val_add_child(bkupval, topval);
        mybackupfilesval = bkupval;
        res = ncxmod_get_backup_files(backup_file_cbfn, bkupval);
    }

    return res;

}  /* agt_state_init2 */

// ----------------------------------------------------------------------------!

/**
 * \fn agt_state_cleanup
 * \brief Cleanup the module data structures
 * \return none
 */
void agt_state_cleanup (void)
{
    if (agt_state_init_done) {
        statemod = NULL;
        mysessionsval = NULL;
        myschemasval = NULL;
        mysessionobj = NULL;
        myschemaobj = NULL;
        m__free(ietf_netconf_path);

        agt_rpc_unregister_method(AGT_STATE_MODULE, AGT_STATE_GET_SCHEMA);

        agt_state_init_done = FALSE;
    }
}  /* agt_state_cleanup */

// ----------------------------------------------------------------------------!

/**
 * \fn agt_state_add_session
 * \brief Add a session entry to the netconf-state DM
 * \param scb session control block to use for the info
 * \return status
 */
status_t agt_state_add_session (ses_cb_t *scb)
{
    val_value_t *session;
    status_t     res;

    assert (scb && " param scb is NULL");

    res = NO_ERR;
    session = make_session_val(scb, mysessionobj, &res);
    if (!session) {
        return res;
    }

    val_add_child_sorted(session, mysessionsval);
    return NO_ERR;

}  /* agt_state_add_session */

// ----------------------------------------------------------------------------!

/**
 * \fn agt_state_remove_session
 * \brief Remove a session entry from the netconf-state DM
 * \param sid session ID to find and delete
 * \return none
 */
void
    agt_state_remove_session (ses_id_t sid)
{
    val_value_t  *sessionval, *idval;

    if (mysessionsval == NULL) {
        /* cleanup already done */
        return;
    }

    for (sessionval = val_get_first_child(mysessionsval);
         sessionval != NULL;
         sessionval = val_get_next_child(sessionval)) {

        idval = val_find_child(sessionval, 
                               AGT_STATE_MODULE, 
                               AGT_STATE_OBJ_SESSIONID);
        if (!idval) {
            SET_ERROR(ERR_INTERNAL_VAL);
        } else if (VAL_UINT(idval) == sid) {
            dlq_remove(sessionval);
            val_free_value(sessionval);
            return;
        }
    }
    /* session already removed -- ignore the error */

}  /* agt_state_remove_session */


// ----------------------------------------------------------------------------!

/**
 * \fn agt_state_add_module_schema
 * \brief Add a schema entry to the netconf-state DM
 * \param mod module to add
 * \return status
 */
status_t 
    agt_state_add_module_schema (ncx_module_t *mod)
{
    val_value_t *schema;
    status_t     res;

    assert ( mod && " param mod is NULL");

    if (!agt_advertise_module_needed(mod->name)) {
        return NO_ERR;
    }

    res = NO_ERR;
    schema = make_schema_val(mod, myschemaobj, &res);
    if (!schema) {
        return res;
    }

    val_add_child_sorted(schema, myschemasval);
    return res;

}  /* agt_state_add_module_schema */


/**
 * \fn agt_state_add_netconf_schema
 * \brief Add a schema entry to the netconf-state DM for ietf-netconf
 * \return status
 */
status_t 
    agt_state_add_netconf_schema (void)
{
    status_t res = NO_ERR;
    val_value_t *schema = make_netconf_schema_val(myschemaobj, &res);
    if (!schema) {
        return res;
    }

    val_add_child_sorted(schema, myschemasval);
    return res;

}  /* agt_state_add_netconf_schema */


/**
 * \fn agt_state_add_backup_file
 * \brief Add a backup-file entry to the netconf-state DM for the
 *  specified file
 * \param filename name of the backup file to add
 * \return status
 */
status_t 
    agt_state_add_backup_file (const xmlChar *filename)
{
    if (mybackupfilesval == NULL) {
        return NO_ERR;
    }

    xmlChar tbuff[TSTAMP_MIN_SIZE];
    tstamp_datetime(tbuff);

    status_t res = add_backup_file(mybackupfilesval, 
                                   (const char *)filename, tbuff);

    return res;
    
}  /* agt_state_add_backup_file */


/**
 * \fn agt_state_delete_backup_file
 * \brief Remove and delete a backup-file entry to the 
 *   netconf-state DM for the specified file
 * \param filename name of the backup file to delete
 * \return none
 */
void
    agt_state_delete_backup_file (const xmlChar *filename)
{
    if (mybackupfilesval == NULL) {
        return;
    }

    val_value_t *listval = val_get_first_child(mybackupfilesval);
    for (; listval; listval = val_get_next_child(listval)) {
        val_value_t *keyval = val_find_child(listval, NULL, NCX_EL_NAME);
        if (keyval && VAL_STR(keyval) &&
            !xml_strcmp(filename, VAL_STR(keyval))) {
            val_remove_child(listval);
            val_free_value(listval);
            return;
        }
    }

}  /* agt_state_delete_backup_file */


/* END file agt_state.c */
