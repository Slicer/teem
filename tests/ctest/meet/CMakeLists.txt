
teem_create_test_bin(meet_enmall enmall.c)
teem_add_test(meet_enmall)

teem_create_test_bin(meet_kernall kernall.c)
teem_add_test(meet_kernall)

teem_create_test_bin(meet_buildinfo buildinfo.c)
teem_add_test(meet_buildinfo)

teem_create_test_bin(meet_probeSS probeSS.c)
teem_add_test(meet_probeSS NAME meet_probeSS_box01  ARGS -k box -supp 1.0 -pnum 1500)
teem_add_test(meet_probeSS NAME meet_probeSS_cos01  ARGS -k cos -supp 1.0 -pnum 1500)
teem_add_test(meet_probeSS NAME meet_probeSS_cos02  ARGS -k cos -supp 2.0 -pnum 1200)
teem_add_test(meet_probeSS NAME meet_probeSS_cos04  ARGS -k cos -supp 4.0 -pnum 1000)
teem_add_test(meet_probeSS NAME meet_probeSS_cos10  ARGS -k cos -supp 9.0 -pnum 800)
teem_add_test(meet_probeSS NAME meet_probeSS_ctmr02 ARGS -k ctmr -supp 2.0 -pnum 1300)
teem_add_test(meet_probeSS NAME meet_probeSS_ctmr04 ARGS -k ctmr -supp 4.0 -pnum 1300)
teem_add_test(meet_probeSS NAME meet_probeSS_ctmr10 ARGS -k ctmr -supp 9.0 -pnum 1300)
