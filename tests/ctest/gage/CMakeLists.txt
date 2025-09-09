
teem_create_test_bin(gage_tsbp tsbp.c)
teem_add_test(gage_tsbp ARGS -sbp 0-5-4.2)

teem_create_test_bin(gage_probeScl probeScl.c)
teem_add_test(gage_probeScl)

teem_create_test_bin(gage_probePolynomial probePolynomial.c)
teem_add_test(gage_probePolynomial)

teem_create_test_bin(gage_probeMulti probeMulti.c)
teem_add_test(gage_probeMulti)
