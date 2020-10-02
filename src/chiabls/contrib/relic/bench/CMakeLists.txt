macro(ADD_MODULE MODULE)
	add_executable(bench_${MODULE} bench_${MODULE}.c)
	if (STLIB)
		target_link_libraries(bench_${MODULE} ${RELIC_S})
	else(STLIB)
		if (SHLIB)
			target_link_libraries(bench_${MODULE} ${RELIC})
		endif(SHLIB)
	endif(STLIB)
	if (STBIN)
		set_target_properties(bench_${MODULE} PROPERTIES LINK_SEARCH_END_STATIC 1)
	endif(STBIN)
	add_test(bench_${MODULE} ${SIMUL} ${SIMAR} ${EXECUTABLE_OUTPUT_PATH}/bench_${MODULE})
endmacro(ADD_MODULE)

if (CHECK)
	ADD_MODULE(err)
endif(CHECK)

if (WITH_BN)
	ADD_MODULE(bn)
endif(WITH_BN)

if (WITH_DV)
	ADD_MODULE(dv)
endif(WITH_DV)

if (WITH_FP)
	ADD_MODULE(fp)
endif(WITH_FP)

if (WITH_FPX)
	ADD_MODULE(fpx)
endif(WITH_FPX)

if (WITH_FB)
	ADD_MODULE(fb)
endif(WITH_FB)

if (WITH_FBX)
	ADD_MODULE(fbx)
endif(WITH_FBX)

if (WITH_EP)
	ADD_MODULE(ep)
endif(WITH_EP)

if (WITH_EPX)
	ADD_MODULE(epx)
endif(WITH_EPX)

if (WITH_EB)
	ADD_MODULE(eb)
endif(WITH_EB)

if (WITH_ED)
	ADD_MODULE(ed)
endif (WITH_ED)

if (WITH_EC)
	ADD_MODULE(ec)
endif(WITH_EC)

if (WITH_PP)
	ADD_MODULE(pp)
endif(WITH_PP)

if (WITH_PC)
	ADD_MODULE(pc)
endif(WITH_PC)

if (WITH_CP)
	ADD_MODULE(cp)
endif(WITH_CP)

ADD_MODULE(rand)
