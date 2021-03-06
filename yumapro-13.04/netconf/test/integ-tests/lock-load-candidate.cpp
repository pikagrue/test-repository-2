#define BOOST_TEST_MODULE IntegTestLockLoadCandidate

#include "configure-yuma-integtest.h"

namespace YumaTest {

// ---------------------------------------------------------------------------|
// Initialise the spoofed command line arguments 
// ---------------------------------------------------------------------------|
const char* SpoofedArgs::argv[] = {
    ( "yuma-test" ),
    ( "--modpath=../../modules/netconfcentral"
               ":../../modules/ietf"
               ":../../modules/yang"
               ":../modules/yang"
               ":../../modules/test/pass"
               ":../../modules/test/fail" ),
    ( "--runpath=../modules/sil" ),
    ( "--access-control=off" ),
    ( "--log=./yuma-op/test-lock-load-candidate.txt" ),
    ( "--log-level=debug3" ),
    ( "--no-config" ),          // ignore /etc/yumapro/netconfd-pro.conf
    ( "--target=candidate" ),
};

#include "define-yuma-integtest-global-fixture.h"

} // namespace YumaTest
