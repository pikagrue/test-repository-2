
#ifndef _H_agt_ietf_notif
#define _H_agt_ietf_notif
/* 
 * Copyright (c) 2008-2012, Andy Bierman, All Rights Reserved.
 * Copyright (c) 2012, YumaWorks, Inc., All Rights Reserved.
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *

*** Generated by yangdump-pro 12.09.29bd99a-M

    Combined SIL header
    module ietf-netconf-notifications
    revision 2012-02-06
    namespace urn:ietf:params:xml:ns:yang:ietf-netconf-notifications
    organization IETF NETCONF (Network Configuration Protocol) Working Group

 */

#include <xmlstring.h>

#include "cap.h"
#include "dlq.h"
#include "ncxtypes.h"
#include "op.h"
#include "status.h"
#include "val.h"

#ifdef __cplusplus
extern "C" {
#endif

#define y_ietf_netconf_notifications_M_ietf_netconf_notifications \
    (const xmlChar *)"ietf-netconf-notifications"
#define y_ietf_netconf_notifications_R_ietf_netconf_notifications \
    (const xmlChar *)"2012-02-06"

#define y_ietf_netconf_notifications_N_added_capability \
    (const xmlChar *)"added-capability"

#define y_ietf_netconf_notifications_N_by_user (const xmlChar *)"by-user"
#define y_ietf_netconf_notifications_N_changed_by (const xmlChar *)"changed-by"

#define y_ietf_netconf_notifications_N_confirm_event \
    (const xmlChar *)"confirm-event"
#define y_ietf_netconf_notifications_N_datastore (const xmlChar *)"datastore"
#define y_ietf_netconf_notifications_N_deleted_capability \
    (const xmlChar *)"deleted-capability"
#define y_ietf_netconf_notifications_N_edit (const xmlChar *)"edit"
#define y_ietf_netconf_notifications_N_killed_by (const xmlChar *)"killed-by"
#define y_ietf_netconf_notifications_N_modified_capability \
    (const xmlChar *)"modified-capability"
#define y_ietf_netconf_notifications_N_netconf_capability_change \
    (const xmlChar *)"netconf-capability-change"
#define y_ietf_netconf_notifications_N_netconf_config_change \
    (const xmlChar *)"netconf-config-change"
#define y_ietf_netconf_notifications_N_netconf_confirmed_commit \
    (const xmlChar *)"netconf-confirmed-commit"
#define y_ietf_netconf_notifications_N_netconf_session_end \
    (const xmlChar *)"netconf-session-end"
#define y_ietf_netconf_notifications_N_netconf_session_start \
    (const xmlChar *)"netconf-session-start"
#define y_ietf_netconf_notifications_N_operation (const xmlChar *)"operation"
#define y_ietf_netconf_notifications_N_server (const xmlChar *)"server"
#define y_ietf_netconf_notifications_N_server_or_user \
    (const xmlChar *)"server-or-user"
#define y_ietf_netconf_notifications_N_session_id (const xmlChar *)"session-id"
#define y_ietf_netconf_notifications_N_source_host \
    (const xmlChar *)"source-host"
#define y_ietf_netconf_notifications_N_target (const xmlChar *)"target"
#define y_ietf_netconf_notifications_N_termination_reason \
    (const xmlChar *)"termination-reason"
#define y_ietf_netconf_notifications_N_timeout (const xmlChar *)"timeout"
#define y_ietf_netconf_notifications_N_username (const xmlChar *)"username"


/********************************************************************
* FUNCTION y_ietf_netconf_notifications_netconf_config_change_send
* 
* Send a y_ietf_netconf_notifications_netconf_config_change notification
* Called by your code when notification event occurs
* 
********************************************************************/
extern void y_ietf_netconf_notifications_netconf_config_change_send 
    (const xmlChar *username,
     uint32 session_id,
     const xmlChar *source_host,
     const xmlChar *datastore,
     dlq_hdr_t *auditrecQ);


/********************************************************************
* FUNCTION y_ietf_netconf_notifications_netconf_capability_change_send
* 
* Send a y_ietf_netconf_notifications_netconf_capability_change notification
* Called by your code when notification event occurs
* 
********************************************************************/
extern void y_ietf_netconf_notifications_netconf_capability_change_send
    (const xmlChar *username,
     uint32 session_id,
     const xmlChar *source_host,
     cap_change_t cap_change,
     const xmlChar *capstr);


/********************************************************************
* FUNCTION y_ietf_netconf_notifications_netconf_session_start_send
* 
* Send a y_ietf_netconf_notifications_netconf_session_start notification
* Called by your code when notification event occurs
* 
********************************************************************/
extern void y_ietf_netconf_notifications_netconf_session_start_send (
    const xmlChar *username,
    uint32 session_id,
    const xmlChar *source_host);


/********************************************************************
* FUNCTION y_ietf_netconf_notifications_netconf_session_end_send
* 
* Send a y_ietf_netconf_notifications_netconf_session_end notification
* Called by your code when notification event occurs
* 
********************************************************************/
extern void y_ietf_netconf_notifications_netconf_session_end_send (
    const xmlChar *username,
    uint32 session_id,
    const xmlChar *source_host,
    uint32 killed_by,
    const xmlChar *termination_reason);


/********************************************************************
* FUNCTION y_ietf_netconf_notifications_netconf_confirmed_commit_send
* 
* Send a y_ietf_netconf_notifications_netconf_confirmed_commit notification
* Called by your code when notification event occurs
* 
********************************************************************/
extern void y_ietf_netconf_notifications_netconf_confirmed_commit_send (
    const xmlChar *username,
    uint32 session_id,
    const xmlChar *source_host,
    const xmlChar *confirm_event,
    ncx_confirm_event_t event,
    uint32 timeout);

/********************************************************************
* FUNCTION y_ietf_netconf_notifications_init
* 
* initialize the ietf-netconf-notifications server instrumentation library
* 
* RETURNS:
*     error status
********************************************************************/
extern status_t y_ietf_netconf_notifications_init (void);


/********************************************************************
* FUNCTION y_ietf_netconf_notifications_init2
* 
* SIL init phase 2: non-config data structures
* Called after running config is loaded
* 
* RETURNS:
*     error status
********************************************************************/
extern status_t y_ietf_netconf_notifications_init2 (void);

/********************************************************************
* FUNCTION y_ietf_netconf_notifications_cleanup
*    cleanup the server instrumentation library
* 
********************************************************************/
extern void y_ietf_netconf_notifications_cleanup (void);

#ifdef __cplusplus
} /* end extern 'C' */
#endif

#endif