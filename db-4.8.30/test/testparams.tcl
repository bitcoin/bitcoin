# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$

source ./include.tcl
global is_freebsd_test
global tcl_platform
global rpc_tests
global one_test
global serial_tests
set serial_tests {rep002 rep005 rep016 rep020 rep022 rep026 rep031 rep063 \
    rep078 rep079}

set subs {bigfile dead env fop lock log memp multi_repmgr mutex plat recd rep \
	repmgr rpc rsrc sdb sdbtest sec si test txn}

set test_names(bigfile)	[list bigfile001 bigfile002]
set test_names(compact) [list test111 test112 test113 test114 test115 test117]
set test_names(dead)    [list dead001 dead002 dead003 dead004 dead005 dead006 \
    dead007]
set test_names(elect)	[list rep002 rep005 rep016 rep020 rep022 rep026 \
    rep063 rep067 rep069 rep076]
set test_names(env)	[list env001 env002 env003 env004 env005 env006 \
    env007 env008 env009 env010 env011 env012 env013 env014 env015 env016 \
    env017 env018]
set test_names(fop)	[list fop001 fop002 fop003 fop004 fop005 fop006 \
    fop007 fop008]
set test_names(init)	[list rep029 rep030 rep031 rep033 rep037 rep038 rep039\
    rep055 rep060 rep061 rep062 rep070 rep072 rep084 rep085 rep086 rep087]
set test_names(inmemdb)	[list fop007 fop008 sdb013 sdb014 \
    sdb015 sdb016 sdb017 sdb018 sdb019 sdb020]
set test_names(lock)    [list lock001 lock002 lock003 lock004 lock005 lock006]
set test_names(log)     [list log001 log002 log003 log004 log005 log006 \
    log007 log008 log009]
set test_names(memp)	[list memp001 memp002 memp003 memp004]
set test_names(mutex)	[list mut001 mut002 mut003]
set test_names(plat)	[list plat001]
set test_names(recd)	[list recd001 recd002 recd003 recd004 recd005 recd006 \
    recd007 recd008 recd009 recd010 recd011 recd012 recd013 recd014 recd015 \
    recd016 recd017 recd018 recd019 recd020 recd022 recd023 recd024]
set test_names(rep)	[list rep001 rep002 rep003 rep005 rep006 rep007 \
    rep008 rep009 rep010 rep011 rep012 rep013 rep014 rep015 rep016 rep017 \
    rep018 rep019 rep020 rep021 rep022 rep023 rep024 rep025 rep026 rep027 \
    rep028 rep029 rep030 rep031 rep032 rep033 rep034 rep035 rep036 rep037 \
    rep038 rep039 rep040 rep041 rep042 rep043 rep044 rep045 rep046 rep047 \
    rep048 rep049 rep050 rep051 rep052 rep053 rep054 rep055 \
    rep058 rep060 rep061 rep062 rep063 rep064 rep065 rep066 rep067 \
    rep068 rep069 rep070 rep071 rep072 rep073 rep074 rep075 rep076 rep077 \
    rep078 rep079 rep080 rep081 rep082 rep083 rep084 rep085 rep086 rep087 rep088]
set test_names(rep_inmem) [list rep001 rep005 rep006 rep007 rep010 rep012 rep013\
    rep014 rep016 rep019 rep020 rep021 rep022 rep023 rep024 rep025 \
    rep026 rep028 rep029 rep030 rep031 rep032 rep033 rep034 rep035 \
    rep037 rep038 rep039 rep040 rep041 rep044 rep045 rep046 rep047 \
    rep048 rep049 rep050 rep051 rep052 rep053 rep054 rep055 rep060 \
    rep061 rep062 rep063 rep064 rep066 rep067 rep069 rep070 rep071 \
    rep072 rep073 rep074 rep075 rep076 rep077 rep080 ]
set test_names(repmgr)	[list repmgr001 repmgr002 repmgr003 repmgr004 \
    repmgr005 repmgr006 repmgr007 repmgr008 repmgr009 repmgr010 repmgr011 \
    repmgr012 repmgr013 repmgr014 repmgr015 repmgr016 repmgr017 repmgr018 \
    repmgr019]
set test_names(multi_repmgr) [list repmgr022 repmgr023 repmgr024 \
    repmgr025 repmgr026 repmgr027 repmgr028 repmgr029 repmgr030 repmgr031 \
    repmgr032]
set test_names(rpc)	[list rpc001 rpc002 rpc003 rpc004 rpc005 rpc006]
set test_names(rsrc)	[list rsrc001 rsrc002 rsrc003 rsrc004]
set test_names(sdb)	[list sdb001 sdb002 sdb003 sdb004 sdb005 sdb006 \
    sdb007 sdb008 sdb009 sdb010 sdb011 sdb012 sdb013 sdb014 sdb015 sdb016 \
    sdb017 sdb018 sdb019 sdb020 ]
set test_names(sdbtest)	[list sdbtest001 sdbtest002]
set test_names(sec)	[list sec001 sec002]
set test_names(si)	[list si001 si002 si003 si004 si005 si006 si007 si008]
set test_names(test)	[list test001 test002 test003 test004 test005 \
    test006 test007 test008 test009 test010 test011 test012 test013 test014 \
    test015 test016 test017 test018 test019 test020 test021 test022 test023 \
    test024 test025 test026 test027 test028 test029 test030 test031 test032 \
    test033 test034 test035 test036 test037 test038 test039 test040 test041 \
    test042 test043 test044 test045 test046 test047 test048 test049 test050 \
    test051 test052 test053 test054 test055 test056 test057 test058 test059 \
    test060 test061 test062 test063 test064 test065 test066 test067 test068 \
    test069 test070 test071 test072 test073 test074 test076 test077 \
    test078 test079 test081 test082 test083 test084 test085 test086 \
    test087 test088 test089 test090 test091 test092 test093 test094 test095 \
    test096 test097 test098 test099 test100 test101 test102 test103 test107 \
    test109 test110 test111 test112 test113 test114 test115 test116 test117 \
    test119 test120 test121 test122 test123 test125]

set test_names(txn)	[list txn001 txn002 txn003 txn004 txn005 txn006 \
    txn007 txn008 txn009 txn010 txn011 txn012 txn013 txn014]

set rpc_tests(berkeley_db_svc) [concat $test_names(test) $test_names(sdb)]
set rpc_tests(berkeley_db_cxxsvc) $test_names(test)
set rpc_tests(berkeley_db_javasvc) $test_names(test)

# FreeBSD, in version 5.4, has problems dealing with large messages
# over RPC.  Exclude those tests.  We believe these problems are
# resolved for later versions.  SR [#13542]
set freebsd_skip_tests_for_rpc [list test003 test008 test009 test012 test017 \
    test028 test081 test095 test102 test103 test119 sdb004 sdb011]
set freebsd_5_4 0
if { $is_freebsd_test } {
	set version $tcl_platform(osVersion)
	if { [is_substr $version "5.4"] } {
		set freebsd_5_4 1
	}
}
if { $freebsd_5_4 } {
	foreach svc {berkeley_db_svc berkeley_db_cxxsvc berkeley_db_javasvc} {
		foreach test $freebsd_skip_tests_for_rpc {
			set idx [lsearch -exact $rpc_tests($svc) $test]
			if { $idx >= 0 } {
				set rpc_tests($svc)\
				    [lreplace $rpc_tests($svc) $idx $idx]
			}
		}
	}
}

# JE tests are a subset of regular RPC tests -- exclude these ones.
# be fixable by modifying tests dealing with unsorted duplicates, second line
# will probably never work unless certain features are added to JE (record
# numbers, bulk get, etc.).
set je_exclude {(?x)    # Turn on extended syntax
	test(010|026|027|028|030|031|032|033|034|   # These should be fixable by
	     035|039|041|046|047|054|056|057|062|   # modifying tests to avoid
	     066|073|081|085)|                      # unsorted dups, etc.

	test(011|017|018|022|023|024|029|040|049|   # Not expected to work with
	     062|083|095)                           # JE until / unless features
						    # are added to JE (record
						    # numbers, bulk gets, etc.)
}
set rpc_tests(berkeley_dbje_svc) [lsearch -all -inline -not -regexp \
                                  $rpc_tests(berkeley_db_svc) $je_exclude]

# Source all the tests, whether we're running one or many.
foreach sub $subs {
	foreach test $test_names($sub) {
		source $test_path/$test.tcl
	}
}

# Reset test_names if we're running only one test.
if { $one_test != "ALL" } {
	foreach sub $subs {
		set test_names($sub) ""
	}
	set type [string trim $one_test 0123456789]
	set test_names($type) [list $one_test]
}

source $test_path/archive.tcl
source $test_path/backup.tcl
source $test_path/byteorder.tcl
source $test_path/dbm.tcl
source $test_path/foputils.tcl
source $test_path/hsearch.tcl
source $test_path/join.tcl
source $test_path/logtrack.tcl
source $test_path/ndbm.tcl
source $test_path/parallel.tcl
source $test_path/reputils.tcl
source $test_path/reputilsnoenv.tcl
source $test_path/sdbutils.tcl
source $test_path/shelltest.tcl
source $test_path/sijointest.tcl
source $test_path/siutils.tcl
source $test_path/testutils.tcl
source $test_path/upgrade.tcl

set parms(recd001) 0
set parms(recd002) 0
set parms(recd003) 0
set parms(recd004) 0
set parms(recd005) ""
set parms(recd006) 0
set parms(recd007) ""
set parms(recd008) {4 4}
set parms(recd009) 0
set parms(recd010) 0
set parms(recd011) {200 15 1}
set parms(recd012) {0 49 25 100 5}
set parms(recd013) 100
set parms(recd014) ""
set parms(recd015) ""
set parms(recd016) ""
set parms(recd017) 0
set parms(recd018) 10
set parms(recd019) 50
set parms(recd020) ""
set parms(recd022) ""
set parms(recd023) ""
set parms(recd024) ""
set parms(rep001) {1000 "001"}
set parms(rep002) {10 3 "002"}
set parms(rep003) "003"
set parms(rep005) ""
set parms(rep006) {1000 "006"}
set parms(rep007) {10 "007"}
set parms(rep008) {10 "008"}
set parms(rep009) {10 "009"}
set parms(rep010) {100 "010"}
set parms(rep011) "011"
set parms(rep012) {10 "012"}
set parms(rep013) {10 "013"}
set parms(rep014) {10 "014"}
set parms(rep015) {100 "015" 3}
set parms(rep016) ""
set parms(rep017) {10 "017"}
set parms(rep018) {10 "018"}
set parms(rep019) {3 "019"}
set parms(rep020) ""
set parms(rep021) {3 "021"}
set parms(rep022) ""
set parms(rep023) {10 "023"}
set parms(rep024) {1000 "024"}
set parms(rep025) {200 "025"}
set parms(rep026) ""
set parms(rep027) {1000 "027"}
set parms(rep028) {100 "028"}
set parms(rep029) {200 "029"}
set parms(rep030) {500 "030"}
set parms(rep031) {200 "031"}
set parms(rep032) {200 "032"}
set parms(rep033) {200 "033"}
set parms(rep034) {2 "034"}
set parms(rep035) {100 "035"}
set parms(rep036) {200 "036"}
set parms(rep037) {1500 "037"}
set parms(rep038) {200 "038"}
set parms(rep039) {200 "039"}
set parms(rep040) {200 "040"}
set parms(rep041) {500 "041"}
set parms(rep042) {10 "042"}
set parms(rep043) {25 "043"}
set parms(rep044) {"044"}
set parms(rep045) {"045"}
set parms(rep046) {200 "046"}
set parms(rep047) {200 "047"}
set parms(rep048) {3000 "048"}
set parms(rep049) {10 "049"}
set parms(rep050) {10 "050"}
set parms(rep051) {1000 "051"}
set parms(rep052) {200 "052"}
set parms(rep053) {200 "053"}
set parms(rep054) {200 "054"}
set parms(rep055) {200 "055"}
set parms(rep058) "058"
set parms(rep060) {200 "060"}
set parms(rep061) {500 "061"}
set parms(rep062) "062"
set parms(rep063) ""
set parms(rep064) {10 "064"}
set parms(rep065) {3}
set parms(rep066) {10 "066"}
set parms(rep067) ""
set parms(rep068) {"068"}
set parms(rep069) {200 "069"}
set parms(rep070) {200 "070"}
set parms(rep071) { 10 "071"}
set parms(rep072) {200 "072"}
set parms(rep073) {200 "073"}
set parms(rep074) {"074"}
set parms(rep075) {"075"}
set parms(rep076) ""
set parms(rep077) {"077"}
set parms(rep078) {"078"}
set parms(rep079) {"079"}
set parms(rep080) {200 "080"}
set parms(rep081) {200 "081"}
set parms(rep082) {200 "082"}
set parms(rep083) {200 "083"}
set parms(rep084) {200 "084"}
set parms(rep085) {20 "085"}
set parms(rep086) {"086"}
set parms(rep087) {200 "087"}
set parms(rep088) {20 "088"}
set parms(repmgr001) {100 "001"}
set parms(repmgr002) {100 "002"}
set parms(repmgr003) {100 "003"}
set parms(repmgr004) {100 "004"}
set parms(repmgr005) {100 "005"}
set parms(repmgr006) {1000 "006"}
set parms(repmgr007) {100 "007"}
set parms(repmgr008) {100 "008"}
set parms(repmgr009) {10 "009"}
set parms(repmgr010) {100 "010"}
set parms(repmgr011) {100 "011"}
set parms(repmgr012) {100 "012"}
set parms(repmgr013) {100 "013"}
set parms(repmgr014) {100 "014"}
set parms(repmgr015) {100 "015"}
set parms(repmgr016) {100 "016"}
set parms(repmgr017) {1000 "017"}
set parms(repmgr018) {100 "018"}
set parms(repmgr019) {100 "019"}
set parms(repmgr022) ""
set parms(repmgr023) ""
set parms(repmgr024) ""
set parms(repmgr025) ""
set parms(repmgr026) ""
set parms(repmgr027) ""
set parms(repmgr028) ""
set parms(repmgr029) ""
set parms(repmgr030) ""
set parms(repmgr031) ""
set parms(repmgr032) ""
set parms(subdb001) ""
set parms(subdb002) 10000
set parms(subdb003) 1000
set parms(subdb004) ""
set parms(subdb005) 100
set parms(subdb006) 100
set parms(subdb007) ""
set parms(subdb008) ""
set parms(subdb009) ""
set parms(subdb010) ""
set parms(subdb011) {13 10}
set parms(subdb012) ""
set parms(sdb001) ""
set parms(sdb002) 10000
set parms(sdb003) 1000
set parms(sdb004) ""
set parms(sdb005) 100
set parms(sdb006) 100
set parms(sdb007) ""
set parms(sdb008) ""
set parms(sdb009) ""
set parms(sdb010) ""
set parms(sdb011) {13 10}
set parms(sdb012) ""
set parms(sdb013) 10
set parms(sdb014) ""
set parms(sdb015) 1000
set parms(sdb016) 100
set parms(sdb017) ""
set parms(sdb018) 100
set parms(sdb019) 100
set parms(sdb020) 10
set parms(si001) {200 "001"}
set parms(si002) {200 "002"}
set parms(si003) {200 "003"}
set parms(si004) {200 "004"}
set parms(si005) {200 "005"}
set parms(si006) {200 "006"}
set parms(si007) {10 "007"}
set parms(si008) {10 "008"}
set parms(test001) {10000 0 0 "001"}
set parms(test002) 10000
set parms(test003) ""
set parms(test004) {10000 "004" 0}
set parms(test005) 10000
set parms(test006) {10000 0 "006" 5}
set parms(test007) {10000 "007" 5}
set parms(test008) {"008" 0}
set parms(test009) ""
set parms(test010) {10000 5 "010"}
set parms(test011) {10000 5 "011"}
set parms(test012)  ""
set parms(test013) 10000
set parms(test014) 10000
set parms(test015) {7500 0}
set parms(test016) 10000
set parms(test017) {0 19 "017"}
set parms(test018) 10000
set parms(test019) 10000
set parms(test020) 10000
set parms(test021) 10000
set parms(test022) ""
set parms(test023) ""
set parms(test024) 10000
set parms(test025) {10000 0 "025"}
set parms(test026) {2000 5 "026"}
set parms(test027) {100}
set parms(test028) ""
set parms(test029) 10000
set parms(test030) 10000
set parms(test031) {10000 5 "031"}
set parms(test032) {10000 5 "032" 0}
set parms(test033) {10000 5 "033"}
set parms(test034) 10000
set parms(test035) 10000
set parms(test036) 10000
set parms(test037) 100
set parms(test038) {10000 5 "038"}
set parms(test039) {10000 5 "039"}
set parms(test040) 10000
set parms(test041) 10000
set parms(test042) 1000
set parms(test043) 10000
set parms(test044) {5 10 0}
set parms(test045) 1000
set parms(test046) ""
set parms(test047) ""
set parms(test048) ""
set parms(test049) ""
set parms(test050) ""
set parms(test051) ""
set parms(test052) ""
set parms(test053) ""
set parms(test054) ""
set parms(test055) ""
set parms(test056) ""
set parms(test057) ""
set parms(test058) ""
set parms(test059) ""
set parms(test060) ""
set parms(test061) ""
set parms(test062) {200 200 "062"}
set parms(test063) ""
set parms(test064) ""
set parms(test065) ""
set parms(test066) ""
set parms(test067) {1000 "067"}
set parms(test068) ""
set parms(test069) {50 "069"}
set parms(test070) {4 2 1000 CONSUME 0 -txn "070"}
set parms(test071) {1 1 10000 CONSUME 0 -txn "071"}
set parms(test072) {512 20 "072"}
set parms(test073) {512 50 "073"}
set parms(test074) {-nextnodup 100 "074"}
set parms(test076) {1000 "076"}
set parms(test077) {1000 "077"}
set parms(test078) {100 512 "078"}
set parms(test079) {10000 512 "079" 20}
set parms(test081) {13 "081"}
set parms(test082) {-prevnodup 100 "082"}
set parms(test083) {512 5000 2}
set parms(test084) {10000 "084" 65536}
set parms(test085) {512 3 10 "085"}
set parms(test086) ""
set parms(test087) {512 50 "087"}
set parms(test088) ""
set parms(test089) 1000
set parms(test090) {10000 "090"}
set parms(test091) {4 2 1000 0 "091"}
set parms(test092) {1000}
set parms(test093) {10000 "093"}
set parms(test094) {10000 10 "094"}
set parms(test095) {"095"}
set parms(test096) {512 1000 19}
set parms(test097) {500 400}
set parms(test098) ""
set parms(test099) 10000
set parms(test100) {10000 "100"}
set parms(test101) {1000 -txn "101"}
set parms(test102) {1000 "102"}
set parms(test103) {100 4294967250 "103"}
set parms(test107) ""
set parms(test109) {"109"}
set parms(test110) {10000 3}
set parms(test111) {10000 "111"}
set parms(test112) {80000 "112"}
set parms(test113) {10000 5 "113"}
set parms(test114) {10000 "114"}
set parms(test115) {10000 "115"}
set parms(test116) {"116"}
set parms(test117) {10000 "117"}
set parms(test119) {"119"}
set parms(test120) {"120"}
set parms(test121) {"121"}
set parms(test122) {"122"}
set parms(test123) ""
set parms(test125) ""

# RPC server executables.  Each of these is tested (if it exists)
# when running the RPC tests.
set svc_list { berkeley_db_svc berkeley_db_cxxsvc \
    berkeley_db_javasvc berkeley_dbje_svc }
set rpc_svc berkeley_db_svc

# Shell script tests.  Each list entry is a {directory filename} pair,
# invoked with "/bin/sh filename".
set shelltest_list {
	{ scr001	chk.code }
	{ scr002	chk.def }
	{ scr003	chk.define }
	{ scr004	chk.javafiles }
	{ scr005	chk.nl }
	{ scr006	chk.offt }
	{ scr007	chk.proto }
	{ scr008	chk.pubdef }
	{ scr009	chk.srcfiles }
	{ scr010	chk.str }
	{ scr011	chk.tags }
	{ scr012	chk.vx_code }
	{ scr013	chk.stats }
	{ scr014	chk.err }
	{ scr015	chk.cxxtests }
	{ scr016	chk.bdb }
	{ scr017	chk.db185 }
	{ scr018	chk.comma }
	{ scr019	chk.include }
	{ scr020	chk.inc }
	{ scr021	chk.flags }
	{ scr022	chk.rr }
	{ scr023	chk.q }
	{ scr024	chk.bdb }
	{ scr025	chk.cxxmulti }
	{ scr026	chk.method }
	{ scr027	chk.javas }
	{ scr028	chk.rtc }
	{ scr029	chk.get }
	{ scr030	chk.build }
	{ scr031	chk.copy }
	{ scr032	chk.rpc }
	{ scr033	chk.codegen }
	{ scr034	chk.mtx }
	{ scr035	chk.osdir }
}
